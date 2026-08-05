#ifndef DEBUG_PANEL_H
#define DEBUG_PANEL_H

#include <lvgl/lvgl.h>

#include "model/vehicle_state.h"

typedef struct debug_panel debug_panel_t;

enum {
    DEBUG_WARNING_SEAT_BELT,
    DEBUG_WARNING_HANDBRAKE,
    DEBUG_WARNING_BRAKE,
    DEBUG_WARNING_ENGINE,
    DEBUG_WARNING_COOLANT,
    DEBUG_WARNING_HIGH_BEAM,
    DEBUG_WARNING_LOW_BEAM,
    DEBUG_WARNING_COUNT
};

enum {
    DEBUG_STEERING_UP    = 1U << 0,
    DEBUG_STEERING_DOWN  = 1U << 1,
    DEBUG_STEERING_OK    = 1U << 2,
    DEBUG_STEERING_BACK  = 1U << 3,
    DEBUG_STEERING_LEFT  = 1U << 4,
    DEBUG_STEERING_RIGHT = 1U << 5
};

debug_panel_t *debug_panel_create(void);
void debug_panel_update(debug_panel_t *panel, const vehicle_state_t *state,
                        bool connected);
bool debug_panel_get_warning(const debug_panel_t *panel, int warning);
uint32_t debug_panel_take_steering_actions(debug_panel_t *panel);

#endif
