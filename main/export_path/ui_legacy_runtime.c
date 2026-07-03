#include "ui_runtime_common.h"

#include "ui.h"
#include "ui_font_profile.h"

#include "bsp_obd_dsp/elm327_ble_client.h"
#include "bsp_obd_dsp/nvs_storage.h"

typedef struct {
    const char *name;
    const char *unit;
    uint32_t color;
} disp_item_meta_t;

#define SWEEP_RPM_PEAK   8000
#define SWEEP_SPEED_PEAK 999
#define SWEEP_STEPS_UP   6
#define SWEEP_STEPS_DOWN 6
#define SWEEP_TOTAL      (SWEEP_STEPS_UP + SWEEP_STEPS_DOWN)

static int s_sweep_step = 0;
static bool s_sweep_pending = false;
static bool s_prev_ble_connected = false;

static const disp_item_meta_t s_disp_meta[DISP_ITEM_COUNT] = {
    {"CLT", "\xC2\xB0" "C", 0x44AAFF},
    {"IAT", "\xC2\xB0" "C", 0x44FF88},
    {"OIL", "\xC2\xB0" "C", 0xFF7722},
    {"LOD", "%", 0xFFCC00},
    {"TPS", "%", 0xFF8844},
    {"RPM", "rpm", 0x66CCFF},
    {"SPD", "km/h", 0xFFFFFF},
    {"BAT", "V", 0xAACCFF},
    {"OIP", "bar", 0xFFD166},
    {"BKT", "\xC2\xB0" "C", 0xFF5A5A},
    {"BST", "bar", 0x00DD88},
};

/** 返回当前开机扫表动画所在的步进序号。 */
int ui_runtime_sweep_step_get(void)
{
    return s_sweep_step;
}

/**
 * 从一组缓存值里读取指定仪表项的显示值
 *
 * 核心职责：统一处理各类数据项的有效性判断，并输出可显示的整数结果
 */
bool disp_item_read_value(disp_item_t item,
                          int16_t clt,
                          int16_t iat,
                          int16_t oil,
                          int16_t load_pct,
                          int16_t tps,
                          int32_t bat_mv,
                          int16_t oilp_x10,
                          int16_t brake_x10,
                          uint16_t rpm,
                          uint16_t speed,
                          int16_t boost_x10,
                          int32_t *out)
{
    if (!out) {
        return false;
    }

    switch (item) {
    case DISP_ITEM_CLT:
        if (clt > -40) {
            *out = clt;
            return true;
        }
        return false;
    case DISP_ITEM_IAT:
        if (iat > -40) {
            *out = iat;
            return true;
        }
        return false;
    case DISP_ITEM_OIL:
        if (oil > -41) {
            *out = oil;
            return true;
        }
        return false;
    case DISP_ITEM_LOAD:
        if (load_pct >= 0) {
            *out = load_pct;
            return true;
        }
        return false;
    case DISP_ITEM_TPS:
        if (tps >= 0) {
            *out = tps;
            return true;
        }
        return false;
    case DISP_ITEM_RPM:
        *out = rpm;
        return true;
    case DISP_ITEM_SPEED:
        *out = speed;
        return true;
    case DISP_ITEM_BAT:
        if (bat_mv > 0) {
            *out = bat_mv;
            return true;
        }
        return false;
    case DISP_ITEM_OILP:
        if (oilp_x10 >= 0) {
            *out = oilp_x10;
            return true;
        }
        return false;
    case DISP_ITEM_BKT:
        if (brake_x10 > -1000) {
            *out = brake_x10;
            return true;
        }
        return false;
    case DISP_ITEM_BOOST:
        if (boost_x10 != -32768) {
            *out = boost_x10;
            return true;
        }
        return false;
    default:
        return false;
    }
}

/** 根据扫表动画进度生成某个仪表项的演示值。 */
int32_t disp_item_sweep_value(disp_item_t item, float ratio)
{
    switch (item) {
    case DISP_ITEM_CLT:
    case DISP_ITEM_IAT:
    case DISP_ITEM_OIL:
        return (int32_t)(120.0f * ratio);
    case DISP_ITEM_LOAD:
    case DISP_ITEM_TPS:
        return (int32_t)(100.0f * ratio);
    case DISP_ITEM_RPM:
        return (int32_t)(SWEEP_RPM_PEAK * ratio);
    case DISP_ITEM_SPEED:
        return (int32_t)(SWEEP_SPEED_PEAK * ratio);
    case DISP_ITEM_BAT:
        return (int32_t)(12000.0f + 2400.0f * ratio);
    case DISP_ITEM_OILP:
        return (int32_t)(100.0f * ratio);
    case DISP_ITEM_BKT:
        return (int32_t)(600.0f * ratio);
    case DISP_ITEM_BOOST:
        return (int32_t)(20.0f * ratio);
    default:
        return 0;
    }
}

/**
 * 把某个仪表值格式化成标签文本
 *
 * 核心职责：按不同单位选择合适的小数位和符号格式
 */
void disp_item_set_text(lv_obj_t *label, disp_item_t item, int32_t value, bool valid)
{
    if (!label) {
        return;
    }

    if (!valid) {
        ui_label_set_text_if_changed(label, "--");
        return;
    }

    if (item == DISP_ITEM_BAT) {
        ui_label_set_text_fmt_if_changed(label, "%d.%d", (int)(value / 1000), (int)((value % 1000) / 100));
    } else if (item == DISP_ITEM_OILP) {
        int32_t abs_val = (value < 0) ? -value : value;
        ui_label_set_text_fmt_if_changed(label, "%d.%d", (int)(value / 10), (int)(abs_val % 10));
    } else if (item == DISP_ITEM_BKT) {
        ui_label_set_text_fmt_if_changed(label, "%ld", (long)(value / 10));
    } else if (item == DISP_ITEM_BOOST) {
        int32_t abs_val = (value < 0) ? -value : value;
        ui_label_set_text_fmt_if_changed(label,
                                         "%s%d.%d",
                                         (value < 0) ? "-" : "",
                                         (int)(abs_val / 10),
                                         (int)(abs_val % 10));
    } else {
        ui_label_set_text_fmt_if_changed(label, "%ld", (long)value);
    }
}

/** 按当前数值和告警阈值刷新标签颜色。 */
void disp_item_set_value_color(lv_obj_t *label,
                               disp_item_t item,
                               int32_t value,
                               bool valid,
                               int16_t brake_warn_x10,
                               int16_t oil_warn_x10)
{
    lv_color_t color = lv_color_hex(0xFFFFFF);

    if (!label) {
        return;
    }

    if (valid && item == DISP_ITEM_BKT && value >= brake_warn_x10) {
        color = lv_color_hex(0xFF4D4D);
    } else if (valid && item == DISP_ITEM_OILP && value >= oil_warn_x10) {
        color = lv_color_hex(0xFF4D4D);
    }

    lv_obj_set_style_text_color(label, color, LV_PART_MAIN);
}

/** 同步某个槽位的名称、单位和标题色。 */
void disp_item_sync_meta(lv_obj_t *name_label,
                         lv_obj_t *unit_label,
                         uint8_t *cache_slot,
                         disp_item_t item)
{
    if (!name_label || !unit_label || !cache_slot) {
        return;
    }

    if (*cache_slot == (uint8_t)item) {
        return;
    }

    lv_label_set_text(name_label, s_disp_meta[item].name);
    lv_label_set_text(unit_label, s_disp_meta[item].unit);
    lv_obj_set_style_text_color(name_label, lv_color_hex(s_disp_meta[item].color), LV_PART_MAIN);
    *cache_slot = (uint8_t)item;
}

/** 返回指定显示项的短名称。 */
const char *ui_disp_item_name(uint8_t item)
{
    if (item >= DISP_ITEM_COUNT) {
        return "";
    }
    return s_disp_meta[item].name;
}

/**
 * 推进旧版运行时动画状态机
 *
 * 核心职责：检测蓝牙连上瞬间，并驱动开机扫表动画的起停节奏
 */
void ui_legacy_runtime_tick(bool logo_visible)
{
    bool ble_now = elm327_ble_is_connected();

    if (ble_now && !s_prev_ble_connected) {
        if (logo_visible) {
            s_sweep_pending = true;
        } else {
            s_sweep_step = 1;
        }
    }
    s_prev_ble_connected = ble_now;

    if (s_sweep_step > 0) {
        s_sweep_step++;
        if (s_sweep_step > SWEEP_TOTAL) {
            s_sweep_step = 0;
        }
    }
}

/** 在 Logo 页面退出后补触发挂起中的扫表动画。 */
void ui_legacy_runtime_on_logo_exit(void)
{
    if (s_sweep_pending) {
        s_sweep_pending = false;
        s_sweep_step = 1;
    }
}
