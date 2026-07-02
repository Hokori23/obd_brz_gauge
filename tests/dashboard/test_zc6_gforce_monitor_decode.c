#include "../../main/app_obd_dsp/zc6_gforce_monitor_decode.h"

static void test_monitor_decode_compiles(void)
{
    static const char *sample_line = "0D0 00 11 22 33 44 55 05 FD";
    uint8_t payload[8] = {0};
    int16_t lat_x100 = 0;
    int16_t lon_x100 = 0;
    bool payload_ok = zc6_gforce_extract_0d0_payload_from_monitor_line(sample_line, payload);
    bool decode_ok = zc6_gforce_decode_monitor_line(sample_line, &lat_x100, &lon_x100);

    (void)payload_ok;
    (void)decode_ok;
    (void)payload;
    (void)lat_x100;
    (void)lon_x100;
}

int test_zc6_gforce_monitor_decode_dummy_symbol(void)
{
    test_monitor_decode_compiles();
    return 0;
}
