#include "../../main/app_obd_dsp/ui_gforce_plot_logic.h"

int test_ui_gforce_plot_logic_dummy_symbol(void)
{
    ui_gforce_plot_state_t state;
    size_t active_bins = 0u;
    size_t active_bin = 0u;
    float held_radius = 0.0f;

    ui_gforce_plot_reset(&state);
    if (state.display_lat_g != 0.0f || state.display_lon_g != 0.0f) {
        return 1;
    }

    ui_gforce_plot_step(&state, 2.0f, 0.0f);
    if (!(state.display_lat_g > 0.0f && state.display_lat_g < UI_GFORCE_PLOT_MAX_G)) {
        return 2;
    }

    for (size_t i = 0; i < UI_GFORCE_PLOT_HISTORY_BINS; ++i) {
        if (state.history_radius[i] > 0.0f) {
            active_bin = i;
            ++active_bins;
        }
    }
    if (active_bins == 0u) {
        return 3;
    }
    held_radius = state.history_radius[active_bin];

    ui_gforce_plot_step(&state, 0.0f, 0.0f);
    if (state.display_lat_g != 0.0f || state.display_lon_g != 0.0f) {
        return 4;
    }
    if (state.history_radius[active_bin] != held_radius) {
        return 5;
    }

    if (ui_gforce_plot_angle_to_bin(1.0f, 0.0f, UI_GFORCE_PLOT_HISTORY_BINS) >= UI_GFORCE_PLOT_HISTORY_BINS) {
        return 6;
    }

    if (ui_gforce_plot_clamp(2.0f, -1.0f, 1.0f) != 1.0f) {
        return 7;
    }

    return 0;
}
