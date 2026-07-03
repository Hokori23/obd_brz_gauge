#include "ui_gforce_plot_logic.h"

#include <math.h>
#include <string.h>

#define UI_GFORCE_PLOT_DEADZONE_G 0.015f
#define UI_GFORCE_PLOT_PI_F 3.14159265f

static float ui_gforce_plot_absf(float value)
{
    return (value < 0.0f) ? -value : value;
}

void ui_gforce_plot_reset(ui_gforce_plot_state_t *state)
{
    if (state == NULL) {
        return;
    }

    memset(state, 0, sizeof(*state));
}

float ui_gforce_plot_clamp(float value, float min_value, float max_value)
{
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}

size_t ui_gforce_plot_angle_to_bin(float lat_g, float lon_g, size_t bin_count)
{
    float angle;
    float normalized;
    size_t index;

    if (bin_count == 0u) {
        return 0u;
    }

    angle = atan2f(lon_g, lat_g);
    if (angle < 0.0f) {
        angle += (2.0f * UI_GFORCE_PLOT_PI_F);
    }
    normalized = angle / (2.0f * UI_GFORCE_PLOT_PI_F);
    index = (size_t)(normalized * (float)bin_count);
    if (index >= bin_count) {
        index = bin_count - 1u;
    }
    return index;
}

void ui_gforce_plot_step(ui_gforce_plot_state_t *state, float lat_g, float lon_g)
{
    float lat_target;
    float lon_target;
    float delta_lat;
    float delta_lon;
    float delta_mag;
    float follow = 0.14f;
    float magnitude;
    float normalized_radius;
    size_t bin_index;

    if (state == NULL) {
        return;
    }

    lat_target = ui_gforce_plot_clamp(lat_g, -UI_GFORCE_PLOT_MAX_G, UI_GFORCE_PLOT_MAX_G);
    lon_target = ui_gforce_plot_clamp(lon_g, -UI_GFORCE_PLOT_MAX_G, UI_GFORCE_PLOT_MAX_G);

    if (ui_gforce_plot_absf(lat_target) < UI_GFORCE_PLOT_DEADZONE_G) {
        lat_target = 0.0f;
    }
    if (ui_gforce_plot_absf(lon_target) < UI_GFORCE_PLOT_DEADZONE_G) {
        lon_target = 0.0f;
    }

    if ((lat_target == 0.0f) && (lon_target == 0.0f)) {
        state->display_lat_g = 0.0f;
        state->display_lon_g = 0.0f;
    } else {
        delta_lat = lat_target - state->display_lat_g;
        delta_lon = lon_target - state->display_lon_g;
        delta_mag = sqrtf((delta_lat * delta_lat) + (delta_lon * delta_lon));

        if (delta_mag > 0.40f) {
            follow = 0.34f;
        } else if (delta_mag > 0.20f) {
            follow = 0.24f;
        }

        state->display_lat_g = ui_gforce_plot_clamp(state->display_lat_g + (delta_lat * follow),
                                                    -UI_GFORCE_PLOT_MAX_G,
                                                    UI_GFORCE_PLOT_MAX_G);
        state->display_lon_g = ui_gforce_plot_clamp(state->display_lon_g + (delta_lon * follow),
                                                    -UI_GFORCE_PLOT_MAX_G,
                                                    UI_GFORCE_PLOT_MAX_G);
    }

    magnitude = sqrtf((lat_target * lat_target) + (lon_target * lon_target));
    normalized_radius = ui_gforce_plot_clamp(magnitude / UI_GFORCE_PLOT_MAX_G, 0.0f, 1.0f);
    if (normalized_radius <= 0.01f) {
        return;
    }

    bin_index = ui_gforce_plot_angle_to_bin(lat_target,
                                            lon_target,
                                            UI_GFORCE_PLOT_HISTORY_BINS);
    if (state->history_radius[bin_index] < normalized_radius) {
        state->history_radius[bin_index] = normalized_radius;
    }
}

