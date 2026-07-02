#include "../../main/app_obd_dsp/zc6_gforce_decode.h"

_Static_assert(ZC6_GFORCE_LAT_X100_FROM_BYTE(0x00u) == 0,
               "0x00 lateral byte should decode to 0.00g");
_Static_assert(ZC6_GFORCE_LAT_X100_FROM_BYTE(0x01u) == 20,
               "0x01 lateral byte should decode to 0.20g");
_Static_assert(ZC6_GFORCE_LAT_X100_FROM_BYTE(0xFEu) == -40,
               "0xFE lateral byte should decode to -0.40g");

_Static_assert(ZC6_GFORCE_LON_X100_FROM_BYTE(0x00u) == 0,
               "0x00 longitudinal byte should decode to 0.00g");
_Static_assert(ZC6_GFORCE_LON_X100_FROM_BYTE(0x03u) == -30,
               "0x03 longitudinal byte should decode to -0.30g");
_Static_assert(ZC6_GFORCE_LON_X100_FROM_BYTE(0xFDu) == 30,
               "0xFD longitudinal byte should decode to +0.30g");

static void test_payload_decode_compiles(void)
{
    const uint8_t payload[8] = {0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x05u, 0xFDu};
    int16_t lat_x100 = 0;
    int16_t lon_x100 = 0;
    bool ok = zc6_gforce_decode_0d0_payload(payload, &lat_x100, &lon_x100);

    (void)ok;
    (void)lat_x100;
    (void)lon_x100;
}

int test_zc6_gforce_decode_dummy_symbol(void)
{
    test_payload_decode_compiles();
    return 0;
}
