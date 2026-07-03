#include "elm327_ble_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h"
#include "esp_bt_defs.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_memory_utils.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "app_obd_dsp/aux_sensor_demand.h"
#include "app_obd_dsp/obd_poll_schedule_logic.h"
#include "app_obd_dsp/obd_data_cache.h"
#include "app_obd_dsp/zc6_gear_monitor_decode.h"
#include "app_obd_dsp/zc6_gforce_monitor_decode.h"
#include "app_obd_dsp/vehicle_profiles.h"
#include "app_obd_dsp/zc6_gforce_decode.h"
#include "bsp_obd_dsp/ble_scan_buffer_profile.h"
#include "racechrono_ble_diy.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include "nvs_storage.h"

// UUID 鐢悂鍣?
#define UUID16_OBD_SERVICE      0xFFF0  // 鐢瓕顫咵LM327 BLE闁倿鍘ら崳?(FFF1閸?FFF2闁氨鐓?
#define UUID16_OBD_SERVICE_18F0  0x18F0  // IOS-Vlink / Vlink (2AF1閸?2AF0闁氨鐓?
#define UUID16_OBD_SERVICE_FF12  0xFF12  // 闁劌鍨嶸iecar缁涘鈧倿鍘ら崳銊╁帳缂冾喗婀囬崝?FF15閸?FF14闁氨鐓?
#define UUID16_OBD_WRITE_CHAR    0xFFF1
#define UUID16_CCCD              0x2902

static const char *TAG = "elm327_ble";

typedef enum {
    OBD_POLL_SLOT_RPM = 0,
    OBD_POLL_SLOT_IAT,
    OBD_POLL_SLOT_SPEED,
    OBD_POLL_SLOT_CLT,
    OBD_POLL_SLOT_LOAD,
    OBD_POLL_SLOT_TPS,
    OBD_POLL_SLOT_OIL,
    OBD_POLL_SLOT_BAT,
    OBD_POLL_SLOT_BOOST,
} obd_poll_slot_t;

typedef enum {
    OBD_STREAM_MODE_PID_POLL = 0,
    OBD_STREAM_MODE_GFORCE_MONITOR,
    OBD_STREAM_MODE_ZC6_GEAR_MONITOR,
} obd_stream_mode_t;

#define OBD_IDLE_NO_DEMAND_DELAY_MS 200u

static esp_gatt_if_t s_gattc_if = 0;
static uint16_t s_conn_id = 0xFFFF;
static esp_bd_addr_t s_peer_bda = {0};
static bool s_connected = false;
static bool s_have_service = false;
static uint16_t s_service_start = 0x0001, s_service_end = 0xFFFF; // default: full range
static uint16_t s_all_attr_end = 0xFFFF; // tracks highest seen end handle
static bool s_have_18f0 = false;          // 0x18F0 閺堝秴濮?(IOS-Vlink 閻喐顒淥BD闁矮淇婇張宥呭)
static uint16_t s_18f0_start = 0, s_18f0_end = 0;
static bool s_have_ff12 = false;          // 0xFF12 閺堝秴濮?(婢跺洭鈧?
static uint16_t s_ff12_start = 0, s_ff12_end = 0;
static uint16_t s_char_write_handle = 0; // FFF1
static uint16_t s_char_notify_handle = 0; // 娴兼ê鍘?FFF2閿涘本鐥呴張澶婂灟閸ョ偠鎯?FFF1
static uint16_t s_cccd_handle = 0;
static esp_gatt_write_type_t s_write_type = ESP_GATT_WRITE_TYPE_RSP; // 閸愭瑧琚崹瀣剁礉閺嶈宓侀悧鐟扮窙鐏炵偞鈧嗗殰閸斻劑鈧瀚?
static elm327_ble_callbacks_t s_cbs = {0};
static char s_target_name[32] = "OBDII";

// ---- 閹殿偅寮垮Ο鈥崇础閻╃鍙?----
static bool s_scan_only_mode = false;  // true=娴犲懏澹傞幓蹇庣瑝鏉╃偞甯?
static ble_scan_found_cb_t s_scan_cb = NULL;
static ble_scan_result_t *s_scan_list = NULL;
static int s_scan_count = 0;
static bool s_ble_inited = false;  // BLE 閸楀繗顔呴弽鍫熸Ц閸氾箑鍑￠崚婵嗩潗閸?
static bool s_poll_task_started = false; // 鏉烆喛顕楁禒璇插閺勵垰鎯佸鎻掑灡瀵?
static uint8_t s_oil_query_mode = 0;     // 瑜版挸澧犻弻銉嚄濡€崇础缁便垹绱?(0-2)
static int s_mode21_oil_idx = 33;        // Mode21 閺堢儤琛ュ〒鈺佺摟閼哄倻鍌ㄥ鏇礉閼奉亪鈧倸绨查弴瀛樻煀
static int16_t s_last_mode21_oil = -100; // 娑撳﹥顐糓ode21鐟欙絾鐎介崙铏规畱濞岃淇?

// ---- 閸╄桨绨潪锕€鐎烽惃鍕ˉ濞撯晜鐓＄拠銏㈢摜閻?----
static oil_temp_query_mode_t s_oil_mode_priority[4] = {
    OIL_TEMP_MODE_PID_5C,
    OIL_TEMP_MODE_UDS_22_10_17,
    OIL_TEMP_MODE_TOYOTA_21_01,
};  // 姒涙顓绘导妯哄帥缁狙嶇礉閸氼垰濮╅崥搴濈矤鏉烇箑鐎烽柊宥囩枂閺囧瓨鏌?
static uint32_t s_oil_mode_fail_count[5] = {0};  // 濮ｅ繋閲滃Ο鈥崇础(poll idx 0~4)閻ㄥ嫯绻涚紒顓炪亼鐠愩儲顐奸弫?
#define OIL_MODE_FAIL_THRESHOLD 5  // 閺屾劖膩瀵繐銇戠拹銉︻偧閺佹媽鎻崚鐗堫劃閸婄厧鎮楅幍宥呭瀼閹广垹鍩屾稉瀣╃娑?
static bool s_vehicle_profile_inited = false;

// 濞岃淇拠濠冩焽缂佺喕顓?
static struct {
    uint32_t mode0_ok;  // 01 5C 閹存劕濮涘▎鈩冩殶
    uint32_t mode1_ok;  // 22 10 17 閹存劕濮涘▎鈩冩殶
    uint32_t mode2_ok;  // 21 01 閹存劕濮涘▎鈩冩殶
    uint32_t mode3_ok;  // 22 11 1F (Mazda) 閹存劕濮涘▎鈩冩殶
    uint32_t mode4_ok;  // 22 13 10 (Mazda) 閹存劕濮涘▎鈩冩殶
    uint32_t mode0_fail;
    uint32_t mode1_fail;
    uint32_t mode2_fail;
    uint32_t mode3_fail;
    uint32_t mode4_fail;
    int16_t last_raw_temp; // 閸樼喎顫愬〒鈺佸閿涘牊婀潻鍥ㄦ姢閿?
    int16_t last_filtered_temp; // 鏉╁洦鎶ら崥搴刊鎼?
} s_oil_diag = {0};

static int8_t s_oil_temp_offset = 0;  // 閻劍鍩涢弽鈥冲櫙閸嬪繒些闁插骏绱濋崡鏇氱秴 鎺矯

// 婢х偛濮為崗銊ョ湰 ready 閺嶅洤绻?
static volatile bool s_elm_ready = true; // 閸掓繂顫愰崗浣筋啅閸欐垿鈧線顩婚弶?ATZ
static volatile bool s_expect_mode21 = false; // true=娑撳﹥娼崨鎴掓姢閺?21 01閿涘瞼鐡戝?61 01 閸濆秴绨?
static volatile bool s_gforce_monitor_active = false;
static volatile bool s_zc6_gear_monitor_active = false;
bool elm327_ble_send_ascii_blocking(const char *ascii_cmd);

#define GFORCE_MONITOR_BUF_SIZE 160
static char s_gforce_monitor_buf[GFORCE_MONITOR_BUF_SIZE];
static size_t s_gforce_monitor_len = 0;
static int64_t s_gforce_monitor_last_log_us = 0;

#define ZC6_GEAR_MONITOR_BUF_SIZE 160
static char s_zc6_gear_monitor_buf[ZC6_GEAR_MONITOR_BUF_SIZE];
static size_t s_zc6_gear_monitor_len = 0;
static int64_t s_zc6_gear_monitor_last_log_us = 0;

static ble_scan_result_t *ble_scan_buffer_alloc(void)
{
    size_t bytes = BLE_SCAN_BUFFER_BYTES(BLE_SCAN_BUFFER_CAPACITY);
    ble_scan_result_t *buffer = heap_caps_calloc(BLE_SCAN_BUFFER_CAPACITY,
                                                 sizeof(ble_scan_result_t),
                                                 MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (buffer != NULL) {
        ESP_LOGI(TAG, "Allocated BLE scan buffer in PSRAM (%u bytes)", (unsigned)bytes);
        return buffer;
    }

    buffer = heap_caps_calloc(BLE_SCAN_BUFFER_CAPACITY,
                              sizeof(ble_scan_result_t),
                              MALLOC_CAP_8BIT);
    if (buffer != NULL) {
        ESP_LOGW(TAG, "Falling back to internal RAM for BLE scan buffer (%u bytes)", (unsigned)bytes);
    }
    return buffer;
}

static bool ble_scan_buffer_ensure(void)
{
    if (s_scan_list != NULL) {
        return true;
    }

    s_scan_list = ble_scan_buffer_alloc();
    if (s_scan_list == NULL) {
        ESP_LOGE(TAG, "Failed to allocate BLE scan buffer (%u bytes)",
                 (unsigned)BLE_SCAN_BUFFER_BYTES(BLE_SCAN_BUFFER_CAPACITY));
        return false;
    }

    ESP_LOGI(TAG, "BLE scan buffer ready: capacity=%u psram=%s",
             (unsigned)BLE_SCAN_BUFFER_CAPACITY,
             esp_ptr_external_ram(s_scan_list) ? "yes" : "no");
    return true;
}

// 婢舵艾瀵橀崫宥呯安缁鳖垳袧缂傛挸鍟块崠鐚寸礄21 01 缁涘鏆遍崫宥呯安閸掑棗顦挎稉鐙烲E閸栧拑绱?
#define ACCUM_BUF_SIZE 512
static char s_accum_buf[ACCUM_BUF_SIZE];
static size_t s_accum_len = 0;
static int64_t s_accum_start_us = 0; // 缁鳖垳袧瀵偓婵妞傞梻?(us)

// ---- 閼奉亜濮╅崡蹇氼唴濡偓濞村娴夐崗?----
static volatile int s_protocol_detect_idx = -1;  // -1=閺堫亜婀Λ鈧ù瀣剁礉0-10=濮濓絽婀亸婵婄槸閸楀繗顔呴崣?
static volatile bool s_protocol_detect_got_response = false;  // 閺勵垰鎯侀弨璺哄煂閺堝鏅ラ崫宥呯安
static volatile int32_t s_protocol_detect_rpm = -1;  // 濡偓濞村鍩岄惃?RPM 閸婄》绱?1=閺冪媴绱?

// ---- 濞岃淇弻銉嚄濡€崇础鏉烆剚宕?----
// 鐏?oil_temp_query_mode_t 鏉烆剚宕叉稉楦跨枂鐠囥垻鍌ㄥ?(0-2)
static inline uint8_t oil_mode_to_poll_idx(oil_temp_query_mode_t mode) {
    switch (mode) {
        case OIL_TEMP_MODE_PID_5C: return 0;
        case OIL_TEMP_MODE_UDS_22_10_17: return 1;
        case OIL_TEMP_MODE_TOYOTA_21_01: return 2;
        case OIL_TEMP_MODE_MAZDA_22_111F: return 3;
        case OIL_TEMP_MODE_MAZDA_22_1310: return 4;
        default: return 0;
    }
}

static uint8_t get_active_vehicle_idx_safe(void) {
    // 鐡掑﹦鏅悽?vehicle_profile_get 瑜版帡娴傞敍宀冪箹闁插瞼娲块幒銉ㄧ箲閸ョ偛缍嬮崜宥囧偍瀵洏鈧?
    return nvs_cfg_get()->vehicle_profile_idx;
}

static const char *get_vehicle_fixed_header_cmd(void) {
    uint8_t vidx = get_active_vehicle_idx_safe();
    if (vidx == 0) {
        return "ATSH7E0\r"; // ZC6 閸ュ搫鐣剧拠閿嬬湴婢?
    }
    return "ATSH7E0\r";     // ZD8 閸忓牅濞囬悽銊ユ倱娑撯偓鐠囬攱鐪版径杈剧礉闁灝鍘ら懛顏勫З濠曞倻些
}

// 閸掓繂顫愰崠鏍ㄨˉ濞撯晜鐓＄拠銏㈢摜閻ｃ儻绱欐禒搴ゆ簠閸ㄥ鍘ょ純顔款嚢閸?primary/secondary/tertiary 娴兼ê鍘涚痪褔鎽奸敍?
static void init_oil_temp_strategy(void) {
    // 閸?profile 閻ㄥ嫬鐣弫缈犵喘閸忓牏楠囬柧鎾呯窗primary 鏉╃偟鐢绘径杈Е(闂冨牆鈧吋顐?閸氬骸娲栭柅鈧崚?secondary閵嗕辜ertiary閵?
    // ZC6/ZD8 閻?secondary/tertiary 瀹歌尙鐤?NONE 閳?娴犲秵妲搁崡鏇氱閸ュ搫鐣惧Ο鈥崇础(鐞涘奔璐熸稉宥呭綁)閿?
    // MX-5 = 1310 閳?閸ョ偤鈧偓 111F閿涘奔琚辩粔?Mazda 濞岃淇?PID 闁€燁洬閻╂牓鈧?
    const oil_temp_strategy_t *st = vehicle_profile_get_oil_temp_strategy();
    s_oil_mode_priority[0] = st ? st->primary   : OIL_TEMP_MODE_PID_5C;
    s_oil_mode_priority[1] = st ? st->secondary : OIL_TEMP_MODE_NONE;
    s_oil_mode_priority[2] = st ? st->tertiary  : OIL_TEMP_MODE_NONE;
    if (s_oil_mode_priority[0] == OIL_TEMP_MODE_TOYOTA_21_01) {
        s_mode21_oil_idx = 33; // ZC6 姒涙顓婚崶鍝勭暰缁便垹绱?
    }

    // 闁插秶鐤嗘径杈Е鐠佲剝鏆?
    memset(s_oil_mode_fail_count, 0, sizeof(s_oil_mode_fail_count));
    s_oil_query_mode = 0;
    s_vehicle_profile_inited = true;
    
    const vehicle_profile_t *profile = vehicle_profile_get_active();
    ESP_LOGI(TAG, "Oil temp strategy fixed for [%s]: Primary=%d",
             profile ? profile->name : "UNKNOWN", s_oil_mode_priority[0]);
}

// 閺嶈宓佽ぐ鎾冲缁涙牜鏆愰柅澶嬪娑撳绔存稉顏呯叀鐠囥垺膩瀵?
static oil_temp_query_mode_t get_next_oil_query_mode(uint8_t *poll_idx) {
    if (!s_vehicle_profile_inited) {
        init_oil_temp_strategy();
    }
    
    // 闁秴宸绘导妯哄帥缁狙冨灙鐞涱煉绱濋幍鎯у煂妫ｆ牔閲滈張澶嬫櫏娑撴梹婀潻鍥у婢惰精瑙﹂惃鍕佸?
    for (int i = 0; i < 3; i++) {
        oil_temp_query_mode_t mode = s_oil_mode_priority[i];
        if (mode == OIL_TEMP_MODE_NONE) continue;
        
        uint8_t idx = oil_mode_to_poll_idx(mode);
        // 婵″倹鐏夋潻娆庨嚋濡€崇础婢惰精瑙﹀▎鈩冩殶鐏忔垳绨梼鍫濃偓纭风礉閹存牞鈧懏澧嶉張澶嬆佸蹇涘厴鐡掑懓绻冮梼鍫濃偓闂寸啊閿涘矂鍣伴悽銊ㄧ箹娑?
        if (s_oil_mode_fail_count[idx] < OIL_MODE_FAIL_THRESHOLD) {
            *poll_idx = idx;
            return mode;
        }
    }
    
    // 閹碘偓閺堝膩瀵繘鍏樻径杈Е鏉╁洤顦块敍宀勫櫢缂冾喖銇戠拹銉吀閺佹澘鑻熸担璺ㄦ暏妫ｆ牞顩﹀Ο鈥崇础
    memset(s_oil_mode_fail_count, 0, sizeof(s_oil_mode_fail_count));
    *poll_idx = oil_mode_to_poll_idx(s_oil_mode_priority[0]);
    return s_oil_mode_priority[0];
}

// 鐠佹澘缍嶅▽瑙勪刊閺屻儴顕楅惃鍕灇閸?婢惰精瑙﹂敍鍫濆敶闁劋濞囬悽顭掔礆
static void record_oil_temp_success(oil_temp_query_mode_t mode) {
    uint8_t idx = oil_mode_to_poll_idx(mode);
    s_oil_mode_fail_count[idx] = 0;  // 閹存劕濮涢敍灞剧闂嗚泛銇戠拹銉吀閺?
    
    // 閺囧瓨鏌婄拠濠冩焽缂佺喕顓?
    switch (mode) {
        case OIL_TEMP_MODE_PID_5C:
            s_oil_diag.mode0_ok++;
            break;
        case OIL_TEMP_MODE_UDS_22_10_17:
            s_oil_diag.mode1_ok++;
            break;
        case OIL_TEMP_MODE_TOYOTA_21_01:
            s_oil_diag.mode2_ok++;
            break;
        case OIL_TEMP_MODE_MAZDA_22_111F:
            s_oil_diag.mode3_ok++;
            break;
        case OIL_TEMP_MODE_MAZDA_22_1310:
            s_oil_diag.mode4_ok++;
            break;
        default:
            break;
    }
    ESP_LOGD(TAG, "Oil temp query SUCCESS for mode %u (fail_count reset to 0)", mode);
}

static void record_oil_temp_failure(oil_temp_query_mode_t mode) {
    uint8_t idx = oil_mode_to_poll_idx(mode);
    s_oil_mode_fail_count[idx]++;
    
    // 閺囧瓨鏌婄拠濠冩焽缂佺喕顓?
    switch (mode) {
        case OIL_TEMP_MODE_PID_5C:
            s_oil_diag.mode0_fail++;
            break;
        case OIL_TEMP_MODE_UDS_22_10_17:
            s_oil_diag.mode1_fail++;
            break;
        case OIL_TEMP_MODE_TOYOTA_21_01:
            s_oil_diag.mode2_fail++;
            break;
        case OIL_TEMP_MODE_MAZDA_22_111F:
            s_oil_diag.mode3_fail++;
            break;
        case OIL_TEMP_MODE_MAZDA_22_1310:
            s_oil_diag.mode4_fail++;
            break;
        default:
            break;
    }
    ESP_LOGD(TAG, "Oil temp query FAILED for mode %u (fail_count now %u)", mode, s_oil_mode_fail_count[idx]);
}

// 姒涙顓婚崶鐐剁殶娑撳氦鐤嗙拠顫崲閸斺槄绱欓崣顖炩偓澶涚礆
static void default_on_connected(void) { ESP_LOGI(TAG, "OBD BLE connected"); }
static void default_on_disconnected(void) { ESP_LOGI(TAG, "OBD BLE disconnected"); }
static void default_on_raw_notify(const uint8_t *data, size_t len) {
    // 娴?debug 缁狙冨焼閹垫挸宓冮崢鐔奉潗閺佺増宓侀敍鍫㈡晸娴溠傜瑝鏉堟挸鍤敍?
    if (esp_log_level_get(TAG) >= ESP_LOG_DEBUG) {
        char printbuf[128] = {0};
        size_t plen = (len < sizeof(printbuf)-1) ? len : sizeof(printbuf)-1;
        for (size_t i = 0; i < plen; i++) {
            printbuf[i] = (data[i] >= 0x20 && data[i] < 0x7F) ? data[i] : '.';
        }
        ESP_LOGD(TAG, "RAW[%d]: %s", (int)len, printbuf);
    }
    // 閼汇儲甯撮弨璺哄煂 '>'閿涘矁銆冪粈?ELM 閸戝棗顦總鏂ょ礉閸欘垰褰傞柅浣风瑓娑撯偓閺?
    for (size_t i = 0; i < len; ++i) {
        if (data[i] == '>') { s_elm_ready = true; break; }
    }
}

// ---- 閸楀繗顔呴懛顏勫З濡偓濞村鍤遍弫?----
// 鐏忔繆鐦幍鈧張澶婂礂鐠侇噯绱?-11閿涘绱濋柅姘崇箖閸欐垿鈧?01 0C 閿涘牐顕?RPM閿涘娼甸崚銈嗘焽閸楀繗顔呴弰顖氭儊閺堝鏅?
static void elm327_send_standard_init_sequence(uint8_t protocol_to_use, const char *fixed_header_cmd)
{
    char atsp_cmd[16];
    const char *init_cmds[] = {
        "ATZ\r",
        "ATE0\r",
        "ATL0\r",
        "ATS1\r",
        "ATH0\r",
        "ATAT1\r",
        "ATST 19\r",
        NULL,
        NULL,
    };

    snprintf(atsp_cmd, sizeof(atsp_cmd), "ATSP%u\r", (unsigned)protocol_to_use);
    init_cmds[7] = atsp_cmd;
    init_cmds[8] = fixed_header_cmd;

    s_elm_ready = true;
    s_gforce_monitor_active = false;
    s_zc6_gear_monitor_active = false;
    s_gforce_monitor_len = 0u;
    s_gforce_monitor_last_log_us = 0;
    s_zc6_gear_monitor_len = 0u;
    s_zc6_gear_monitor_last_log_us = 0;
    s_accum_len = 0u;
    s_accum_buf[0] = '\0';
    s_expect_mode21 = false;
    obd_data_clear_actual_gear();

    for (size_t i = 0; i < sizeof(init_cmds) / sizeof(init_cmds[0]); ++i) {
        elm327_ble_send_ascii_blocking(init_cmds[i]);
        ESP_LOGI(TAG, "AT init cmd: %s", init_cmds[i]);
        vTaskDelay(pdMS_TO_TICKS(30));
    }

    elm327_ble_send_ascii_blocking("01 00\r");
    vTaskDelay(pdMS_TO_TICKS(100));
}

static void elm327_enter_gforce_monitor_mode(void)
{
    const char *monitor_cmds[] = {
        "ATE0\r",
        "ATL0\r",
        "ATS0\r",
        "ATH1\r",
        "ATCRA0D0\r",
        "ATMA\r",
    };

    s_gforce_monitor_active = false;
    s_zc6_gear_monitor_active = false;
    s_gforce_monitor_len = 0u;
    s_accum_len = 0u;
    s_accum_buf[0] = '\0';
    s_expect_mode21 = false;

    for (size_t i = 0; i < sizeof(monitor_cmds) / sizeof(monitor_cmds[0]); ++i) {
        elm327_ble_send_ascii_blocking(monitor_cmds[i]);
        vTaskDelay(pdMS_TO_TICKS((i + 1u == sizeof(monitor_cmds) / sizeof(monitor_cmds[0])) ? 80u : 30u));
    }

    s_gforce_monitor_active = true;
    ESP_LOGI(TAG, "Entered G-force OBD monitor mode for CAN 0x0D0");
}

static void elm327_exit_gforce_monitor_mode(uint8_t protocol_to_use, const char *fixed_header_cmd)
{
    s_elm_ready = true;
    elm327_ble_send_ascii_blocking("\r");
    vTaskDelay(pdMS_TO_TICKS(80));
    elm327_send_standard_init_sequence(protocol_to_use, fixed_header_cmd);
    ESP_LOGI(TAG, "Exited G-force OBD monitor mode");
}

static void elm327_enter_zc6_gear_monitor_mode(void)
{
    const char *monitor_cmds[] = {
        "ATE0\r",
        "ATL0\r",
        "ATS0\r",
        "ATH1\r",
        "ATCRA141\r",
        "ATMA\r",
    };

    s_gforce_monitor_active = false;
    s_zc6_gear_monitor_active = false;
    s_zc6_gear_monitor_len = 0u;
    s_accum_len = 0u;
    s_accum_buf[0] = '\0';
    s_expect_mode21 = false;
    obd_data_clear_actual_gear();

    for (size_t i = 0; i < sizeof(monitor_cmds) / sizeof(monitor_cmds[0]); ++i) {
        elm327_ble_send_ascii_blocking(monitor_cmds[i]);
        vTaskDelay(pdMS_TO_TICKS((i + 1u == sizeof(monitor_cmds) / sizeof(monitor_cmds[0])) ? 80u : 30u));
    }

    s_zc6_gear_monitor_active = true;
    ESP_LOGI(TAG, "Entered ZC6 gear OBD monitor mode for CAN 0x141");
}

static void elm327_exit_zc6_gear_monitor_mode(uint8_t protocol_to_use, const char *fixed_header_cmd)
{
    s_elm_ready = true;
    elm327_ble_send_ascii_blocking("\r");
    vTaskDelay(pdMS_TO_TICKS(80));
    elm327_send_standard_init_sequence(protocol_to_use, fixed_header_cmd);
    ESP_LOGI(TAG, "Exited ZC6 gear OBD monitor mode");
}

static void elm327_log_gforce_monitor_sample(const char *line, int16_t lat_x100, int16_t lon_x100)
{
    int64_t now_us;

    if (line == NULL) {
        return;
    }

    now_us = esp_timer_get_time();
    if ((now_us - s_gforce_monitor_last_log_us) < 1000000LL) {
        return;
    }

    s_gforce_monitor_last_log_us = now_us;
    ESP_LOGI(TAG,
             "GFORCE OBD sample lat=%d.%02dg lon=%d.%02dg line=%.32s",
             (int)(lat_x100 / 100),
             abs((int)(lat_x100 % 100)),
             (int)(lon_x100 / 100),
             abs((int)(lon_x100 % 100)),
             line);
}

static bool elm327_parse_gforce_monitor_line(const char *line)
{
    {
        int16_t lat_x100 = 0;
        int16_t lon_x100 = 0;

        if (!zc6_gforce_decode_monitor_line(line, &lat_x100, &lon_x100)) {
            return false;
        }

        obd_data_set_lat_accel_x100(lat_x100);
        obd_data_set_lon_accel_x100(lon_x100);
        elm327_log_gforce_monitor_sample(line, lat_x100, lon_x100);
        return true;
    }
}

static void elm327_feed_gforce_monitor_bytes(const uint8_t *data, size_t len)
{
    if (data == NULL || len == 0u) {
        return;
    }

    for (size_t i = 0u; i < len; ++i) {
        char ch = (char)data[i];

        if (ch == '>') {
            s_elm_ready = true;
            continue;
        }
        if (ch == '\r' || ch == '\n') {
            if (s_gforce_monitor_len > 0u) {
                s_gforce_monitor_buf[s_gforce_monitor_len] = '\0';
                elm327_parse_gforce_monitor_line(s_gforce_monitor_buf);
                s_gforce_monitor_len = 0u;
            }
            continue;
        }
        if ((unsigned char)ch < 0x20u || (unsigned char)ch > 0x7Eu) {
            continue;
        }
        if (s_gforce_monitor_len + 1u >= GFORCE_MONITOR_BUF_SIZE) {
            s_gforce_monitor_len = 0u;
        }
        s_gforce_monitor_buf[s_gforce_monitor_len++] = ch;
    }
}

static void elm327_log_zc6_gear_monitor_sample(const char *line, enGear gear)
{
    int64_t now_us;

    if (line == NULL) {
        return;
    }

    now_us = esp_timer_get_time();
    if ((now_us - s_zc6_gear_monitor_last_log_us) < 1000000LL) {
        return;
    }

    s_zc6_gear_monitor_last_log_us = now_us;
    ESP_LOGI(TAG, "ZC6 gear OBD sample gear=%d line=%.32s", (int)gear, line);
}

static bool elm327_parse_zc6_gear_monitor_line(const char *line)
{
    enGear gear = GEAR_NEUTRAL;

    if (!zc6_gear_decode_monitor_line(line, &gear)) {
        return false;
    }

    obd_data_set_actual_gear(gear);
    elm327_log_zc6_gear_monitor_sample(line, gear);
    return true;
}

static void elm327_feed_zc6_gear_monitor_bytes(const uint8_t *data, size_t len)
{
    if (data == NULL || len == 0u) {
        return;
    }

    for (size_t i = 0u; i < len; ++i) {
        char ch = (char)data[i];

        if (ch == '>') {
            s_elm_ready = true;
            continue;
        }
        if (ch == '\r' || ch == '\n') {
            if (s_zc6_gear_monitor_len > 0u) {
                s_zc6_gear_monitor_buf[s_zc6_gear_monitor_len] = '\0';
                elm327_parse_zc6_gear_monitor_line(s_zc6_gear_monitor_buf);
                s_zc6_gear_monitor_len = 0u;
            }
            continue;
        }
        if ((unsigned char)ch < 0x20u || (unsigned char)ch > 0x7Eu) {
            continue;
        }
        if (s_zc6_gear_monitor_len + 1u >= ZC6_GEAR_MONITOR_BUF_SIZE) {
            s_zc6_gear_monitor_len = 0u;
        }
        s_zc6_gear_monitor_buf[s_zc6_gear_monitor_len++] = ch;
    }
}

static int elm327_auto_detect_protocol(void) {
    ESP_LOGI(TAG, "=== Starting protocol auto-detect ===");
    
    // 閸忓牆褰傞柅渚€鈧氨鏁ら崚婵嗩潗閸栨牕鎳℃禒銈忕礄娑撳秵绉归崣濠傚礂鐠侇噣鈧瀚ㄩ敍?
    const char *init_cmds[] = {
        "ATZ\r",        // 婢跺秳缍?
        "ATE0\r",       // Echo off
        "ATAT1\r",      // 閼奉亪鈧倸绨查弮璺虹碍
        "ATST 19\r",    // 鐠佸墽鐤嗙搾鍛
    };
    
    for (size_t i = 0; i < sizeof(init_cmds) / sizeof(init_cmds[0]); ++i) {
        elm327_ble_send_ascii_blocking(init_cmds[i]);
        vTaskDelay(pdMS_TO_TICKS(30));
    }
    
    vTaskDelay(pdMS_TO_TICKS(200));
    
    // 鐏忔繆鐦崡蹇氼唴 1-11
    for (int proto = 1; proto <= 11; proto++) {
        ESP_LOGI(TAG, "[DETECT] Trying protocol %d...", proto);
        
        // 鐠佸墽鐤嗛崡蹇氼唴
        char atsp_cmd[16];
        snprintf(atsp_cmd, sizeof(atsp_cmd), "ATSP%d\r", proto);
        elm327_ble_send_ascii_blocking(atsp_cmd);
        vTaskDelay(pdMS_TO_TICKS(50));
        
        // 閸掓繂顫愰崠鏍梾濞村濮搁幀?
        s_protocol_detect_idx = proto;
        s_protocol_detect_got_response = false;
        s_protocol_detect_rpm = -1;
        
        // 閸欐垿鈧焦绁寸拠鏇炴嚒娴?01 0C (鐠?RPM)
        elm327_ble_send_ascii_blocking("01 0C\r");
        esp_log_level_t prev_level = esp_log_level_get(TAG);
        esp_log_level_set(TAG, ESP_LOG_INFO);
        ESP_LOGI(TAG, "[DETECT] Sent 01 0C, waiting...");
        esp_log_level_set(TAG, prev_level);
        
        // 缁涘绶熼崫宥呯安閺堚偓婢?2 缁?
        uint32_t wait_ms = 0;
        while (wait_ms < 2000) {
            vTaskDelay(pdMS_TO_TICKS(50));
            wait_ms += 50;
            
            if (s_protocol_detect_got_response) {
                // 閹存劕濮涢敍?
                s_protocol_detect_idx = -1;  // 缂佹挻娼Λ鈧ù瀣佸?
                ESP_LOGI(TAG, "[DETECT] Protocol %d: SUCCESS! (RPM=%ld)", proto, s_protocol_detect_rpm);
                return proto;
            }
        }
        
        ESP_LOGW(TAG, "[DETECT] Protocol %d: No valid response (timeout)", proto);
    }
    
    s_protocol_detect_idx = -1;  // 缂佹挻娼Λ鈧ù瀣佸?
    ESP_LOGW(TAG, "=== Protocol auto-detect FAILED ===");
    return 0;  // 鏉╂柨娲?0 鐞涖劎銇氬Λ鈧ù瀣亼鐠愩儻绱濇担璺ㄦ暏姒涙顓婚崡蹇氼唴 6
}

static void default_on_parsed_rpm(uint16_t rpm) { ESP_LOGD(TAG, "RPM: %u", rpm); obd_data_set_rpm(rpm); }
static void default_on_parsed_speed(uint8_t kmh) { ESP_LOGD(TAG, "SPEED: %u km/h", kmh); obd_data_set_speed(kmh); }
static void default_on_parsed_coolant_temp(uint32_t coolant_temp) { ESP_LOGD(TAG, "CLT: %u C", coolant_temp); obd_data_set_coolant_temp((int16_t)coolant_temp); }
static void default_on_parsed_intake_temp(uint32_t intake_temp) { ESP_LOGD(TAG, "IAT: %u C", intake_temp); obd_data_set_intake_temp((int16_t)intake_temp); }

// 閸愬懓浠堝銉ュ徔閸戣姤鏆熼敍姘安閻劍琛ュ〒鈺佷焊缁夊鍣洪崥搴＄摠閸?
static inline void obd_data_set_oil_temp_with_offset(int16_t temp) {
    int16_t adjusted = temp + s_oil_temp_offset;
    // 绾喕绻氭禒宥呮躬閺堝鏅ラ懠鍐ㄦ纯
    if (adjusted < -20) adjusted = -20;
    if (adjusted > 150) adjusted = 150;
    if (s_oil_temp_offset != 0) {
        ESP_LOGD(TAG, "OIL offset applied: %d + %d = %d", temp, s_oil_temp_offset, adjusted);
    }
    obd_data_set_oil_temp(adjusted);
}

static void default_on_parsed_oil_temp(uint32_t oil_temp)
{
    static int16_t s_oil_filtered = -100;
    static int16_t s_oil_pending = -100;
    static uint8_t s_oil_pending_cnt = 0;
    static uint32_t s_total_updates = 0;
    
    s_total_updates++;
    int16_t in = (int16_t)oil_temp;
    s_oil_diag.last_raw_temp = in;
    
    // 閸╄櫣顢呴懠鍐ㄦ纯濡偓閺屻儻绱?20 閸?150鎺矯
    if (in < -20 || in > 150) {
        ESP_LOGW(TAG, "OIL: Out of range raw=%d", in);
        return;
    }

    // ---- 閺€纭呯箻閻ㄥ嫬閽╁鎴犵暬濞?----
    // 1. 閸掓繂顫愰崠鏍▉濞堢绱伴惄瀛樺复閹恒儱褰堥崜?3 娑擃亝婀侀弫鍫濃偓?
    if (s_oil_filtered <= -40) {
        s_oil_filtered = in;
        s_oil_pending = in;
        s_oil_pending_cnt = 1;
        s_oil_diag.last_filtered_temp = in;
        ESP_LOGI(TAG, "OIL: Init with raw=%d", in);
        obd_data_set_oil_temp_with_offset(s_oil_filtered);
        return;
    }

    int diff = abs((int)in - (int)s_oil_filtered);

    // 2. 鐏忓繐褰夐崠鏍电礄<=5鎺矯閿涘绱拌箛顐︹偓鐔锋惙鎼存棑绱濋惄瀛樺复闁插洨鎾?
    if (diff <= 5) {
        s_oil_pending = -100;
        s_oil_pending_cnt = 0;
        // 閸旂姵娼堥獮鍐叉綆閿?5% 閸樺棗褰?+ 35% 閺傛澘鈧》绱欓崫宥呯安閺囨潙鎻╅敍?
        s_oil_filtered = (int16_t)((65 * s_oil_filtered + 35 * in) / 100);
        s_oil_diag.last_filtered_temp = s_oil_filtered;
        ESP_LOGD(TAG, "OIL: Small change raw=%d filtered=%d", in, s_oil_filtered);
        obd_data_set_oil_temp_with_offset(s_oil_filtered);
        return;
    }

    // 3. 娑擃厾鐡戦崣妯哄閿?~15鎺矯閿涘绱伴棁鈧憰?2 濞嗭紕鈥樼拋?
    if (diff <= 15) {
        if (s_oil_pending == in) {
            s_oil_pending_cnt++;
        } else {
            s_oil_pending = in;
            s_oil_pending_cnt = 1;
            ESP_LOGD(TAG, "OIL: Medium spike first occurrence raw=%d, need confirmation", in);
            return;
        }
        
        if (s_oil_pending_cnt >= 2) {
            // 绾喛顓婚弮鐘侯嚖閿涘矂鍣扮痪?
            s_oil_filtered = (int16_t)((50 * s_oil_filtered + 50 * in) / 100);
            s_oil_pending = -100;
            s_oil_pending_cnt = 0;
            s_oil_diag.last_filtered_temp = s_oil_filtered;
            ESP_LOGI(TAG, "OIL: Medium change confirmed raw=%d filtered=%d", in, s_oil_filtered);
            obd_data_set_oil_temp_with_offset(s_oil_filtered);
            return;
        }
        
        // 閺嗗倹妞傛稉宥咁槱閻?
        ESP_LOGD(TAG, "OIL: Medium spike pending (%u/%d)", s_oil_pending_cnt, 2);
        return;
    }

    // ---- 4. 婢堆冨綁閸栨牭绱?15鎺矯閿?---
    ESP_LOGW(TAG, "OIL: Large spike filtered=%d raw=%d (铻?%d)", s_oil_filtered, in, diff);
    
    if (diff >= 20) {
        // 闂団偓鐟?3 濞嗭紕鈥樼拋銈嗘降娣団€叉崲婢堆嗙儲閸?
        if (s_oil_pending == in) {
            s_oil_pending_cnt++;
        } else {
            s_oil_pending = in;
            s_oil_pending_cnt = 1;
            return;
        }
        
        if (s_oil_pending_cnt >= 3) {
            s_oil_filtered = in;  // 婢堆嗙儲閸欐娲块幒銉╁櫚缁?
            s_oil_pending = -100;
            s_oil_pending_cnt = 0;
            s_oil_diag.last_filtered_temp = s_oil_filtered;
            ESP_LOGI(TAG, "OIL: Large change ACCEPTED raw=%d filtered=%d (confirmed 3x)", in, s_oil_filtered);
            obd_data_set_oil_temp_with_offset(s_oil_filtered);
        }
    }
}
static void default_on_parsed_load_pct(uint32_t load_pct) { ESP_LOGD(TAG, "LOAD: %u%%", load_pct); obd_data_set_load_pct((int16_t)load_pct); }
static void default_on_parsed_control_module_voltage(uint32_t bat_mv) { ESP_LOGD(TAG, "BAT: %u.%uV", bat_mv/1000, (bat_mv%1000)/100); obd_data_set_bat_mv((int32_t)bat_mv); }
static void default_on_parsed_throttle_position(uint32_t tps_pct) { ESP_LOGD(TAG, "TPS: %u%%", tps_pct); obd_data_set_tps((int16_t)tps_pct); }
// MAP(kPa) 閳?濞懧ょ枂鐞涖劌甯?0.1bar)閿涙俺銆冮崢?= (MAP - 婢堆勭毜閳?00kPa)閿?0kPa = 0.1bar
static void default_on_parsed_manifold_pressure(uint32_t map_kpa) {
    int16_t boost_x10 = (int16_t)(((int32_t)map_kpa - 100) / 10);
    if (boost_x10 < 0) boost_x10 = 0; // 娑撳秵妯夌粈楦跨閸?閻喓鈹?閿涘奔绮?鐠?
    ESP_LOGD(TAG, "MAP: %u kPa -> boost %d.%d bar", map_kpa, boost_x10/10, boost_x10%10);
    obd_data_set_boost_x10(boost_x10);
}
 
static void obd_poll_task(void *arg) {
    static const obd_poll_schedule_entry_t s_poll_seq_na[] = {
        {OBD_POLL_SLOT_RPM, AUX_OBD_DEMAND_RPM},
        {OBD_POLL_SLOT_SPEED, AUX_OBD_DEMAND_SPEED},
        {OBD_POLL_SLOT_TPS, AUX_OBD_DEMAND_TPS},
        {OBD_POLL_SLOT_RPM, AUX_OBD_DEMAND_RPM},
        {OBD_POLL_SLOT_SPEED, AUX_OBD_DEMAND_SPEED},
        {OBD_POLL_SLOT_LOAD, AUX_OBD_DEMAND_LOAD},
        {OBD_POLL_SLOT_TPS, AUX_OBD_DEMAND_TPS},
        {OBD_POLL_SLOT_RPM, AUX_OBD_DEMAND_RPM},
        {OBD_POLL_SLOT_SPEED, AUX_OBD_DEMAND_SPEED},
        {OBD_POLL_SLOT_TPS, AUX_OBD_DEMAND_TPS},
        {OBD_POLL_SLOT_CLT, AUX_OBD_DEMAND_CLT},
        {OBD_POLL_SLOT_OIL, AUX_OBD_DEMAND_OIL},
        {OBD_POLL_SLOT_IAT, AUX_OBD_DEMAND_IAT},
        {OBD_POLL_SLOT_BAT, AUX_OBD_DEMAND_BAT},
        {OBD_POLL_SLOT_RPM, AUX_OBD_DEMAND_RPM},
        {OBD_POLL_SLOT_SPEED, AUX_OBD_DEMAND_SPEED},
    };
    static const obd_poll_schedule_entry_t s_poll_seq_boost[] = {
        {OBD_POLL_SLOT_RPM, AUX_OBD_DEMAND_RPM},
        {OBD_POLL_SLOT_SPEED, AUX_OBD_DEMAND_SPEED},
        {OBD_POLL_SLOT_TPS, AUX_OBD_DEMAND_TPS},
        {OBD_POLL_SLOT_BOOST, AUX_OBD_DEMAND_BOOST},
        {OBD_POLL_SLOT_RPM, AUX_OBD_DEMAND_RPM},
        {OBD_POLL_SLOT_SPEED, AUX_OBD_DEMAND_SPEED},
        {OBD_POLL_SLOT_LOAD, AUX_OBD_DEMAND_LOAD},
        {OBD_POLL_SLOT_TPS, AUX_OBD_DEMAND_TPS},
        {OBD_POLL_SLOT_RPM, AUX_OBD_DEMAND_RPM},
        {OBD_POLL_SLOT_SPEED, AUX_OBD_DEMAND_SPEED},
        {OBD_POLL_SLOT_TPS, AUX_OBD_DEMAND_TPS},
        {OBD_POLL_SLOT_CLT, AUX_OBD_DEMAND_CLT},
        {OBD_POLL_SLOT_OIL, AUX_OBD_DEMAND_OIL},
        {OBD_POLL_SLOT_IAT, AUX_OBD_DEMAND_IAT},
        {OBD_POLL_SLOT_BAT, AUX_OBD_DEMAND_BAT},
        {OBD_POLL_SLOT_RPM, AUX_OBD_DEMAND_RPM},
        {OBD_POLL_SLOT_SPEED, AUX_OBD_DEMAND_SPEED},
    };

    (void)arg;

    for (;;) {
        if (s_connected) {
            vTaskDelay(pdMS_TO_TICKS(2000));
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    uint32_t tick_count = 0;
    obd_stream_mode_t stream_mode = OBD_STREAM_MODE_PID_POLL;
    const nvs_user_cfg_t *cfg = nvs_cfg_get();
    uint8_t protocol_to_use = cfg->protocol;

    if (protocol_to_use == 0) {
        ESP_LOGI(TAG, "Protocol auto-detect enabled (current NVS: 0-auto)");
        int detected_proto = elm327_auto_detect_protocol();

        if (detected_proto > 0) {
            protocol_to_use = (uint8_t)detected_proto;

            nvs_user_cfg_t new_cfg = *cfg;
            new_cfg.protocol = protocol_to_use;
            nvs_cfg_set(&new_cfg);

            ESP_LOGI(TAG, "Protocol auto-detect SUCCESS! Saving protocol %d to NVS", protocol_to_use);
        } else {
            protocol_to_use = 6;
            ESP_LOGW(TAG, "Protocol auto-detect FAILED, using fallback protocol 6");
        }
    }

    const char *fixed_header_cmd = get_vehicle_fixed_header_cmd();
    elm327_send_standard_init_sequence(protocol_to_use, fixed_header_cmd);
    init_oil_temp_strategy();

    const vehicle_profile_t *active_profile = vehicle_profile_get_active();
    ESP_LOGI(TAG, "Active vehicle profile: %s", active_profile ? active_profile->name : "Unknown");

    while (1) {
        uint16_t slot_delay_ms = nvs_cfg_get_obd_poll_slot_delay_ms(cfg);
        const vehicle_profile_t *vp = vehicle_profile_get_active();
        const obd_poll_schedule_entry_t *poll_seq = (vp && vp->has_boost) ? s_poll_seq_boost : s_poll_seq_na;
        uint32_t poll_seq_len = (uint32_t)((vp && vp->has_boost)
                                               ? (sizeof(s_poll_seq_boost) / sizeof(s_poll_seq_boost[0]))
                                               : (sizeof(s_poll_seq_na) / sizeof(s_poll_seq_na[0])));
        size_t poll_index = 0u;
        uint32_t demand_mask = aux_sensor_demand_get_obd_mask();
        bool gforce_monitor_needed = aux_sensor_demand_is_gforce_obd_enabled();
        bool zc6_gear_monitor_needed = aux_sensor_demand_is_zc6_gear_obd_enabled();

        if (!s_connected) {
            stream_mode = OBD_STREAM_MODE_PID_POLL;
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        if (gforce_monitor_needed) {
            if (stream_mode != OBD_STREAM_MODE_GFORCE_MONITOR) {
                elm327_enter_gforce_monitor_mode();
                stream_mode = OBD_STREAM_MODE_GFORCE_MONITOR;
            }
            vTaskDelay(pdMS_TO_TICKS(120));
            continue;
        }

        if (zc6_gear_monitor_needed) {
            if (stream_mode != OBD_STREAM_MODE_ZC6_GEAR_MONITOR) {
                elm327_enter_zc6_gear_monitor_mode();
                stream_mode = OBD_STREAM_MODE_ZC6_GEAR_MONITOR;
            }
            vTaskDelay(pdMS_TO_TICKS(120));
            continue;
        }

        if (stream_mode == OBD_STREAM_MODE_GFORCE_MONITOR) {
            elm327_exit_gforce_monitor_mode(protocol_to_use, fixed_header_cmd);
            stream_mode = OBD_STREAM_MODE_PID_POLL;
        }
        if (stream_mode == OBD_STREAM_MODE_ZC6_GEAR_MONITOR) {
            elm327_exit_zc6_gear_monitor_mode(protocol_to_use, fixed_header_cmd);
            stream_mode = OBD_STREAM_MODE_PID_POLL;
        }

        if (!obd_poll_schedule_find_next_active_index(poll_seq,
                                                      poll_seq_len,
                                                      tick_count,
                                                      demand_mask,
                                                      &poll_index)) {
            tick_count = 0u;
            vTaskDelay(pdMS_TO_TICKS(OBD_IDLE_NO_DEMAND_DELAY_MS));
            continue;
        }
        tick_count = (uint32_t)poll_index;
        obd_poll_slot_t poll_slot = (obd_poll_slot_t)poll_seq[poll_index].slot_id;

        switch (poll_slot) {
        case OBD_POLL_SLOT_RPM:
            elm327_ble_send_ascii_blocking("01 0C\r");
            ESP_LOGD(TAG, "Send 01 0C");
            break;
        case OBD_POLL_SLOT_IAT:
            elm327_ble_send_ascii_blocking("01 0F\r");
            ESP_LOGD(TAG, "Send 01 0F");
            break;
        case OBD_POLL_SLOT_SPEED:
            elm327_ble_send_ascii_blocking("01 0D\r");
            ESP_LOGD(TAG, "Send 01 0D");
            break;
        case OBD_POLL_SLOT_CLT:
            elm327_ble_send_ascii_blocking("01 05\r");
            ESP_LOGD(TAG, "Send 01 05");
            break;
        case OBD_POLL_SLOT_LOAD:
            elm327_ble_send_ascii_blocking("01 04\r");
            ESP_LOGD(TAG, "[Slot4] Send 01 04 (engine load)");
            break;
        case OBD_POLL_SLOT_TPS:
            elm327_ble_send_ascii_blocking("01 11\r");
            ESP_LOGD(TAG, "[Slot5] Send 01 11 (TPS)");
            break;
        case OBD_POLL_SLOT_OIL: {
            uint8_t poll_idx = 0;
            oil_temp_query_mode_t mode = get_next_oil_query_mode(&poll_idx);

            if (mode == OIL_TEMP_MODE_PID_5C) {
                elm327_ble_send_ascii_blocking("01 5C\r");
                s_expect_mode21 = false;
                ESP_LOGI(TAG, "[Slot6] Send 01 5C (Standard oil temp PID)");
            } else if (mode == OIL_TEMP_MODE_UDS_22_10_17) {
                elm327_ble_send_ascii_blocking("22 10 17\r");
                s_expect_mode21 = false;
                ESP_LOGI(TAG, "[Slot6] Send 22 10 17 (UDS oil temp)");
            } else if (mode == OIL_TEMP_MODE_TOYOTA_21_01) {
                elm327_ble_send_ascii_blocking("21 01\r");
                s_expect_mode21 = true;
                ESP_LOGI(TAG, "[Slot6] Send 21 01 (Toyota oil temp)");
            } else if (mode == OIL_TEMP_MODE_MAZDA_22_111F) {
                elm327_ble_send_ascii_blocking("22 11 1F\r");
                s_expect_mode21 = false;
                ESP_LOGI(TAG, "[Slot6] Send 22 11 1F (Mazda oil temp)");
            } else if (mode == OIL_TEMP_MODE_MAZDA_22_1310) {
                elm327_ble_send_ascii_blocking("22 13 10\r");
                s_expect_mode21 = false;
                ESP_LOGI(TAG, "[Slot6] Send 22 13 10 (Mazda oil temp)");
            } else {
                ESP_LOGW(TAG, "[Slot6] No valid oil temp mode available");
                s_expect_mode21 = false;
            }
            s_oil_query_mode = poll_idx;
            break;
        }
        case OBD_POLL_SLOT_BAT:
            elm327_ble_send_ascii_blocking("01 42\r");
            ESP_LOGD(TAG, "[Slot7] Send 01 42 (bat voltage)");
            break;
        case OBD_POLL_SLOT_BOOST: {
            const vehicle_profile_t *vp_for_boost = vehicle_profile_get_active();
            if (vp_for_boost && vp_for_boost->has_boost) {
                elm327_ble_send_ascii_blocking("01 0B\r");
                ESP_LOGD(TAG, "[Slot8] Send 01 0B (boost/MAP)");
            }
            break;
        }
        default:
            break;
        }

        tick_count++;
        if (tick_count >= poll_seq_len) {
            tick_count = 0;
        }

        vTaskDelay(pdMS_TO_TICKS(slot_delay_ms));
    }
}
// Mode 21 婢舵艾鎶氱憴锝嗙€介崳顭掔窗娴?"61 01" 娑斿鎮楅幓鎰絿閹碘偓閺堝鏆熼幑顔肩摟閼?
// 鐠哄疇绻?ELM327 鐞涘苯褰块崜宥囩磻 ("N: ") 閸?ISO-TP 鏉╃偟鐢荤敮褍绨崚妤€鐡ч懞?(0x20~0x2F)
// 鏉╂柨娲栭幓鎰絿閸掓壆娈戠€涙濡弫甯礉缂佹挻鐏夌€涙ê鍙?out[]
static int parse_mode21_data(const char *buf, uint32_t *out, int max_out) {
    const char *p = strstr(buf, "61 01");
    if (!p) return 0;
    p += 5; // 鐠哄疇绻?"61 01"
    if (*p == ' ') p++;

    int count = 0;
    bool new_line = false;

    while (*p && count < max_out) {
        if (*p == '>') break;
        if (*p == '\r' || *p == '\n') {
            new_line = true;
            p++;
            continue;
        }
        if (new_line) {
            // 鐠哄疇绻?"N: " 閸撳秶绱戦敍鍫滅娑擃亝鍨ㄦ径姘嚋閺佹澘鐡?+ 閸愭帒褰?+ 缁岀儤鐗搁敍?
            while (isdigit((unsigned char)*p)) p++;
            if (*p == ':') p++;
            while (*p == ' ') p++;
            // 鐠哄疇绻?ISO-TP 鏉╃偟鐢荤敮褍绨崚妤€鐡ч懞?(0x20~0x2F)
            if (isxdigit((unsigned char)*p) && isxdigit((unsigned char)*(p+1))) {
                char tmp[3] = {*p, *(p+1), '\0'};
                unsigned bval = (unsigned)strtoul(tmp, NULL, 16);
                if (bval >= 0x20 && bval <= 0x2F) {
                    p += 2;
                    if (*p == ' ') p++;
                }
            }
            new_line = false;
            continue;
        }
        // 鐟欙絾鐎芥稉鈧稉顏勫磩閸忣叀绻橀崚璺虹摟閼哄倸顕?
        if (isxdigit((unsigned char)*p) && isxdigit((unsigned char)*(p+1))) {
            char tmp[3] = {*p, *(p+1), '\0'};
            out[count++] = (uint32_t)strtoul(tmp, NULL, 16);
            p += 2;
        } else {
            p++;
        }
        if (*p == ' ') p++;
    }
    return count;
}

// 娴?Mode21 閺佺増宓佹稉顓熷絹閸欐牗婧€濞岃淇€涙濡敍鍫滀簰 BRZ ZC6 娑撳搫寮懓鍐跨礆
// BRZ ZC6 闁艾鐖堕崷銊ユ祼鐎规矮缍呯純顕嗙礉娴ｅ棙鏁幐浣藉殰闁倸绨查幖婊呭偍
static bool extract_mode21_oil_temp(const uint32_t *d, int count, int32_t *oil_c) {
    if (!d || count <= 0 || !oil_c) return false;

    int16_t coolant = obd_data_get_coolant_temp();
    
    ESP_LOGI(TAG, "Mode21 extract: total_count=%d, coolant=%d", count, coolant);

    // ZC6 閸ュ搫鐣鹃崷鏉挎絻 + 閸ュ搫鐣剧槐銏犵穿娴兼ê鍘涢敍宀勪缉閸忓秷鍤滈柅鍌氱安鐠囶垶鈧鈧?
    if (get_active_vehicle_idx_safe() == 0 && count > 33) {
        int32_t zc6_fixed = (int32_t)d[33] - 40;
        if (zc6_fixed >= -10 && zc6_fixed <= 150) {
            s_mode21_oil_idx = 33;
            s_oil_diag.mode2_ok++;
            *oil_c = zc6_fixed;
            ESP_LOGI(TAG, "Mode21 ZC6 fixed idx=33 -> %dC", (int)zc6_fixed);
            return true;
        }
        ESP_LOGW(TAG, "Mode21 ZC6 fixed idx invalid: raw=%u", (unsigned)d[33]);
    }

    // ---- 缁涙牜鏆?1: 娴ｈ法鏁ゆ稉濠冾偧閹垫儳鍩岄惃鍕偍瀵洩绱欒箛顐︹偓鐔荤熅瀵板嫸绱?---
    if (s_mode21_oil_idx >= 0 && s_mode21_oil_idx < count) {
        int32_t c = (int32_t)d[s_mode21_oil_idx] - 40;
        if (c >= -10 && c <= 150) {  // 鐎硅姤婢楅懠鍐ㄦ纯妤犲矁鐦?
            *oil_c = c;
            ESP_LOGD(TAG, "Mode21: Using cached idx=%d -> %dC", s_mode21_oil_idx, (int)c);
            s_oil_diag.mode2_ok++;
            return true;
        }
    }

    // ---- 缁涙牜鏆?2: 閺呴缚鍏橀幖婊呭偍 ----
    // 闁插洨鏁ゆ稉銈夋▉濞堝灚鎮崇槐顫窗閸忓牅寮楅弽鍏碱梾閺屻儰绗屽瀛樹刊閻ㄥ嫬妯婂鍌︾礄鍗?5鎺矯閿涘绱濋崘宥嗗⒖婢堆嗗瘱閸?
    int best_idx = -1;
    int32_t best_temp = 0;
    int best_distance = 32767;
    int strict_count = 0;

    for (int idx = 0; idx < count; idx++) {
        int32_t c = (int32_t)d[idx] - 40;

        // 閸╄櫣顢呴懠鍐ㄦ纯濡偓閺屻儻绱?10 閸?150鎺矯閿涘牆顔斿▔娑崇礆
        if (c < -10 || c > 150) continue;

        // 娑撱儲鐗搁崠褰掑帳閿涙碍琛ュ〒鈺佺安濮ｆ梹鎸夊〒鈺冣棦妤傛ɑ鍨ㄩ幒銉ㄧ箮閿涘牓鈧艾鐖?0~15鎺矯瀹割喖绱撻敍?
        if (coolant > -40) {
            int diff = (int)c - (int)coolant;
            
            // 缁楊兛绔撮梼鑸殿唽閿涙矮寮楅弽?鍗?5鎺矯
            if (diff >= -25 && diff <= 25) {
                // 娴兼ê鍘涢柅澶嬪閿涙碍琛ュ〒鈺冩殣妤傛ü绨瀛樹刊閿涘牆浜搁悜顓㈠櫤閹圭喎銇戦惃鍕⒖閻炲棛骞囩挒鈽呯礆
                int priority = (diff >= 0 && diff <= 15) ? 1000 : 
                               (diff > 15 && diff <= 25) ? 500 : 100;
                int score = priority - abs(diff);  // 瀹割喖绱撶搾濠傜毈閸掑棜绉烘?
                
                if (score > best_distance) {
                    best_distance = score;
                    best_idx = idx;
                    best_temp = c;
                    strict_count++;
                    ESP_LOGI(TAG, "  Strict match: idx=%d temp=%dC diff=%d score=%d", idx, (int)c, diff, score);
                }
            }
        } else {
            // 濮樺瓨淇弮鐘虫櫏閺冭绱濋崣鏍︽崲娴ｆ洘婀侀弫鍫熶刊鎼达讣绱欐担鍡氼唶瑜版洝顒熼崨濠忕礆
            if (best_idx < 0) {
                best_idx = idx;
                best_temp = c;
                ESP_LOGW(TAG, "  Fallback (no coolant): idx=%d temp=%dC", idx, (int)c);
            }
        }
    }

    if (best_idx >= 0) {
        ESP_LOGI(TAG, "Mode21 selected: idx=%d temp=%dC (strict_matches=%d)", best_idx, (int)best_temp, strict_count);
        s_mode21_oil_idx = best_idx;
        s_last_mode21_oil = (int16_t)best_temp;
        s_oil_diag.mode2_ok++;
        *oil_c = best_temp;
        return true;
    }
    
    ESP_LOGW(TAG, "Mode21: No valid candidate found (coolant=%d)", coolant);
    s_oil_diag.mode2_fail++;
    return false;
}

static void start_scan(void) {
    esp_ble_gap_start_scanning(10); // 10s
}

static bool parse_mac_string(const char *s, esp_bd_addr_t out_bda) {
    if (!s) return false;
    unsigned int b[6] = {0};
    int n = sscanf(s, "%2x:%2x:%2x:%2x:%2x:%2x", &b[0], &b[1], &b[2], &b[3], &b[4], &b[5]);
    if (n != 6) return false;
    for (int i = 0; i < 6; i++) out_bda[i] = (uint8_t)b[i];
    return true;
}

static void extract_adv_name(const uint8_t *adv_data, uint8_t adv_data_len, uint8_t scan_rsp_len,
                             char *out_name, size_t out_name_len) {
    if (!out_name || out_name_len == 0) return;
    out_name[0] = '\0';
    if (!adv_data) return;

    uint8_t total_len = adv_data_len + scan_rsp_len;
    uint8_t idx = 0;
    while (idx + 1 < total_len) {
        uint8_t field_len = adv_data[idx];
        if (field_len == 0) break;
        if (idx + 1 + field_len > total_len) break;

        uint8_t ad_type = adv_data[idx + 1];
        uint8_t payload_len = field_len - 1;
        const uint8_t *payload = &adv_data[idx + 2];

        if ((ad_type == ESP_BLE_AD_TYPE_NAME_CMPL || ad_type == ESP_BLE_AD_TYPE_NAME_SHORT) && payload_len > 0) {
            size_t copy_len = payload_len < (out_name_len - 1) ? payload_len : (out_name_len - 1);
            memcpy(out_name, payload, copy_len);
            out_name[copy_len] = '\0';
            return;
        }
        idx += (uint8_t)(field_len + 1);
    }
}

static void normalize_name(const char *src, char *dst, size_t dst_len) {
    if (!dst || dst_len == 0) return;
    if (!src) {
        dst[0] = '\0';
        return;
    }

    size_t out = 0;
    for (size_t i = 0; src[i] != '\0' && out + 1 < dst_len; i++) {
        char c = src[i];
        if (c == ' ' || c == '-' || c == '_' || c == '\t') continue;
        dst[out++] = (char)tolower((unsigned char)c);
    }
    dst[out] = '\0';
}

static bool match_device_target(const esp_ble_gap_cb_param_t *pr, const char *target_name,
                                char *found_name, size_t found_name_len) {
    if (!pr) return false;
    extract_adv_name(pr->scan_rst.ble_adv, pr->scan_rst.adv_data_len, pr->scan_rst.scan_rsp_len,
                     found_name, found_name_len);

    if (target_name == NULL || target_name[0] == '\0') return true;

    esp_bd_addr_t target_bda = {0};
    if (parse_mac_string(target_name, target_bda)) {
        return memcmp(pr->scan_rst.bda, target_bda, sizeof(esp_bd_addr_t)) == 0;
    }

    if (found_name[0] == '\0') return false;

    char found_norm[40] = {0};
    char target_norm[40] = {0};
    normalize_name(found_name, found_norm, sizeof(found_norm));
    normalize_name(target_name, target_norm, sizeof(target_norm));
    if (found_norm[0] == '\0' || target_norm[0] == '\0') return false;

    // 閸忕厧顔愰惌顓炴倳/閹搭亝鏌囬崥? 閸忋劎鐡戦妴浣稿瘶閸氼偁鈧浇顫﹂崠鍛儓閸у洩顫嬫稉鍝勬嚒娑?
    return (strcmp(found_norm, target_norm) == 0) ||
           (strstr(found_norm, target_norm) != NULL) ||
           (strstr(target_norm, found_norm) != NULL);
}

static void request_discovery(void) {
    // NULL = 閸欐垹骞囬幍鈧張澶嬫箛閸斺槄绱濋崗鐓庮啇娑撳秴鎮揢UID閻ㄥ嚐LM327闁倿鍘ら崳?
    esp_ble_gattc_search_service(s_gattc_if, s_conn_id, NULL);
}

static void enable_notify_if_ready(void) {
    if (s_cccd_handle) {
        uint8_t notify_en[2] = {0x01, 0x00};
        esp_ble_gattc_write_char_descr(s_gattc_if, s_conn_id, s_cccd_handle,
                                       sizeof(notify_en), notify_en,
                                       ESP_GATT_WRITE_TYPE_RSP, ESP_GATT_AUTH_REQ_NONE);
    }
}

static void gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);
static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);

void elm327_ble_init_and_start(const char *target_name, const elm327_ble_callbacks_t *cbs) {
    if (cbs) s_cbs = *cbs;
    if (target_name && target_name[0]) {
        strncpy(s_target_name, target_name, sizeof(s_target_name)-1);
        s_target_name[sizeof(s_target_name)-1] = '\0';
    }

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
    if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_IDLE) {
        ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    }
    if (esp_bt_controller_get_status() != ESP_BT_CONTROLLER_STATUS_ENABLED) {
        ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));
    }
    if (!esp_bluedroid_get_status()) {
        ESP_ERROR_CHECK(esp_bluedroid_init());
        ESP_ERROR_CHECK(esp_bluedroid_enable());
    } else if (esp_bluedroid_get_status() != ESP_BLUEDROID_STATUS_ENABLED) {
        ESP_ERROR_CHECK(esp_bluedroid_enable());
    }

    ESP_ERROR_CHECK(esp_ble_gap_register_callback(gap_event_handler));
    ESP_ERROR_CHECK(esp_ble_gattc_register_callback(gattc_event_handler));
    ESP_ERROR_CHECK(esp_ble_gattc_app_register(0));
    s_ble_inited = true;
}

bool elm327_ble_send_command(const uint8_t *data, size_t len) {
    if (!s_connected || s_char_write_handle == 0) {
        s_elm_ready = true; // 閺冪姵纭堕崣鎴︹偓浣规閹垹顦查弽鍥х箶閿涘矂妲诲顫閻╃绉撮弮?
        return false;
    }
    if (len == 0 || data == NULL) { s_elm_ready = true; return false; }
    esp_err_t err = esp_ble_gattc_write_char(s_gattc_if, s_conn_id, s_char_write_handle,
                                             len, (uint8_t *)data,
                                             s_write_type, ESP_GATT_AUTH_REQ_NONE);
    if (err != ESP_OK) s_elm_ready = true; // 閸欐垿鈧礁銇戠拹銉ょ瘍鐟曚焦浠径?
    return err == ESP_OK;
}

// 闂冭顢ｉ惄鏉戝煂娑撳﹣绔存稉顏勬惙鎼存梻绮ㄩ弶鐕傜礄閺€璺哄煂 '>'閿涘鎮楅崘宥呭絺闁?
bool elm327_ble_send_ascii_blocking(const char *ascii_cmd)
{
    uint32_t waited_ms = 0;
    while (!s_elm_ready && waited_ms < 3000) {
        vTaskDelay(pdMS_TO_TICKS(10));
        waited_ms += 10;
    }
    if (!s_elm_ready) {
        ESP_LOGW(TAG, "Timeout (>3s) waiting previous response, forcing send: %s", ascii_cmd);
        s_elm_ready = true; // 闁灝鍘ゅ濠氭敚閿涘瞼鎴风紒顓炲絺闁?
    }
    s_elm_ready = false;
    uint8_t buf[32];
    size_t n = elm327_ble_ascii_cmd_to_bytes(ascii_cmd, buf, sizeof(buf));
    if (n) return elm327_ble_send_command(buf, n);
    else {
        s_elm_ready = true;
        return false;
    }
}

// 鐏?ASCII 閹稿洣鎶?婵?"01 0C\r")婢跺秴鍩楅崚鎷岀翻閸戣櫣绱﹂崘鎻掑隘閿涘苯鎮撻弮璺哄箵闂勩倗鈹栭惂钘夌摟缁楋讣绱濇穱婵囧瘮 ELM327 閹碘偓闂団偓閻?ASCII 閺嶇厧绱?
size_t elm327_ble_ascii_cmd_to_bytes(const char *ascii, uint8_t *out_buf, size_t out_buf_len) {
    size_t out = 0;
    const char *p = ascii;
    while (*p && out < out_buf_len) {
        if (*p == ' ' || *p == '\t') {
            p++;                // 鐠哄疇绻冪粚铏规缁?
            continue;
        }
        out_buf[out++] = (uint8_t)(*p++); // 閻╁瓨甯存径宥呭煑 ASCII 鐎涙濡?
    }
    return out;
}

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    racechrono_ble_diy_handle_gap_event(event, param);

    switch (event) {
    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: {
        start_scan();
        break;
    }
    case ESP_GAP_BLE_SCAN_RESULT_EVT: {
        esp_ble_gap_cb_param_t *pr = param;
        if (pr->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_RES_EVT) {
            char dev_name[32] = {0};
            extract_adv_name(pr->scan_rst.ble_adv, pr->scan_rst.adv_data_len,
                             pr->scan_rst.scan_rsp_len, dev_name, sizeof(dev_name));

            if (s_scan_only_mode) {
                // 閹殿偅寮垮Ο鈥崇础閿涙碍鏁归梿鍡氼啎婢跺洤鍨悰?
                if (dev_name[0] != '\0' && s_scan_count < BLE_SCAN_MAX_DEVICES && s_scan_list != NULL) {
                    // 濡偓閺屻儲妲搁崥锕€鍑＄€涙ê婀?
                    bool exists = false;
                    for (int i = 0; i < s_scan_count; i++) {
                        if (strcmp(s_scan_list[i].name, dev_name) == 0) {
                            exists = true;
                            break;
                        }
                    }
                    if (!exists) {
                        snprintf(s_scan_list[s_scan_count].name,
                                 sizeof(s_scan_list[s_scan_count].name),
                                 "%s",
                                 dev_name);
                        memcpy(s_scan_list[s_scan_count].addr, pr->scan_rst.bda, 6);
                        s_scan_list[s_scan_count].rssi = pr->scan_rst.rssi;
                        s_scan_count++;
                        ESP_LOGI(TAG, "Scan found [%d]: %s (RSSI %d)", s_scan_count, dev_name, pr->scan_rst.rssi);
                        if (s_scan_cb) s_scan_cb(&s_scan_list[s_scan_count - 1], s_scan_count);
                    }
                }
            } else {
                // 濮濓絽鐖跺Ο鈥崇础閿涙艾灏柊宥呮倳缁夋澘鎮楁潻鐐村复
                bool matched = match_device_target(pr, s_target_name, dev_name, sizeof(dev_name));
                ESP_LOGD(TAG, "Scan: name=%s rssi=%d target=%s match=%d",
                         dev_name[0] ? dev_name : "<no-name>", pr->scan_rst.rssi,
                         s_target_name, matched);
                if (matched) {
                    ESP_LOGI(TAG, "Found target %s (dev=%s), connecting...",
                             s_target_name, dev_name[0] ? dev_name : "<no-name>");
                    esp_ble_gap_stop_scanning();
                    esp_ble_gattc_open(s_gattc_if, pr->scan_rst.bda, pr->scan_rst.ble_addr_type, true);
                }
            }
        }
        break;
    }
    case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
    default:
        break;
    }
}

static void gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param) {
    switch (event) {
    case ESP_GATTC_REG_EVT: {
        s_gattc_if = gattc_if;
        esp_ble_scan_params_t scan_params = {
            .scan_type              = BLE_SCAN_TYPE_ACTIVE,
            .own_addr_type          = BLE_ADDR_TYPE_PUBLIC,
            .scan_filter_policy     = BLE_SCAN_FILTER_ALLOW_ALL,
            .scan_interval          = 0x60,
            .scan_window            = 0x30,
            .scan_duplicate         = BLE_SCAN_DUPLICATE_DISABLE
        };
        esp_ble_gap_set_scan_params(&scan_params);
        break;
    }
    case ESP_GATTC_CONNECT_EVT: {
        s_connected = true;
        s_conn_id = param->connect.conn_id;
        memcpy(s_peer_bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
        if (s_cbs.on_connected) s_cbs.on_connected();
        request_discovery();
        break;
    }
    case ESP_GATTC_OPEN_EVT: {
        if (param->open.status != ESP_GATT_OK) {
            ESP_LOGE(TAG, "Open failed status=%d", param->open.status);
            nvs_error_log_record(TAG, ESP_FAIL, "BLE open failed");
            start_scan();
        }
        break;
    }
    case ESP_GATTC_SEARCH_RES_EVT: {
        const esp_gatt_id_t *srvc_id = &param->search_res.srvc_id;
        uint16_t sh = param->search_res.start_handle;
        uint16_t eh = param->search_res.end_handle;
        if (srvc_id->uuid.len == ESP_UUID_LEN_16) {
            ESP_LOGI(TAG, "Service found: UUID=0x%04X handle=%04X~%04X",
                     srvc_id->uuid.uuid.uuid16, sh, eh);
            if (srvc_id->uuid.uuid.uuid16 == UUID16_OBD_SERVICE) {
                s_have_service = true;
                s_service_start = sh;
                s_service_end = eh;
                ESP_LOGI(TAG, "Target service FFF0 matched");
            } else if (srvc_id->uuid.uuid.uuid16 == UUID16_OBD_SERVICE_18F0) {
                s_have_18f0 = true;
                s_18f0_start = sh;
                s_18f0_end = eh;
                ESP_LOGI(TAG, "Target service 18F0 matched (IOS-Vlink OBD)");
            } else if (srvc_id->uuid.uuid.uuid16 == UUID16_OBD_SERVICE_FF12) {
                s_have_ff12 = true;
                s_ff12_start = sh;
                s_ff12_end = eh;
                ESP_LOGI(TAG, "Target service FF12 matched");
            }
        } else {
            ESP_LOGI(TAG, "Service found: UUID(long) handle=%04X~%04X", sh, eh);
        }
        // 鐠佹澘缍嶉張鈧径顪產ndle閼煎啫娲块敍宀€鏁ゆ禍搴″幑鎼存洖鍙忛柌蹇旂叀閹?
        if (eh > s_all_attr_end || s_all_attr_end == 0xFFFF) s_all_attr_end = eh;
        break;
    }
    case ESP_GATTC_SEARCH_CMPL_EVT: {
        ESP_LOGI(TAG, "Service discovery complete. have_FFF0=%d have_18F0=%d have_FF12=%d",
                 s_have_service, s_have_18f0, s_have_ff12);

        // 娴兼ê鍘涙い鍝勭碍: 0xFFF0 > 0x18F0(IOS-Vlink) > 0xFF12 > 閸忋劏瀵栭崶鏉戝幑鎼?
        if (!s_have_service) {
            if (s_have_18f0) {
                s_service_start = s_18f0_start;
                s_service_end   = s_18f0_end;
                ESP_LOGI(TAG, "Using 18F0 service range 0x%04X~0x%04X", s_service_start, s_service_end);
            } else if (s_have_ff12) {
                s_service_start = s_ff12_start;
                s_service_end   = s_ff12_end;
                ESP_LOGI(TAG, "Using FF12 service range 0x%04X~0x%04X", s_service_start, s_service_end);
            } else {
                s_service_start = 0x0001;
                s_service_end = (s_all_attr_end > 0x0001) ? s_all_attr_end : 0xFFFF;
                ESP_LOGW(TAG, "FFF0/18F0/FF12 not found, using full range 0x0001~0x%04X", s_service_end);
            }
        }

        // 閺嬫矮濡囬崗銊╁劥閻楃懓绶涢敍灞惧瘻鐏炵€塼tr閿涘湹RITE/NOTIFY閿涘鈧褰?
        uint16_t char_count = 0;
        esp_err_t ret = esp_ble_gattc_get_attr_count(gattc_if, s_conn_id,
            ESP_GATT_DB_CHARACTERISTIC, s_service_start, s_service_end, 0, &char_count);
        ESP_LOGI(TAG, "get_attr_count ret=%d, char_count=%d", ret, char_count);

        if (ret != ESP_OK || char_count == 0) {
            ESP_LOGE(TAG, "No characteristics found in range! Cannot communicate.");
            nvs_error_log_record(TAG, ESP_ERR_NOT_FOUND, "No GATT characteristics");
            break;
        }

        // 閸掑棝鍘ら悧鐟扮窙閺佹壆绮?
        uint16_t alloc_count = char_count;
        esp_gattc_char_elem_t *chars = (esp_gattc_char_elem_t *)malloc(alloc_count * sizeof(esp_gattc_char_elem_t));
        if (!chars) {
            ESP_LOGE(TAG, "malloc failed");
            nvs_error_log_record(TAG, ESP_ERR_NO_MEM, "Char alloc failed");
            break;
        }

        ret = esp_ble_gattc_get_all_char(gattc_if, s_conn_id,
            s_service_start, s_service_end, chars, &alloc_count, 0);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "get_all_char failed: %d", ret);
            nvs_error_log_record(TAG, ESP_FAIL, "get_all_char failed");
            free(chars); break;
        }

        // 閹垫挸宓冮崗銊╁劥閻楃懓绶涢敍灞借嫙閼奉亜濮╅柅澶婂絿閸愭瑥鍙?闁氨鐓￠崣銉︾労
        ESP_LOGI(TAG, "=== All characteristics (%d) ===", alloc_count);
        for (int i = 0; i < alloc_count; i++) {
            esp_gattc_char_elem_t *c = &chars[i];
            if (c->uuid.len == ESP_UUID_LEN_16) {
                ESP_LOGI(TAG, "  [%d] UUID=0x%04X handle=0x%04X prop=0x%02X",
                         i, c->uuid.uuid.uuid16, c->char_handle, c->properties);
            } else if (c->uuid.len == ESP_UUID_LEN_128) {
                ESP_LOGI(TAG, "  [%d] UUID128=%02X%02X...%02X%02X handle=0x%04X prop=0x%02X",
                         i, c->uuid.uuid.uuid128[15], c->uuid.uuid.uuid128[14],
                            c->uuid.uuid.uuid128[1],  c->uuid.uuid.uuid128[0],
                            c->char_handle, c->properties);
            }
            // 闁褰囩粭顑跨娑擃亜鍙块張濉汻ITE鐏炵偞鈧呮畱閻楃懓绶涙担婊€璐熼崘娆忓綖閺?
            if (s_char_write_handle == 0 &&
                (c->properties & (ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_WRITE_NR))) {
                s_char_write_handle = c->char_handle;
                // 娴兼ê鍘涢悽?WRITE_NR閿涘牊妫ら崫宥呯安閸愭瑱绱氶敍宀勪缉閸?status=3 (WRITE_NOT_PERMIT)
                s_write_type = (c->properties & ESP_GATT_CHAR_PROP_BIT_WRITE_NR)
                               ? ESP_GATT_WRITE_TYPE_NO_RSP
                               : ESP_GATT_WRITE_TYPE_RSP;
                ESP_LOGI(TAG, "  >> Selected as WRITE handle: 0x%04X (write_type=%s)",
                         s_char_write_handle,
                         s_write_type == ESP_GATT_WRITE_TYPE_NO_RSP ? "NO_RSP" : "RSP");
            }
            // 闁褰囩粭顑跨娑擃亜鍙块張濉廜TIFY鐏炵偞鈧呮畱閻楃懓绶涙担婊€璐熼柅姘辩叀閸欍儲鐒?
            if (s_char_notify_handle == 0 &&
                (c->properties & ESP_GATT_CHAR_PROP_BIT_NOTIFY)) {
                s_char_notify_handle = c->char_handle;
                ESP_LOGI(TAG, "  >> Selected as NOTIFY handle: 0x%04X", s_char_notify_handle);
            }
        }
        free(chars);

        if (s_char_write_handle == 0) {
            ESP_LOGE(TAG, "No WRITE characteristic found! Cannot send commands.");
            nvs_error_log_record(TAG, ESP_ERR_NOT_FOUND, "No write characteristic");
            break;
        }
        // 婵″倹鐏夊▽鈩冩箒閻欘剛鐝涢惃鍑琌TIFY閻楃懓绶涢敍灞筋槻閻劌鍟撻崣銉︾労
        if (s_char_notify_handle == 0) {
            s_char_notify_handle = s_char_write_handle;
            ESP_LOGI(TAG, "No NOTIFY char found, using WRITE handle 0x%04X for notify", s_char_notify_handle);
        }

        // 濞夈劌鍞介柅姘辩叀
        int sret = esp_ble_gattc_register_for_notify(gattc_if, s_peer_bda, s_char_notify_handle);
        ESP_LOGI(TAG, "register_for_notify handle=0x%04X ret=%d", s_char_notify_handle, sret);

        // 閺屻儲澹?CCCD
        esp_gattc_descr_elem_t descr_elems[2];
        uint16_t count = 2;
        esp_bt_uuid_t cccd_uuid = { .len = ESP_UUID_LEN_16, .uuid = { .uuid16 = UUID16_CCCD } };
        ret = esp_ble_gattc_get_descr_by_char_handle(gattc_if, s_conn_id,
            s_char_notify_handle, cccd_uuid, descr_elems, &count);
        if (ret == ESP_OK && count > 0) {
            s_cccd_handle = descr_elems[0].handle;
            ESP_LOGI(TAG, "Found CCCD, handle: 0x%04X", s_cccd_handle);
        } else {
            ESP_LOGW(TAG, "CCCD not found (ret=%d cnt=%d)", ret, count);
        }
        enable_notify_if_ready();
        break;
    }
    case ESP_GATTC_WRITE_DESCR_EVT: {
        if (param->write.status == ESP_GATT_OK) {
            ESP_LOGI(TAG, "Notifications enabled");
        } else {
            ESP_LOGW(TAG, "Enable notify failed status=%d", param->write.status);
        }
        break;
    }
    case ESP_GATTC_NOTIFY_EVT: {
        if (s_cbs.on_raw_notify) s_cbs.on_raw_notify(param->notify.value, param->notify.value_len);
        const uint8_t *v = param->notify.value;
        int n = param->notify.value_len;
        if (s_gforce_monitor_active) {
            elm327_feed_gforce_monitor_bytes(v, (size_t)n);
            break;
        }
        if (s_zc6_gear_monitor_active) {
            elm327_feed_zc6_gear_monitor_bytes(v, (size_t)n);
            break;
        }

        // ---- 缁鳖垳袧婢舵艾瀵橀弫鐗堝祦閻╂潙鍩岄弨璺哄煂 '>' 閿涘湕LM327 閹绘劗銇氱粭锔肩礆 ----
        // 缁鳖垳袧鐡掑懏妞傛穱婵囧Б閿?缁夋帒鍞撮張顏呮暪閸?'>' 閸掓瑥宸遍崚璺哄煕閺?
        if (s_accum_len > 0) {
            int64_t now_us = esp_timer_get_time();
            if ((now_us - s_accum_start_us) > 5000000) {
                ESP_LOGW(TAG, "Accum timeout (>5s), flushing %d bytes", (int)s_accum_len);
                s_accum_len = 0;
                s_accum_buf[0] = '\0';
                s_elm_ready = true;
            }
        }
        if (s_accum_len == 0) {
            s_accum_start_us = esp_timer_get_time();
        }
        size_t space_left = ACCUM_BUF_SIZE - 1 - s_accum_len;
        size_t copy_n = ((size_t)n < space_left) ? (size_t)n : space_left;
        memcpy(s_accum_buf + s_accum_len, v, copy_n);
        s_accum_len += copy_n;
        s_accum_buf[s_accum_len] = '\0';

        // 濞屸剝鏁归崚?'>' 鐏忚京鎴风紒顓犵搼
        if (memchr(s_accum_buf, '>', s_accum_len) == NULL) break;

        bool expect_mode21 = s_expect_mode21;
        s_expect_mode21 = false;

        // 閺€璺哄煂鐎瑰本鏆ｉ崫宥呯安閿涘苯绱戞慨瀣掗弸?
        char *buf = s_accum_buf;
        ESP_LOGI(TAG, "FULL[%d]: %.100s", (int)s_accum_len, buf); // 鐠囧﹥鏌? 閹垫挸宓冨В蹇旀蒋鐎瑰本鏆ｉ崫宥呯安

        // ELM327 閸欘垵鍏橀崷銊︽殶閹诡喖澧犻梽鍕敨 echo閿涘瞼鏁?strstr 閸忋劌鍞撮柈銊︽偝缁便垹鎼锋惔鏂裤仈
        // 濞夈劍鍓? p61 韫囧懘銆忛崗鍫滅艾 p41 閸掋倖鏌囬敍灞芥礈娑?2101 閻ㄥ嫬顦跨敮褍鎼锋惔鏂剧秼娑擃厼褰查懗钘夊瘶閸?0x41 鐎涙濡?
        // 鐎佃壈鍤?"41 " 鐞氼偊鏁婄拠顖氬爱闁板秷鈧矁鐑︽潻?Mode21 鐟欙絾鐎?
        char *p61 = strstr(buf, "61 01"); // Mode 21 閸濆秴绨叉径?(缁墽鈥橀崠褰掑帳 "61 01")
        char *p41 = strstr(buf, "41 ");
        char *p62 = strstr(buf, "62 ");

        if (p61 != NULL && expect_mode21) {
            // Mode 21 婢舵艾鎶氶崫宥呯安 (Toyota 2101)
            // 閸欘亝婀佺涵顔款吇閸欐垵鍤禍?21 01 閸涙垝鎶ら幍宥埿掗弸鎰剁礉闂冨弶顒涢崗鏈电铂閸濆秴绨查惃鍕殶閹诡喖鐡ч懞鍌滎潾瀹秆冨瘶閸?"61 01"
            uint32_t d[64] = {0};
            int count = parse_mode21_data(buf, d, 64);
            int32_t oil_c = 0;
            if (extract_mode21_oil_temp(d, count, &oil_c)) {
                ESP_LOGI(TAG, "Mode21 oil temp=%dC (idx=%d, bytes=%d)", 
                         (int)oil_c, s_mode21_oil_idx, count);
                record_oil_temp_success(OIL_TEMP_MODE_TOYOTA_21_01);
                // 闁俺绻冮崶鐐剁殶缂佺喍绔寸挧鏉块挬濠?閸嬪繒些婢跺嫮鎮?
                if (s_cbs.on_parsed_oil_temp) s_cbs.on_parsed_oil_temp((uint32_t)oil_c);
            } else {
                ESP_LOGW(TAG, "21 01 parse failed: count=%d", count);
                record_oil_temp_failure(OIL_TEMP_MODE_TOYOTA_21_01);
            }
        } else if (p41 != NULL) {
            // Mode 01 閸濆秴绨? "41 PP DD ..."
            uint32_t d[6] = {0};
            uint32_t mode = 0, pid = 0;
            int values = sscanf(p41, "%x %x %x %x %x %x %x %x",
                &mode, &pid, &d[0], &d[1], &d[2], &d[3], &d[4], &d[5]);
            ESP_LOGD(TAG, "OBD mode01 mode=%02X pid=%02X d=%02X %02X %02X val=%d",
                     mode, pid, d[0], d[1], d[2], values);
            if (values >= 3 && mode == 0x41) {
                int dc = values - 2;
                
                // 婵″倹鐏夊锝呮躬閸楀繗顔呭Λ鈧ù瀣剁礉閸欘亜顦╅悶?RPM閿?x0C閿?
                if (s_protocol_detect_idx >= 0 && pid != 0x0C) {
                    break;  // 鐠哄疇绻冮棃鐐垫窗閺?PID
                }
                
                switch (pid) {
                    case 0x04: // 閸欐垵濮╅張楦跨閼?(0~100%)
                        if (dc >= 1 && s_cbs.on_parsed_load_pct && s_protocol_detect_idx < 0)
                            s_cbs.on_parsed_load_pct((uint32_t)d[0] * 100 / 255);
                        break;
                    case 0x05: // 濮樺瓨淇?
                        if (dc >= 1 && s_cbs.on_parsed_coolant_temp && s_protocol_detect_idx < 0)
                            s_cbs.on_parsed_coolant_temp((uint32_t)((int32_t)d[0] - 40));
                        break;
                    case 0x0C: // 鏉烆剟鈧?
                        if (dc >= 2) {
                            uint16_t rpm_val = (uint16_t)(((d[0] << 8) | d[1]) / 4);
                            
                            if (s_protocol_detect_idx >= 0) {
                                // 閸楀繗顔呭Λ鈧ù瀣佸?
                                s_protocol_detect_rpm = (int32_t)rpm_val;
                                s_protocol_detect_got_response = true;
                                ESP_LOGI(TAG, "[PROTOCOL_DETECT] Protocol %d: RPM=%u OK", s_protocol_detect_idx, rpm_val);
                            } else {
                                // 濮濓絽鐖跺Ο鈥崇础
                                if (s_cbs.on_parsed_rpm)
                                    s_cbs.on_parsed_rpm(rpm_val);
                            }
                        }
                        break;
                    case 0x0D: // 鏉烇箓鈧?
                        if (dc >= 1 && s_cbs.on_parsed_speed_kmh && s_protocol_detect_idx < 0)
                            s_cbs.on_parsed_speed_kmh((uint8_t)d[0]);
                        break;
                    case 0x0F: // 鏉╂稒鐨靛〒鈺佸
                        if (dc >= 1 && s_cbs.on_parsed_intake_temp && s_protocol_detect_idx < 0)
                            s_cbs.on_parsed_intake_temp((uint32_t)((int32_t)d[0] - 40));
                        break;
                    case 0x11: // 閼哄倹鐨甸梻銊ョ磻鎼?TPS (0~100%)
                        if (dc >= 1 && s_cbs.on_parsed_throttle_position && s_protocol_detect_idx < 0)
                            s_cbs.on_parsed_throttle_position((uint32_t)d[0] * 100 / 255);
                        break;
                    case 0x0B: // 鏉╂稒鐨靛褏顓哥紒婵嗩嚠閸樺濮?MAP (kPa) 閳?濞懧ょ枂鐞涖劌甯?
                        if (dc >= 1 && s_cbs.on_parsed_manifold_pressure && s_protocol_detect_idx < 0)
                            s_cbs.on_parsed_manifold_pressure((uint32_t)d[0]);
                        break;
                    case 0x5C: // 濞岃淇?PID (閺嶅洤鍣敍宀€鏁ゆ禍?ZD8)
                        if (dc >= 1 && s_cbs.on_parsed_oil_temp && s_protocol_detect_idx < 0) {
                            int32_t oil_temp = (int32_t)d[0] - 40;
                            // 妤犲矁鐦夐懠鍐ㄦ纯閿?40 閸?215鎺矯
                            if (oil_temp >= -40 && oil_temp <= 215) {
                                ESP_LOGI(TAG, "[PID 0x5C] Standard oil temp: %dC", (int)oil_temp);
                                record_oil_temp_success(OIL_TEMP_MODE_PID_5C);
                                s_cbs.on_parsed_oil_temp((uint32_t)oil_temp);
                            } else {
                                ESP_LOGD(TAG, "[PID 0x5C] Oil temp out of range: %d (raw=%02X)", (int)oil_temp, d[0]);
                                record_oil_temp_failure(OIL_TEMP_MODE_PID_5C);
                            }
                        }
                        break;
                    case 0x42: // 閻㈠灚鐫滈悽闈涘竾 (mV)
                        if (dc >= 2 && s_cbs.on_parsed_control_module_voltage && s_protocol_detect_idx < 0)
                            s_cbs.on_parsed_control_module_voltage((d[0] << 8) | d[1]);
                        break;
                    default:
                        ESP_LOGD(TAG, "Unhandled PID 0x%02X", pid);
                        break;
                }
            }
        } else if (p62 != NULL) {
            // Mode 22 閸濆秴绨? "62 HH LL D0 D1 ..."  (d0=A, d1=B)
            uint32_t mode22 = 0, ph = 0, pl = 0, d0 = 0, d1 = 0;
            int values = sscanf(p62, "%x %x %x %x %x", &mode22, &ph, &pl, &d0, &d1);
            if (values >= 4 && mode22 == 0x62 && s_cbs.on_parsed_oil_temp) {
                uint32_t pid16 = (ph << 8) | pl;
                if (pid16 == 0x1310) {
                    // Mazda Skyactiv 濞岃淇?PID 1310: (A*256+B)/100 - 40 (鎺矯), 闂団偓 2 娑擃亝鏆熼幑顔肩摟閼?
                    if (values >= 5) {
                        int32_t mazda_oil = (int32_t)(((d0 * 256) + d1) / 100) - 40;
                        if (mazda_oil >= -40 && mazda_oil <= 215) {
                            record_oil_temp_success(OIL_TEMP_MODE_MAZDA_22_1310);
                            s_cbs.on_parsed_oil_temp((uint32_t)mazda_oil);
                        } else {
                            ESP_LOGD(TAG, "[22 13 10] Oil temp out of range: %d (A=%02X B=%02X)", (int)mazda_oil, (unsigned)d0, (unsigned)d1);
                            record_oil_temp_failure(OIL_TEMP_MODE_MAZDA_22_1310);
                        }
                    } else {
                        record_oil_temp_failure(OIL_TEMP_MODE_MAZDA_22_1310);
                    }
                } else if (pid16 == 0x111F) {
                    // Mazda Skyactiv 濞岃淇?PID 111F: A - 50 (鎺矯)
                    int32_t mazda_oil = (int32_t)d0 - 50;
                    if (mazda_oil >= -40 && mazda_oil <= 215) {
                        record_oil_temp_success(OIL_TEMP_MODE_MAZDA_22_111F);
                        s_cbs.on_parsed_oil_temp((uint32_t)mazda_oil);
                    } else {
                        ESP_LOGD(TAG, "[22 11 1F] Oil temp out of range: %d (raw=%02X)", (int)mazda_oil, (unsigned)d0);
                        record_oil_temp_failure(OIL_TEMP_MODE_MAZDA_22_111F);
                    }
                } else if (pid16 == 0x1017 || pid16 == 0x0011 || pid16 == 0x1C00) {
                    record_oil_temp_success(OIL_TEMP_MODE_UDS_22_10_17);
                    s_cbs.on_parsed_oil_temp((uint32_t)((int32_t)d0 - 40));
                } else {
                    record_oil_temp_failure(OIL_TEMP_MODE_UDS_22_10_17);
                }
            }
        } else {
            // 閺冪姵鏅ラ弫鐗堝祦閹存牜鍑介弬鍥ㄦ拱閿涘湤O DATA閵嗕讣EARCHING閵嗕副K 缁涘绱?
            if (expect_mode21) {
                record_oil_temp_failure(OIL_TEMP_MODE_TOYOTA_21_01);
            }
            if (strstr(buf, "NO DATA")) {
                ESP_LOGI(TAG, "NO DATA for last PID"); // 鐠囧﹥鏌? 閸濐亙閲淧ID閺冪姵鏆熼幑?
            } else if (strstr(buf, "SEARCHING")) {
                ESP_LOGI(TAG, "ELM327 searching protocol...");
            } else {
                ESP_LOGI(TAG, "Other response: %.60s", buf); // 鐠囧﹥鏌? 閸忔湹绮張顏嗙叀閸濆秴绨?
            }
        }

        // 閺€璺哄煂鐎瑰本鏆ｉ崫宥呯安閸氬孩绔荤粚铏圭柈缁夘垳绱﹂崘鎻掑隘
        s_accum_len = 0;
        s_accum_buf[0] = '\0';
        break;
    }
    case ESP_GATTC_WRITE_CHAR_EVT: {
        if (param->write.status != ESP_GATT_OK) {
            ESP_LOGW(TAG, "Write failed status=%d", param->write.status);
            s_elm_ready = true; // 閸愭瑥銇戠拹銉︽娑旂喕顩﹂柌濠冩杹閿涘矂妲诲銏ｇ枂鐠囶澀鎹㈤崝鈩冩娑斿懎宕辨担?
        }
        break;
    }
    case ESP_GATTC_DISCONNECT_EVT: {
        s_connected = false;
        s_conn_id = 0xFFFF;
        s_have_service = false;
        s_service_start = 0x0001;
        s_service_end = 0xFFFF;
        s_all_attr_end = 0xFFFF;
        s_have_18f0 = false;
        s_18f0_start = s_18f0_end = 0;
        s_have_ff12 = false;
        s_ff12_start = s_ff12_end = 0;
        s_write_type = ESP_GATT_WRITE_TYPE_RSP; // 閺傤厼绱戦崥搴ㄥ櫢缂冾喖鍟撶猾璇茬€?
        s_expect_mode21 = false;
        s_char_write_handle = s_char_notify_handle = s_cccd_handle = 0;
        s_accum_len = 0; s_accum_buf[0] = '\0'; // 濞撳懐鈹栭崫宥呯安缁鳖垳袧缂傛挸鍟块崠?
        s_protocol_detect_idx = -1;  // 濞撳懐鎮婇崡蹇氼唴濡偓濞村濮搁幀?
        s_protocol_detect_got_response = false;
        s_protocol_detect_rpm = -1;
        s_oil_query_mode = 0;  // 闁插秶鐤嗗▽瑙勪刊閺屻儴顕楃拋鈩冩殶
        if (s_cbs.on_disconnected) s_cbs.on_disconnected();
        // Only restart scan if we were in scan-only mode (BLE selection page)
        // Otherwise, user must manually reconnect from settings
        if (s_scan_only_mode) {
            start_scan();
        }
        break;
    }
    default:
        break;
    }
}


void elm327_ble_start_default(const char *target_name) {

    const elm327_ble_callbacks_t cbs = {
        .on_connected = default_on_connected,
        .on_disconnected = default_on_disconnected,
        .on_raw_notify = default_on_raw_notify,
        .on_parsed_rpm = default_on_parsed_rpm,
        .on_parsed_speed_kmh = default_on_parsed_speed,
        .on_parsed_coolant_temp = default_on_parsed_coolant_temp,
        .on_parsed_intake_temp = default_on_parsed_intake_temp,
        .on_parsed_oil_temp = default_on_parsed_oil_temp,
        .on_parsed_load_pct = default_on_parsed_load_pct,
        .on_parsed_control_module_voltage = default_on_parsed_control_module_voltage,
        .on_parsed_throttle_position = default_on_parsed_throttle_position,
        .on_parsed_manifold_pressure = default_on_parsed_manifold_pressure,
    };
    s_scan_only_mode = false;
    elm327_ble_init_and_start(target_name, &cbs);
    if (!s_poll_task_started) {
        xTaskCreate(obd_poll_task, "obd_poll", 3072, NULL, 4, NULL);
        s_poll_task_started = true;
    }
}

// ---- 閹殿偅寮垮Ο鈥崇础鐎圭偟骞?----

static void ble_ensure_init(void) {
    if (s_ble_inited) return;
    // 閸掓繂顫愰崠?BLE 閸楀繗顔呴弽鍫礄娑撳秷顔曢惄顔界垼閸氬稄绱濇禒鍛灥婵瀵查敍?
    elm327_ble_init_and_start(NULL, NULL);
}

void elm327_ble_scan_only_start(int duration_s, ble_scan_found_cb_t cb) {
    ble_ensure_init();
    if (!ble_scan_buffer_ensure()) {
        return;
    }
    s_scan_only_mode = true;
    s_scan_cb = cb;
    s_scan_count = 0;
    memset(s_scan_list, 0, BLE_SCAN_BUFFER_BYTES(BLE_SCAN_BUFFER_CAPACITY));
    ESP_LOGI(TAG, "Starting scan-only mode (%ds)...", duration_s);
    esp_ble_gap_start_scanning(duration_s);
}

void elm327_ble_scan_only_stop(void) {
    esp_ble_gap_stop_scanning();
    s_scan_only_mode = false;
    ESP_LOGI(TAG, "Scan-only stopped. Found %d devices.", s_scan_count);
}

void elm327_ble_connect_by_name(const char *name) {
    if (!name || name[0] == '\0') return;
    ESP_LOGI(TAG, "Connect by name: %s", name);
    s_scan_only_mode = false;
    strncpy(s_target_name, name, sizeof(s_target_name) - 1);
    s_target_name[sizeof(s_target_name) - 1] = '\0';

    // 鐠佸墽鐤嗘妯款吇閸ョ偠鐨熼敍鍫濐洤閺嬫粏绻曞▽鈩冩箒閿?
    if (!s_cbs.on_connected) {
        s_cbs.on_connected = default_on_connected;
        s_cbs.on_disconnected = default_on_disconnected;
        s_cbs.on_raw_notify = default_on_raw_notify;
        s_cbs.on_parsed_rpm = default_on_parsed_rpm;
        s_cbs.on_parsed_speed_kmh = default_on_parsed_speed;
        s_cbs.on_parsed_coolant_temp = default_on_parsed_coolant_temp;
        s_cbs.on_parsed_intake_temp = default_on_parsed_intake_temp;
        s_cbs.on_parsed_oil_temp = default_on_parsed_oil_temp;
        s_cbs.on_parsed_load_pct = default_on_parsed_load_pct;
        s_cbs.on_parsed_control_module_voltage = default_on_parsed_control_module_voltage;
        s_cbs.on_parsed_throttle_position = default_on_parsed_throttle_position;
        s_cbs.on_parsed_manifold_pressure = default_on_parsed_manifold_pressure;
    }
    // 瀵偓婵澹傞幓蹇ョ礉閹垫儳鍩岄崥搴ゅ殰閸斻劏绻涢幒?
    esp_ble_gap_start_scanning(15);
    // 閸掓稑缂撴潪顔款嚄娴犺濮熼敍鍫濐洤閺嬫粏绻曞▽鈩冩箒閿?
    if (!s_poll_task_started) {
        xTaskCreate(obd_poll_task, "obd_poll", 3072, NULL, 4, NULL);
        s_poll_task_started = true;
    }
}

bool elm327_ble_is_connected(void) {
    return s_connected;
}

void elm327_ble_disconnect(void) {
    if (s_connected && s_gattc_if != 0 && s_conn_id != 0xFFFF) {
        ESP_LOGI(TAG, "Disconnecting from BLE device...");
        esp_ble_gattc_close(s_gattc_if, s_conn_id);
    }
}

const char *elm327_ble_get_connected_name(void) {
    return s_target_name;
}

// ---- 濞岃淇弽鈥冲櫙閹恒儱褰涚€圭偟骞?----
void elm327_oil_temp_set_offset(int8_t offset_c) {
    s_oil_temp_offset = offset_c;
    ESP_LOGI(TAG, "OIL temp offset set to %d鎺矯", offset_c);
}

int8_t elm327_oil_temp_get_offset(void) {
    return s_oil_temp_offset;
}

void elm327_oil_temp_get_diag(elm327_oil_diag_t *out) {
    if (!out) return;
    out->mode0_ok = s_oil_diag.mode0_ok;
    out->mode1_ok = s_oil_diag.mode1_ok;
    out->mode2_ok = s_oil_diag.mode2_ok;
    out->mode0_fail = s_oil_diag.mode0_fail;
    out->mode1_fail = s_oil_diag.mode1_fail;
    out->mode2_fail = s_oil_diag.mode2_fail;
    out->last_raw = s_oil_diag.last_raw_temp;
    out->last_filtered = s_oil_diag.last_filtered_temp;
    out->current_mode = s_oil_query_mode;
    
    ESP_LOGI(TAG, "OIL DIAG: Mode0(01 5C)=%u/%u, Mode1(22 10 17)=%u/%u, Mode2(21 01)=%u/%u, Mode3(22 11 1F)=%u/%u, Mode4(22 13 10)=%u/%u",
             out->mode0_ok, out->mode0_fail, out->mode1_ok, out->mode1_fail,
             out->mode2_ok, out->mode2_fail, s_oil_diag.mode3_ok, s_oil_diag.mode3_fail,
             s_oil_diag.mode4_ok, s_oil_diag.mode4_fail);
}
