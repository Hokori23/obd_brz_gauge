// Info Page - OBD Extra Data
// Layout (360×360 circle):
//   Row 1: [CLT °C] | [OIL °C]
//   Row 2: [LOAD %] | [TPS %]
//   Row 3 (bottom center): [IAT °C]

#include "../ui.h"
#include "../ui_font_profile.h"
#include "../ui_layout.h"
#include "../ui_round_shell.h"

lv_obj_t *ui_ScreenPageInfo  = NULL;
lv_obj_t *ui_LabelInfoCLT    = NULL;
lv_obj_t *ui_LabelInfoIAT    = NULL;
lv_obj_t *ui_LabelInfoLoad   = NULL;
lv_obj_t *ui_LabelInfoTPS    = NULL;
lv_obj_t *ui_LabelInfoOil    = NULL;  // 机油温度 °C (SSM 22 10 17)
lv_obj_t *ui_LabelInfoValue[5] = {NULL, NULL, NULL, NULL, NULL};
lv_obj_t *ui_LabelInfoName[5]  = {NULL, NULL, NULL, NULL, NULL};
lv_obj_t *ui_LabelInfoUnit[5]  = {NULL, NULL, NULL, NULL, NULL};

// Helper: create one data tile (name + value + unit) at a given offset from center
// Tile internal layout (relative to cy):
//   name  center: cy - 28  (font Size16, ~16px tall)
//   value center: cy + 2   (font Size36, ~36px tall)  gap between name bottom and value top ≈ 6px
//   unit  center: cy + 34  (font Size16, ~16px tall)  gap between value bottom and unit top ≈ 8px
// Total tile height ≈ 80px
static lv_obj_t *create_info_tile(lv_obj_t *parent,
                                   const ui_info_layout_t *layout,
                                   lv_obj_t **name_out,
                                   lv_obj_t **unit_out,
                                   const char *name, lv_color_t name_color,
                                   const char *unit,
                                   lv_coord_t cx, lv_coord_t cy)
{
    // Name label
    lv_obj_t *lbl_name = lv_label_create(parent);
    lv_label_set_text(lbl_name, name);
    lv_obj_set_style_text_font(lbl_name, ui_font_typoder(16), LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl_name, name_color, LV_PART_MAIN);
    lv_obj_align(lbl_name, LV_ALIGN_CENTER, cx, cy + layout->tile_name_dy);
    if (name_out) *name_out = lbl_name;

    // Value label (returned so caller can update it)
    lv_obj_t *lbl_val = lv_label_create(parent);
    lv_label_set_text(lbl_val, "--");
    lv_obj_set_style_text_font(lbl_val, ui_font_typoder(36), LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl_val, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_width(lbl_val, layout->tile_value_width);
    lv_obj_set_style_text_align(lbl_val, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(lbl_val, LV_ALIGN_CENTER, cx, cy + layout->tile_value_dy);

    // Unit label
    lv_obj_t *lbl_unit = lv_label_create(parent);
    lv_label_set_text(lbl_unit, unit);
    lv_obj_set_style_text_font(lbl_unit, ui_font_typoder(16), LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl_unit, lv_color_hex(0x888888), LV_PART_MAIN);
    lv_obj_align(lbl_unit, LV_ALIGN_CENTER, cx, cy + layout->tile_unit_dy);
    if (unit_out) *unit_out = lbl_unit;

    return lbl_val;
}

// Helper: horizontal divider  (y = offset from screen center, places center of 1px line)
static void create_hdiv(lv_obj_t *parent, lv_coord_t y, lv_coord_t w)
{
    lv_obj_t *div = lv_obj_create(parent);
    lv_obj_remove_style_all(div);
    lv_obj_set_width(div, w);
    lv_obj_set_height(div, 1);
    lv_obj_align(div, LV_ALIGN_CENTER, 0, y);   // center of div at (parent_cx, parent_cy+y)
    lv_obj_set_style_bg_color(div, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(div, 255, LV_PART_MAIN);
    lv_obj_clear_flag(div, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
}

// Helper: vertical divider
static void create_vdiv(lv_obj_t *parent, lv_coord_t x, lv_coord_t y, lv_coord_t h)
{
    lv_obj_t *div = lv_obj_create(parent);
    lv_obj_remove_style_all(div);
    lv_obj_set_width(div, 1);
    lv_obj_set_height(div, h);
    lv_obj_align(div, LV_ALIGN_CENTER, x, y);
    lv_obj_set_style_bg_color(div, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(div, 255, LV_PART_MAIN);
    lv_obj_clear_flag(div, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
}

lv_obj_t *ui_page_info_content_create(lv_obj_t *parent)
{
    ui_info_layout_t layout;
    ui_info_layout_get(&layout);

    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "INFO");
    lv_obj_set_style_text_font(title, ui_font_typoder(20), LV_PART_MAIN);
    lv_obj_set_style_text_color(title, lv_color_hex(0x666666), LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, layout.title_y);

    create_hdiv(parent, layout.hdiv1_y, layout.hdiv1_width);
    create_hdiv(parent, layout.hdiv2_y, layout.hdiv2_width);
    create_vdiv(parent, layout.vdiv_x, layout.vdiv_y, layout.vdiv_height);

    ui_LabelInfoCLT = create_info_tile(parent, &layout, &ui_LabelInfoName[0], &ui_LabelInfoUnit[0], "CLT", lv_color_hex(0x44AAFF), "掳C", layout.left_col_x, layout.row1_cy);
    ui_LabelInfoOil = create_info_tile(parent, &layout, &ui_LabelInfoName[1], &ui_LabelInfoUnit[1], "OIL", lv_color_hex(0xFF7722), "掳C", layout.right_col_x, layout.row1_cy);
    ui_LabelInfoLoad = create_info_tile(parent, &layout, &ui_LabelInfoName[2], &ui_LabelInfoUnit[2], "LOAD", lv_color_hex(0xFFCC00), "%", layout.left_col_x, layout.row2_cy);
    ui_LabelInfoTPS = create_info_tile(parent, &layout, &ui_LabelInfoName[3], &ui_LabelInfoUnit[3], "TPS", lv_color_hex(0xFF8844), "%", layout.right_col_x, layout.row2_cy);
    ui_LabelInfoIAT = create_info_tile(parent, &layout, &ui_LabelInfoName[4], &ui_LabelInfoUnit[4], "IAT", lv_color_hex(0x44FF88), "掳C", layout.center_col_x, layout.row3_cy);

    ui_LabelInfoValue[0] = ui_LabelInfoCLT;
    ui_LabelInfoValue[1] = ui_LabelInfoOil;
    ui_LabelInfoValue[2] = ui_LabelInfoLoad;
    ui_LabelInfoValue[3] = ui_LabelInfoTPS;
    ui_LabelInfoValue[4] = ui_LabelInfoIAT;
    return parent;
}

void ui_ScreenPageInfo_screen_init(void)
{
    ui_info_layout_t layout;
    ui_info_layout_get(&layout);

    ui_ScreenPageInfo = lv_obj_create(NULL);
    ui_round_screen_apply_base(ui_ScreenPageInfo, lv_color_hex(0x000000));
    ui_round_shell_create_ring(ui_ScreenPageInfo, &layout.shell);

    // Page title
    lv_obj_t *title = lv_label_create(ui_ScreenPageInfo);
    lv_label_set_text(title, "INFO");
    lv_obj_set_style_text_font(title, ui_font_typoder(20), LV_PART_MAIN);
    lv_obj_set_style_text_color(title, lv_color_hex(0x666666), LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, layout.title_y);

    // Grid dividers
    // Row1 cy=-84: unit bottom at cy+34+8 = -42 → pixel 138
    // Hdiv at y=-36 → pixel 144 (6px gap below row1 unit)
    create_hdiv(ui_ScreenPageInfo, layout.hdiv1_y, layout.hdiv1_width);
    // Row2 cy=+22: name top at cy-28-8 = -14 → pixel 166
    // Hdiv at y=-36 → pixel 144, gap to row2 name = 22px ✓
    // Row2 bottom: unit at cy+34+8 = +64 → pixel 244
    // Second hdiv at y=+68 → pixel 248 (4px gap below row2 unit)
    create_hdiv(ui_ScreenPageInfo, layout.hdiv2_y, layout.hdiv2_width);
    // Vertical divider between left and right cols, spans rows 1+2
    // center_y = (-84+22)/2 = -31, height ≈ 196px
    create_vdiv(ui_ScreenPageInfo, layout.vdiv_x, layout.vdiv_y, layout.vdiv_height);

    // ── Row 1 ──  cy = -84  (name pixel≈68, value≈96, unit≈126)
    // CLT: left column cx=-82
    ui_LabelInfoCLT  = create_info_tile(ui_ScreenPageInfo,
                                        &layout,
                                        &ui_LabelInfoName[0], &ui_LabelInfoUnit[0],
                                        "CLT", lv_color_hex(0x44AAFF), "°C",
                                        layout.left_col_x, layout.row1_cy);
    // OIL: right column cx=+82
    ui_LabelInfoOil  = create_info_tile(ui_ScreenPageInfo,
                                        &layout,
                                        &ui_LabelInfoName[1], &ui_LabelInfoUnit[1],
                                        "OIL", lv_color_hex(0xFF7722), "°C",
                                        layout.right_col_x, layout.row1_cy);

    // ── Row 2 ──  cy = +22  (name pixel≈174, value≈202, unit≈232)
    // LOAD: left column
    ui_LabelInfoLoad = create_info_tile(ui_ScreenPageInfo,
                                        &layout,
                                        &ui_LabelInfoName[2], &ui_LabelInfoUnit[2],
                                        "LOAD", lv_color_hex(0xFFCC00), "%",
                                        layout.left_col_x, layout.row2_cy);
    // TPS: right column
    ui_LabelInfoTPS  = create_info_tile(ui_ScreenPageInfo,
                                        &layout,
                                        &ui_LabelInfoName[3], &ui_LabelInfoUnit[3],
                                        "TPS", lv_color_hex(0xFF8844), "%",
                                        layout.right_col_x, layout.row2_cy);

    // ── Row 3: IAT centered ──  cy = +112
    ui_LabelInfoIAT  = create_info_tile(ui_ScreenPageInfo,
                                        &layout,
                                        &ui_LabelInfoName[4], &ui_LabelInfoUnit[4],
                                        "IAT", lv_color_hex(0x44FF88), "°C",
                                        layout.center_col_x, layout.row3_cy);

    ui_LabelInfoValue[0] = ui_LabelInfoCLT;
    ui_LabelInfoValue[1] = ui_LabelInfoOil;
    ui_LabelInfoValue[2] = ui_LabelInfoLoad;
    ui_LabelInfoValue[3] = ui_LabelInfoTPS;
    ui_LabelInfoValue[4] = ui_LabelInfoIAT;

    // Touch events
    lv_obj_add_event_cb(ui_ScreenPageInfo, ui_event_info_background, LV_EVENT_GESTURE, NULL);
}
