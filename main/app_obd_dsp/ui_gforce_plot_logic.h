#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UI_GFORCE_PLOT_HISTORY_BINS 48u
#define UI_GFORCE_PLOT_MAX_G 1.20f

typedef struct {
    float display_lat_g;
    float display_lon_g;
    float history_radius[UI_GFORCE_PLOT_HISTORY_BINS];
} ui_gforce_plot_state_t;

void ui_gforce_plot_reset(ui_gforce_plot_state_t *state);
float ui_gforce_plot_clamp(float value, float min_value, float max_value);
size_t ui_gforce_plot_angle_to_bin(float lat_g, float lon_g, size_t bin_count);
void ui_gforce_plot_step(ui_gforce_plot_state_t *state, float lat_g, float lon_g);

#ifdef __cplusplus
}
#endif

