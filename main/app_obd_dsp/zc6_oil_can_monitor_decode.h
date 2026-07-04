#ifndef APP_OBD_DSP_ZC6_OIL_CAN_MONITOR_DECODE_H
#define APP_OBD_DSP_ZC6_OIL_CAN_MONITOR_DECODE_H

#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

static inline bool zc6_oil_can_extract_360_payload_from_monitor_line(const char *line,
                                                                     uint8_t payload_out[8])
{
    uint32_t tokens[16];
    uint8_t token_lens[16];
    size_t token_count = 0u;
    size_t byte_count = 0u;
    int header_index = -1;
    const char *p = line;

    if (line == NULL || payload_out == NULL) {
        return false;
    }

    while (*p != '\0' && token_count < (sizeof(tokens) / sizeof(tokens[0]))) {
        char token_buf[5] = {0};
        size_t len = 0u;

        if (!isxdigit((unsigned char)*p)) {
            ++p;
            continue;
        }

        while (isxdigit((unsigned char)*p) && len < sizeof(token_buf) - 1u) {
            token_buf[len++] = *p++;
        }
        while (isxdigit((unsigned char)*p)) {
            ++p;
        }

        if (len == 0u) {
            continue;
        }

        tokens[token_count] = (uint32_t)strtoul(token_buf, NULL, 16);
        token_lens[token_count] = (uint8_t)len;
        ++token_count;
    }

    for (size_t i = 0u; i < token_count; ++i) {
        if (token_lens[i] == 3u && tokens[i] == 0x360u) {
            header_index = (int)i;
            break;
        }
    }
    if (header_index < 0) {
        return false;
    }

    for (size_t i = (size_t)header_index + 1u; i < token_count && byte_count < 8u; ++i) {
        if (token_lens[i] == 2u) {
            payload_out[byte_count++] = (uint8_t)tokens[i];
        }
    }

    return byte_count >= 3u;
}

static inline bool zc6_oil_can_decode_360_payload(const uint8_t payload[8], int16_t *oil_temp_c_out)
{
    int16_t oil_temp_c;

    if (payload == NULL || oil_temp_c_out == NULL) {
        return false;
    }

    oil_temp_c = (int16_t)payload[2] - 40;
    if (oil_temp_c < -40 || oil_temp_c > 215) {
        return false;
    }

    *oil_temp_c_out = oil_temp_c;
    return true;
}

static inline bool zc6_oil_can_decode_monitor_line(const char *line, int16_t *oil_temp_c_out)
{
    uint8_t payload[8] = {0};

    if (!zc6_oil_can_extract_360_payload_from_monitor_line(line, payload)) {
        return false;
    }

    return zc6_oil_can_decode_360_payload(payload, oil_temp_c_out);
}

#endif
