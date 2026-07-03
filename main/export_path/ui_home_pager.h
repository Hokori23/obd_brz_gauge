#ifndef OBD_PRJ_UI_HOME_PAGER_H
#define OBD_PRJ_UI_HOME_PAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "lvgl.h"
#include "bsp_obd_dsp/nvs_storage.h"

#define UI_HOME_MAX_TILE_COUNT (1u + UI_DASHBOARD_MAX_PAGES + 1u)
#define UI_HOME_PAGER_DRAG_START_THRESHOLD 12
#define UI_HOME_PAGER_AXIS_BIAS_THRESHOLD 4
#define UI_HOME_PAGER_VERTICAL_TRIGGER_THRESHOLD 20
#define UI_HOME_PAGER_REBOUND_THRESHOLD_PERCENT 18
#define UI_HOME_PAGER_SETTLE_ANIM_MIN_MS 72
#define UI_HOME_PAGER_SETTLE_ANIM_MAX_MS 108
#define UI_HOME_PAGER_SETTLE_TRACE_TAIL_MS 24

typedef enum {
    UI_HOME_PAGER_AXIS_NONE = 0,
    UI_HOME_PAGER_AXIS_X,
    UI_HOME_PAGER_AXIS_Y
} ui_home_pager_axis_t;

typedef struct {
    uint8_t from_page;
    uint8_t to_page;
    bool changed;
} ui_home_pager_commit_t;

typedef struct ui_home_pager ui_home_pager_t;

typedef void (*ui_home_pager_drag_begin_cb)(uint8_t from_page, void *user);
typedef void (*ui_home_pager_commit_cb)(const ui_home_pager_commit_t *commit, void *user);
typedef void (*ui_home_pager_vertical_cb)(lv_dir_t dir, uint8_t active_page, void *user);

typedef struct {
    lv_obj_t *parent;
    lv_coord_t width;
    lv_coord_t height;
    uint8_t page_count;
    uint8_t initial_page;
    ui_home_pager_drag_begin_cb drag_begin_cb;
    ui_home_pager_commit_cb commit_cb;
    ui_home_pager_vertical_cb vertical_cb;
    void *user;
} ui_home_pager_config_t;

/*
 * The pager state lives in the caller so the home runtime can keep a single
 * screen-scoped instance without heap allocation. Treat the fields as private.
 */
struct ui_home_pager {
    lv_obj_t *root;
    lv_obj_t *track;
    lv_obj_t *pages[UI_HOME_MAX_TILE_COUNT];
    uint8_t page_count;
    uint8_t active_page;
    uint8_t target_page;
    bool locked;
    bool settling;
    ui_home_pager_axis_t axis;
    lv_coord_t press_start_x;
    lv_coord_t press_start_y;
    lv_coord_t track_start_x;
    lv_coord_t last_dx;
    lv_coord_t page_width;
    lv_coord_t page_height;
    lv_anim_t settle_anim;
    ui_home_pager_drag_begin_cb drag_begin_cb;
    ui_home_pager_commit_cb commit_cb;
    ui_home_pager_vertical_cb vertical_cb;
    void *user;
};

static inline uint8_t ui_home_pager_clamp_page_index(int32_t page_id, uint8_t page_count)
{
    if (page_count == 0u) {
        return 0u;
    }
    if (page_id <= 0) {
        return 0u;
    }
    if (page_id >= (int32_t)page_count) {
        return (uint8_t)(page_count - 1u);
    }
    return (uint8_t)page_id;
}

/*
 * Keep the axis lock policy as pure logic so host-side tests can cover the
 * gesture state machine without pulling LVGL runtime into the test binary.
 */
static inline ui_home_pager_axis_t ui_home_pager_axis_from_delta(int32_t dx,
                                                                 int32_t dy,
                                                                 bool allow_vertical)
{
    int32_t abs_dx = (dx >= 0) ? dx : -dx;
    int32_t abs_dy = (dy >= 0) ? dy : -dy;

    if (abs_dx >= UI_HOME_PAGER_DRAG_START_THRESHOLD &&
        abs_dx >= (abs_dy + UI_HOME_PAGER_AXIS_BIAS_THRESHOLD)) {
        return UI_HOME_PAGER_AXIS_X;
    }
    if (allow_vertical &&
        abs_dy >= UI_HOME_PAGER_DRAG_START_THRESHOLD &&
        abs_dy >= (abs_dx + UI_HOME_PAGER_AXIS_BIAS_THRESHOLD)) {
        return UI_HOME_PAGER_AXIS_Y;
    }

    return UI_HOME_PAGER_AXIS_NONE;
}

static inline lv_dir_t ui_home_pager_vertical_dir_from_delta(int32_t dy)
{
    int32_t abs_dy = (dy >= 0) ? dy : -dy;

    if (abs_dy < UI_HOME_PAGER_VERTICAL_TRIGGER_THRESHOLD) {
        return LV_DIR_NONE;
    }

    return (dy < 0) ? LV_DIR_TOP : LV_DIR_BOTTOM;
}

static inline uint8_t ui_home_pager_target_page_from_release(lv_coord_t track_x,
                                                             lv_coord_t page_width,
                                                             int32_t dx,
                                                             uint8_t from_page,
                                                             uint8_t page_count)
{
    int32_t threshold;
    int32_t target = from_page;

    LV_UNUSED(track_x);

    if (page_width <= 0 || page_count == 0u) {
        return 0u;
    }

    threshold = ((int32_t)page_width * UI_HOME_PAGER_REBOUND_THRESHOLD_PERCENT) / 100;
    if (threshold < 1) {
        threshold = 1;
    }

    if (dx <= -threshold) {
        target = (int32_t)from_page + 1;
    } else if (dx >= threshold) {
        target = (int32_t)from_page - 1;
    }

    return ui_home_pager_clamp_page_index(target, page_count);
}

static inline uint32_t ui_home_pager_settle_duration_ms_for_distance(lv_coord_t distance,
                                                                     lv_coord_t page_width)
{
    uint32_t abs_distance;
    uint32_t abs_page_width;
    uint32_t duration;
    uint32_t span;

    if (distance < 0) {
        distance = (lv_coord_t)(-distance);
    }
    if (page_width < 0) {
        page_width = (lv_coord_t)(-page_width);
    }

    abs_distance = (uint32_t)distance;
    abs_page_width = (page_width > 0) ? (uint32_t)page_width : 0u;
    span = UI_HOME_PAGER_SETTLE_ANIM_MAX_MS - UI_HOME_PAGER_SETTLE_ANIM_MIN_MS;
    duration = UI_HOME_PAGER_SETTLE_ANIM_MIN_MS;

    if (abs_page_width > 0u && abs_distance > 0u) {
        if (abs_distance > abs_page_width) {
            abs_distance = abs_page_width;
        }
        duration += (span * abs_distance) / abs_page_width;
    }

    if (duration < UI_HOME_PAGER_SETTLE_ANIM_MIN_MS) {
        duration = UI_HOME_PAGER_SETTLE_ANIM_MIN_MS;
    }
    if (duration > UI_HOME_PAGER_SETTLE_ANIM_MAX_MS) {
        duration = UI_HOME_PAGER_SETTLE_ANIM_MAX_MS;
    }

    return duration;
}

void ui_home_pager_init(ui_home_pager_t *pager, const ui_home_pager_config_t *cfg);
void ui_home_pager_deinit(ui_home_pager_t *pager);
lv_obj_t *ui_home_pager_root(const ui_home_pager_t *pager);
lv_obj_t *ui_home_pager_page(const ui_home_pager_t *pager, uint8_t page_id);
uint8_t ui_home_pager_active_page(const ui_home_pager_t *pager);
void ui_home_pager_set_active_page(ui_home_pager_t *pager, uint8_t page_id, bool animate);
void ui_home_pager_set_locked(ui_home_pager_t *pager, bool locked);
void ui_home_pager_set_page_count(ui_home_pager_t *pager, uint8_t page_count);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
