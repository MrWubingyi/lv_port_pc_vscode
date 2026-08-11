#include "debug_panel.h"
#include "ui/images.h"
#include "hal/hal.h"

#include <stdio.h>
#include <stdlib.h>

struct debug_panel {
    lv_display_t *display;
    lv_obj_t *screen;
    lv_obj_t *values[8];
    lv_obj_t *icons[8];
    lv_obj_t *buttons[16];
    int button_indices[16];
    int button_count;
    bool states[8];
    bool state_overridden[8];
    uint32_t steering_buttons;
    uint32_t pending_steering_actions;
    bool synced;
};

static const char *state_text(bool value) { return value ? "ON" : "OFF"; }

static void debug_display_delete_event(lv_event_t *event)
{
    debug_panel_t *panel = lv_event_get_user_data(event);
    panel->display = NULL;
    panel->screen = NULL;
    for (int i = 0; i < 8; ++i) {
        panel->values[i] = NULL;
        panel->icons[i] = NULL;
    }
    for (int i = 0; i < 16; ++i) panel->buttons[i] = NULL;
    panel->button_count = 0;
}

static void set_icon_state(lv_obj_t *icon, bool active)
{
    if (icon == NULL) return;
    lv_obj_set_style_img_recolor_opa(icon, active ? 0 : 180, 0);
    lv_obj_set_style_img_opa(icon, active ? LV_OPA_COVER : LV_OPA_50, 0);
}

static void debug_button_event(lv_event_t *event)
{
    debug_panel_t *panel = lv_event_get_user_data(event);
    int index = -99;
    for (int i = 0; i < panel->button_count; ++i) {
        if (panel->buttons[i] == lv_event_get_current_target(event)) {
            index = panel->button_indices[i];
            break;
        }
    }
    if (index == 0) {
        return;
    } else if (index <= -10) {
        uint32_t button = 1U << (unsigned)(-index - 10);
        panel->steering_buttons ^= button;
        panel->pending_steering_actions |= button;
    } else {
        panel->states[index - 1] = !panel->states[index - 1];
        panel->state_overridden[index - 1] = true;
    }
    debug_panel_update(panel, NULL, true);
}

static lv_obj_t *make_button(lv_obj_t *parent, const char *text, int x, int y,
                             int w, int h, debug_panel_t *panel, int index)
{
    lv_obj_t *button = lv_button_create(parent);
    lv_obj_set_pos(button, x, y);
    lv_obj_set_size(button, w, h);
    if (panel->button_count < 16) {
        panel->buttons[panel->button_count] = button;
        panel->button_indices[panel->button_count++] = index;
    }
    lv_obj_add_event_cb(button, debug_button_event, LV_EVENT_CLICKED, panel);
    lv_obj_t *label = lv_label_create(button);
    lv_label_set_text(label, text);
    lv_obj_center(label);
    return button;
}

debug_panel_t *debug_panel_create(void)
{
    debug_panel_t *panel = calloc(1, sizeof(*panel));
    if (panel == NULL) return NULL;
    panel->display = sdl_hal_add_display(560, 420, "Dashboard Debug Panel");
    if (panel->display == NULL) {
        free(panel);
        return NULL;
    }
    lv_display_add_event_cb(panel->display, debug_display_delete_event,
                            LV_EVENT_DELETE, panel);

    panel->screen = lv_display_get_screen_active(panel->display);
    lv_obj_set_size(panel->screen, 560, 420);
    lv_obj_set_style_bg_color(panel->screen, lv_color_hex(0x0B1120), 0);
    lv_obj_set_style_bg_opa(panel->screen, LV_OPA_COVER, 0);

    lv_obj_t *title = lv_label_create(panel->screen);
    lv_label_set_text(title, "DEBUG PANEL");
    lv_obj_set_pos(title, 20, 18);
    lv_obj_set_style_text_color(title, lv_color_hex(0x59D6EA), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_t *content = panel->screen;

    static const char *names[] = {
        "Seat belt", "Handbrake", "Brake warning", "Engine warning",
        "Coolant warning", "High beam", "Low beam"
    };
    static const lv_img_dsc_t *icon_sources[] = {
        &img_icon_seat_belt_16, &img_icon_brake_warning_16,
        &img_icon_brake_warning_16, &img_icon_check_engine_16,
        &img_icon_coolant_16, &img_icon_high_beam_16, &img_icon_high_beam_16
    };
    for (int i = 0; i < 7; ++i) {
        lv_obj_t *name = lv_label_create(content);
        lv_label_set_text(name, names[i]);
        lv_obj_set_style_text_color(name, lv_color_hex(0xAAB5C9), 0);
        lv_obj_set_pos(name, 42, 72 + i * 42);
        lv_obj_t *value_button = make_button(content, "OFF", 240,
                                             64 + i * 42, 100, 32, panel,
                                             i + 1);
        panel->values[i] = lv_obj_get_child(value_button, 0);
        panel->icons[i] = lv_image_create(value_button);
        lv_image_set_src(panel->icons[i], icon_sources[i]);
        lv_obj_set_pos(panel->icons[i], 9, 8);
        lv_obj_set_pos(panel->values[i], 30, 7);
    }
    lv_obj_t *wheel = lv_label_create(content);
    lv_label_set_text(wheel, "Steering wheel buttons");
    lv_obj_set_style_text_color(wheel, lv_color_hex(0xAAB5C9), 0);
    lv_obj_set_pos(wheel, 380, 72);
    static const char *wheel_names[] = {
        "UP", "DOWN", "OK", "BACK", "LEFT", "RIGHT"
    };
    for (int i = 0; i < 6; ++i)
        make_button(content, wheel_names[i], 380 + (i % 2) * 82,
                    112 + (i / 2) * 48, 72, 34, panel, -10 - i);
    return panel;
}

bool debug_panel_get_warning(const debug_panel_t *panel, int warning)
{
    return panel != NULL && warning >= 0 && warning < DEBUG_WARNING_COUNT &&
           panel->states[warning];
}

uint32_t debug_panel_take_steering_actions(debug_panel_t *panel)
{
    if (panel == NULL) return 0;
    uint32_t actions = panel->pending_steering_actions;
    panel->pending_steering_actions = 0;
    return actions;
}

void debug_panel_update(debug_panel_t *panel, const vehicle_state_t *state,
                        bool connected)
{
    if (panel == NULL || panel->display == NULL) return;
    if (state != NULL && connected) {
        if (!panel->synced) {
            panel->steering_buttons = state->steering_buttons;
            panel->synced = true;
        }
        const bool incoming[7] = {
            state->seatbelt_warning,
            state->handbrake,
            state->braking_warning,
            state->engine_warning,
            state->coolant_warning,
            state->high_beam,
            state->low_beam
        };
        for (int i = 0; i < 7; ++i) {
            if (!panel->state_overridden[i]) panel->states[i] = incoming[i];
        }
    } else if (!connected) {
        for (int i = 0; i < 7; ++i) {
            if (!panel->state_overridden[i]) panel->states[i] = false;
        }
    }
    for (int i = 0; i < 7; ++i) {
        lv_label_set_text(panel->values[i], state_text(panel->states[i]));
        set_icon_state(panel->icons[i], panel->states[i]);
    }
}
