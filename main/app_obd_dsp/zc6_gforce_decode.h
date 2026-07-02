#ifndef APP_OBD_DSP_ZC6_GFORCE_DECODE_H
#define APP_OBD_DSP_ZC6_GFORCE_DECODE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define ZC6_GFORCE_LAT_X100_FROM_BYTE(byte_val) \
    ((int16_t)((int8_t)(uint8_t)(byte_val)) * 20)

#define ZC6_GFORCE_LON_X100_FROM_BYTE(byte_val) \
    ((int16_t)((int8_t)(uint8_t)(byte_val)) * -10)

static inline bool zc6_gforce_decode_0d0_payload(const uint8_t payload[8],
                                                 int16_t *lat_x100_out,
                                                 int16_t *lon_x100_out)
{
    if (payload == NULL || lat_x100_out == NULL || lon_x100_out == NULL) {
        return false;
    }

    *lat_x100_out = ZC6_GFORCE_LAT_X100_FROM_BYTE(payload[6]);
    *lon_x100_out = ZC6_GFORCE_LON_X100_FROM_BYTE(payload[7]);
    return true;
}

#endif
