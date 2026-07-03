#include "../../main/export_path/ui_home_pager.h"

int test_ui_home_pager_logic_dummy_symbol(void)
{
    if (ui_home_pager_axis_from_delta(12, 0, false) != UI_HOME_PAGER_AXIS_X) {
        return 1;
    }
    if (ui_home_pager_axis_from_delta(11, 0, false) != UI_HOME_PAGER_AXIS_NONE) {
        return 2;
    }
    if (ui_home_pager_axis_from_delta(12, 9, true) != UI_HOME_PAGER_AXIS_NONE) {
        return 3;
    }
    if (ui_home_pager_axis_from_delta(6, -12, true) != UI_HOME_PAGER_AXIS_Y) {
        return 4;
    }
    if (ui_home_pager_vertical_dir_from_delta(-20) != LV_DIR_TOP) {
        return 5;
    }
    if (ui_home_pager_vertical_dir_from_delta(19) != LV_DIR_NONE) {
        return 6;
    }
    if (ui_home_pager_target_page_from_release(-160, 320, -80, 1u, 4u) != 2u) {
        return 7;
    }
    if (ui_home_pager_target_page_from_release(-160, 320, 80, 1u, 4u) != 0u) {
        return 8;
    }
    if (ui_home_pager_target_page_from_release(-160, 320, 40, 1u, 4u) != 1u) {
        return 9;
    }
    if (ui_home_pager_target_page_from_release(0, 320, 200, 0u, 4u) != 0u) {
        return 10;
    }
    if (ui_home_pager_target_page_from_release(-960, 320, -200, 3u, 4u) != 3u) {
        return 11;
    }

    return 0;
}
