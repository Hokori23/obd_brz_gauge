#pragma once

#include <stdint.h>

typedef int16_t lv_coord_t;
typedef uint16_t lv_dir_t;
typedef struct _lv_obj_t lv_obj_t;
typedef struct _lv_anim_t {
    void *var;
} lv_anim_t;

#define LV_DIR_NONE 0x00u
#define LV_DIR_LEFT 0x01u
#define LV_DIR_RIGHT 0x02u
#define LV_DIR_TOP 0x04u
#define LV_DIR_BOTTOM 0x08u

#define LV_UNUSED(x) ((void)(x))
