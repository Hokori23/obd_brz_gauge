#include "ui_home_pager.h"

#include <string.h>

static void ui_home_pager_apply_track_x(ui_home_pager_t *pager, lv_coord_t x)
{
    if (pager == NULL || pager->track == NULL) {
        return;
    }

    lv_obj_set_x(pager->track, x);
}

static lv_coord_t ui_home_pager_track_x_for_page(const ui_home_pager_t *pager, uint8_t page_id)
{
    if (pager == NULL || pager->page_width <= 0) {
        return 0;
    }

    return (lv_coord_t)(-((lv_coord_t)page_id) * pager->page_width);
}

static lv_coord_t ui_home_pager_clamp_track_x(const ui_home_pager_t *pager, lv_coord_t x)
{
    lv_coord_t min_x;

    if (pager == NULL || pager->page_count <= 1u || pager->page_width <= 0) {
        return 0;
    }

    min_x = (lv_coord_t)(-((lv_coord_t)(pager->page_count - 1u)) * pager->page_width);
    if (x < min_x) {
        return min_x;
    }
    if (x > 0) {
        return 0;
    }

    return x;
}

static void ui_home_pager_settle_exec_cb(void *var, int32_t value)
{
    ui_home_pager_t *pager = (ui_home_pager_t *)var;

    ui_home_pager_apply_track_x(pager, (lv_coord_t)value);
}

static void ui_home_pager_settle_deleted_cb(lv_anim_t *anim)
{
    ui_home_pager_t *pager = (ui_home_pager_t *)anim->var;

    if (pager == NULL) {
        return;
    }

    pager->settling = false;
    pager->axis = UI_HOME_PAGER_AXIS_NONE;
    pager->last_dx = 0;
    ui_home_pager_apply_track_x(pager, ui_home_pager_track_x_for_page(pager, pager->active_page));
}

static void ui_home_pager_stop_settle(ui_home_pager_t *pager)
{
    if (pager == NULL || pager->track == NULL) {
        return;
    }

    lv_anim_del(pager, (lv_anim_exec_xcb_t)ui_home_pager_settle_exec_cb);
    pager->settling = false;
}

static void ui_home_pager_emit_commit(ui_home_pager_t *pager, uint8_t from_page, uint8_t to_page)
{
    ui_home_pager_commit_t commit;

    if (pager == NULL || pager->commit_cb == NULL) {
        return;
    }

    commit.from_page = from_page;
    commit.to_page = to_page;
    commit.changed = (from_page != to_page);
    pager->commit_cb(&commit, pager->user);
}

static void ui_home_pager_emit_drag_begin(ui_home_pager_t *pager, uint8_t from_page)
{
    if (pager == NULL || pager->drag_begin_cb == NULL) {
        return;
    }

    pager->drag_begin_cb(from_page, pager->user);
}

static void ui_home_pager_start_settle(ui_home_pager_t *pager, uint8_t to_page)
{
    lv_coord_t current_x;
    lv_coord_t target_x;

    if (pager == NULL || pager->track == NULL) {
        return;
    }

    pager->active_page = to_page;
    pager->target_page = to_page;

    current_x = lv_obj_get_x(pager->track);
    target_x = ui_home_pager_track_x_for_page(pager, to_page);
    if (current_x == target_x) {
        pager->settling = false;
        pager->axis = UI_HOME_PAGER_AXIS_NONE;
        pager->last_dx = 0;
        return;
    }

    pager->settling = true;
    lv_anim_init(&pager->settle_anim);
    lv_anim_set_var(&pager->settle_anim, pager);
    lv_anim_set_values(&pager->settle_anim, current_x, target_x);
    lv_anim_set_time(&pager->settle_anim, UI_HOME_PAGER_SETTLE_ANIM_MS);
    lv_anim_set_path_cb(&pager->settle_anim, lv_anim_path_ease_out);
    lv_anim_set_exec_cb(&pager->settle_anim, (lv_anim_exec_xcb_t)ui_home_pager_settle_exec_cb);
    lv_anim_set_deleted_cb(&pager->settle_anim, ui_home_pager_settle_deleted_cb);
    lv_anim_start(&pager->settle_anim);
}

static void ui_home_pager_commit_release(ui_home_pager_t *pager)
{
    uint8_t from_page;
    uint8_t to_page;

    if (pager == NULL) {
        return;
    }

    from_page = pager->target_page;
    to_page = ui_home_pager_target_page_from_release(lv_obj_get_x(pager->track),
                                                     pager->page_width,
                                                     pager->last_dx,
                                                     from_page,
                                                     pager->page_count);

    ui_home_pager_emit_commit(pager, from_page, to_page);
    ui_home_pager_start_settle(pager, to_page);
}

static void ui_home_pager_event_cb(lv_event_t *e)
{
    ui_home_pager_t *pager = (ui_home_pager_t *)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    lv_indev_t *indev;
    lv_point_t point;
    int32_t dx;
    int32_t dy;

    if (pager == NULL || pager->root == NULL || pager->track == NULL) {
        return;
    }

    indev = lv_event_get_indev(e);
    if (indev == NULL) {
        return;
    }

    if (code == LV_EVENT_PRESSED) {
        if (pager->locked || pager->settling) {
            return;
        }

        lv_indev_get_point(indev, &point);
        pager->axis = UI_HOME_PAGER_AXIS_NONE;
        pager->target_page = pager->active_page;
        pager->press_start_x = point.x;
        pager->press_start_y = point.y;
        pager->track_start_x = lv_obj_get_x(pager->track);
        pager->last_dx = 0;
        return;
    }

    if (code != LV_EVENT_PRESSING && code != LV_EVENT_RELEASED && code != LV_EVENT_PRESS_LOST) {
        return;
    }

    if (pager->locked || pager->settling) {
        return;
    }

    lv_indev_get_point(indev, &point);
    dx = point.x - pager->press_start_x;
    dy = point.y - pager->press_start_y;

    if (pager->axis == UI_HOME_PAGER_AXIS_NONE) {
        pager->axis = ui_home_pager_axis_from_delta(dx, dy, pager->active_page == 0u);
        if (pager->axis == UI_HOME_PAGER_AXIS_X) {
            ui_home_pager_emit_drag_begin(pager, pager->target_page);
        }
    }

    if (code == LV_EVENT_PRESSING) {
        if (pager->axis == UI_HOME_PAGER_AXIS_X) {
            pager->last_dx = (lv_coord_t)dx;
            ui_home_pager_apply_track_x(pager,
                                        ui_home_pager_clamp_track_x(
                                            pager,
                                            (lv_coord_t)(pager->track_start_x + dx)));
        }
        return;
    }

    pager->last_dx = (lv_coord_t)dx;
    if (pager->axis == UI_HOME_PAGER_AXIS_X) {
        ui_home_pager_commit_release(pager);
        return;
    }

    if (pager->axis == UI_HOME_PAGER_AXIS_Y && pager->active_page == 0u && pager->vertical_cb != NULL) {
        lv_dir_t dir = ui_home_pager_vertical_dir_from_delta(dy);

        if (dir != LV_DIR_NONE) {
            lv_indev_wait_release(indev);
            pager->vertical_cb(dir, pager->active_page, pager->user);
        }
    }

    pager->axis = UI_HOME_PAGER_AXIS_NONE;
    pager->last_dx = 0;
}

void ui_home_pager_init(ui_home_pager_t *pager, const ui_home_pager_config_t *cfg)
{
    lv_coord_t track_width;

    if (pager == NULL || cfg == NULL || cfg->parent == NULL || cfg->width <= 0 || cfg->height <= 0) {
        return;
    }

    memset(pager, 0, sizeof(*pager));
    pager->page_count = (cfg->page_count > UI_HOME_MAX_TILE_COUNT) ? UI_HOME_MAX_TILE_COUNT : cfg->page_count;
    pager->page_width = cfg->width;
    pager->page_height = cfg->height;
    pager->active_page = ui_home_pager_clamp_page_index(cfg->initial_page, pager->page_count);
    pager->target_page = pager->active_page;
    pager->drag_begin_cb = cfg->drag_begin_cb;
    pager->commit_cb = cfg->commit_cb;
    pager->vertical_cb = cfg->vertical_cb;
    pager->user = cfg->user;

    pager->root = lv_obj_create(cfg->parent);
    lv_obj_remove_style_all(pager->root);
    lv_obj_set_size(pager->root, cfg->width, cfg->height);
    lv_obj_center(pager->root);
    lv_obj_clear_flag(pager->root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(pager->root, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_PRESS_LOCK);
    lv_obj_add_event_cb(pager->root, ui_home_pager_event_cb, LV_EVENT_PRESSED, pager);
    lv_obj_add_event_cb(pager->root, ui_home_pager_event_cb, LV_EVENT_PRESSING, pager);
    lv_obj_add_event_cb(pager->root, ui_home_pager_event_cb, LV_EVENT_RELEASED, pager);
    lv_obj_add_event_cb(pager->root, ui_home_pager_event_cb, LV_EVENT_PRESS_LOST, pager);

    track_width = (lv_coord_t)(cfg->width * ((pager->page_count > 0u) ? pager->page_count : 1u));
    pager->track = lv_obj_create(pager->root);
    lv_obj_remove_style_all(pager->track);
    lv_obj_set_size(pager->track, track_width, cfg->height);
    lv_obj_set_pos(pager->track, ui_home_pager_track_x_for_page(pager, pager->active_page), 0);
    lv_obj_clear_flag(pager->track, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    for (uint8_t i = 0; i < pager->page_count; ++i) {
        pager->pages[i] = lv_obj_create(pager->track);
        lv_obj_remove_style_all(pager->pages[i]);
        lv_obj_set_size(pager->pages[i], cfg->width, cfg->height);
        lv_obj_set_pos(pager->pages[i], (lv_coord_t)(i * cfg->width), 0);
        lv_obj_clear_flag(pager->pages[i], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(pager->pages[i], LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_PRESS_LOCK);
    }
}

void ui_home_pager_deinit(ui_home_pager_t *pager)
{
    if (pager == NULL) {
        return;
    }

    ui_home_pager_stop_settle(pager);
    memset(pager, 0, sizeof(*pager));
}

lv_obj_t *ui_home_pager_root(const ui_home_pager_t *pager)
{
    return (pager != NULL) ? pager->root : NULL;
}

lv_obj_t *ui_home_pager_page(const ui_home_pager_t *pager, uint8_t page_id)
{
    if (pager == NULL || page_id >= pager->page_count) {
        return NULL;
    }

    return pager->pages[page_id];
}

uint8_t ui_home_pager_active_page(const ui_home_pager_t *pager)
{
    return (pager != NULL) ? pager->active_page : 0u;
}

void ui_home_pager_set_active_page(ui_home_pager_t *pager, uint8_t page_id, bool animate)
{
    uint8_t target_page;

    if (pager == NULL) {
        return;
    }

    target_page = ui_home_pager_clamp_page_index(page_id, pager->page_count);
    ui_home_pager_stop_settle(pager);
    pager->axis = UI_HOME_PAGER_AXIS_NONE;
    pager->last_dx = 0;

    if (animate) {
        ui_home_pager_start_settle(pager, target_page);
        return;
    }

    pager->active_page = target_page;
    pager->target_page = target_page;
    ui_home_pager_apply_track_x(pager, ui_home_pager_track_x_for_page(pager, target_page));
}

void ui_home_pager_set_locked(ui_home_pager_t *pager, bool locked)
{
    if (pager == NULL) {
        return;
    }

    pager->locked = locked;
    if (locked) {
        ui_home_pager_stop_settle(pager);
        pager->axis = UI_HOME_PAGER_AXIS_NONE;
        pager->last_dx = 0;
        ui_home_pager_apply_track_x(pager, ui_home_pager_track_x_for_page(pager, pager->active_page));
    }
}

void ui_home_pager_set_page_count(ui_home_pager_t *pager, uint8_t page_count)
{
    lv_coord_t track_width;

    if (pager == NULL) {
        return;
    }

    pager->page_count = (page_count > UI_HOME_MAX_TILE_COUNT) ? UI_HOME_MAX_TILE_COUNT : page_count;
    pager->active_page = ui_home_pager_clamp_page_index(pager->active_page, pager->page_count);
    pager->target_page = pager->active_page;

    if (pager->track == NULL) {
        return;
    }

    track_width = (lv_coord_t)(pager->page_width * ((pager->page_count > 0u) ? pager->page_count : 1u));
    lv_obj_set_size(pager->track, track_width, pager->page_height);
    for (uint8_t i = 0; i < pager->page_count; ++i) {
        if (pager->pages[i] != NULL) {
            lv_obj_set_pos(pager->pages[i], (lv_coord_t)(i * pager->page_width), 0);
        }
    }
    ui_home_pager_apply_track_x(pager, ui_home_pager_track_x_for_page(pager, pager->active_page));
}
