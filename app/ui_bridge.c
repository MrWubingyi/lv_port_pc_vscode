#include "ui_bridge.h"

#include "screens.h"
#include "vehicle_data.h"
#include "debug_panel.h"

#include <stdio.h>
#include <time.h>

#define UI_REFRESH_PERIOD_MS 100
static lv_point_precise_t left_active_points[2];
static lv_point_precise_t right_active_points[2];
#define LEFT_TICK_COUNT 17
#define RIGHT_TICK_COUNT 21
static lv_obj_t *left_ticks[LEFT_TICK_COUNT];
static lv_obj_t *right_ticks[RIGHT_TICK_COUNT];
static lv_point_precise_t tick_points[LEFT_TICK_COUNT + RIGHT_TICK_COUNT][2];
static int left_tick_state[LEFT_TICK_COUNT];
static int right_tick_state[RIGHT_TICK_COUNT];
static vehicle_data_t *dashboard_vehicle_data;
static debug_panel_t *dashboard_debug_panel;
static lv_obj_t *warning_icons[6];
static lv_obj_t *menu_items[6];
static int selected_menu_item;
static lv_timer_t *refresh_timer;

static void update_warning_icons(void) {
    for (int i = 0; i < 6; ++i) {
        bool active = debug_panel_get_warning(dashboard_debug_panel, i);
        if (warning_icons[i] != NULL)
            lv_obj_set_style_opa(warning_icons[i],
                                 active ? LV_OPA_COVER : LV_OPA_20,
                                 LV_PART_MAIN);
    }
}

static void select_menu_item(int index) {
    const int count = (int)(sizeof(menu_items) / sizeof(menu_items[0]));
    index = (index + count) % count;
    for (int i = 0; i < count; ++i) {
        if (menu_items[i] == NULL) continue;
        if (i == index)
            lv_obj_add_state(menu_items[i], LV_STATE_CHECKED);
        else
            lv_obj_remove_state(menu_items[i], LV_STATE_CHECKED);
    }
    selected_menu_item = index;
}

static void update_menu_navigation(void) {
    uint32_t actions =
        debug_panel_take_steering_actions(dashboard_debug_panel);
    if (actions & DEBUG_STEERING_LEFT)
        select_menu_item(selected_menu_item - 1);
    if (actions & DEBUG_STEERING_RIGHT)
        select_menu_item(selected_menu_item + 1);
}

static void animate_tick_opa(void *obj, int32_t value) {
    lv_obj_set_style_line_opa((lv_obj_t *)obj, (uint8_t)value, LV_PART_MAIN);
}

static void set_tick_highlight(lv_obj_t *obj, int highlighted, int *old_state) {
    if (obj == NULL || *old_state == highlighted) return;
    *old_state = highlighted;
    lv_obj_set_style_line_color(obj, lv_color_hex(highlighted ? 0xffffff : 0x8790a7), LV_PART_MAIN);
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, obj);
    lv_anim_set_exec_cb(&anim, animate_tick_opa);
    lv_anim_set_values(&anim, highlighted ? 80 : 255, highlighted ? 255 : 80);
    lv_anim_set_duration(&anim, 260);
    lv_anim_start(&anim);
}

static void create_scale_ticks(void) {
    for (int i = 0; i < LEFT_TICK_COUNT; ++i) {
        tick_points[i][0] = (lv_point_precise_t){0, 0};
        tick_points[i][1] = (lv_point_precise_t){9, 0};
        left_ticks[i] = lv_line_create(objects.panel_left_gauge);
        lv_line_set_points(left_ticks[i], tick_points[i], 2);
        lv_obj_set_pos(left_ticks[i], 68 - i * 39 / 16, 270 - i * 180 / 16);
        lv_obj_set_style_line_width(left_ticks[i], 1, LV_PART_MAIN);
        left_tick_state[i] = -1;
    }
    for (int i = 0; i < RIGHT_TICK_COUNT; ++i) {
        int point_index = LEFT_TICK_COUNT + i;
        tick_points[point_index][0] = (lv_point_precise_t){0, 0};
        tick_points[point_index][1] = (lv_point_precise_t){9, 0};
        right_ticks[i] = lv_line_create(objects.panel_right_gauge);
        lv_line_set_points(right_ticks[i], tick_points[point_index], 2);
        lv_obj_set_pos(right_ticks[i], 17 + i * 40 / 20, 270 - i * 180 / 20);
        lv_obj_set_style_line_width(right_ticks[i], 1, LV_PART_MAIN);
        right_tick_state[i] = -1;
    }
}

static void update_scale_ticks(int speed, int rpm) {
    int left_index = (speed + 2) / 5;
    if (left_index >= LEFT_TICK_COUNT) left_index = LEFT_TICK_COUNT - 1;
    int right_index = (rpm + 50) / 100;
    if (right_index >= RIGHT_TICK_COUNT) right_index = RIGHT_TICK_COUNT - 1;
    for (int i = 0; i < LEFT_TICK_COUNT; ++i)
        set_tick_highlight(left_ticks[i], i >= left_index - 4 && i <= left_index + 4, &left_tick_state[i]);
    for (int i = 0; i < RIGHT_TICK_COUNT; ++i)
        set_tick_highlight(right_ticks[i], i >= right_index - 4 && i <= right_index + 4, &right_tick_state[i]);
}

static void update_clock(void) {
    time_t now = time(NULL);
    struct tm local_time;

    if (localtime_r(&now, &local_time) != NULL) {
        lv_label_set_text_fmt(objects.lbl_time, "%02d:%02d",
                              local_time.tm_hour, local_time.tm_min);
    }
}

static void set_connection_state(bool connected) {
    const char *title = "Waiting for vehicle";
    uint32_t color = 0xFF9F43;
    if (connected) {
        title = "Vehicle online";
        color = 0x59D6EA;
    }
    lv_label_set_text(objects.lbl_status_title, title);
    lv_obj_set_style_text_color(
        objects.lbl_status_title, lv_color_hex(color), LV_PART_MAIN);
}

static void update_active_lines(int speed, int rpm) {
    int speed_value = speed > 80 ? 80 : speed;
    int rpm_value = rpm > 2000 ? 2000 : rpm;

    /* The generated gauge places 0..80 on y=270..90, not on the
       complete 280 px backbone. Keep the active needle on that scale. */
    left_active_points[0].x = 68 - speed_value * 39 / 80;
    left_active_points[0].y = 270 - speed_value * 180 / 80;
    left_active_points[1].x = 70;
    left_active_points[1].y = 280;
    /* Right ticks use 0..2000 RPM over the same y=270..90 interval. */
    /* Tick marks extend left from the backbone; use their intersection with
       line_right_main (x=25..66) so the active segment keeps the backbone's
       angle. */
    right_active_points[0].x = 25 + rpm_value * 41 / 2000;
    right_active_points[0].y = 270 - rpm_value * 180 / 2000;
    right_active_points[1].x = 25;
    right_active_points[1].y = 280;

    lv_line_set_points(objects.line_left_active, left_active_points, 2);
    lv_line_set_points(objects.line_rigth_active, right_active_points, 2);
}

static void refresh_cb(lv_timer_t *timer) {
    LV_UNUSED(timer);

    if (dashboard_vehicle_data == NULL) return;

    vehicle_state_t state;
    uint64_t last_receive_ms;
    bool valid = vehicle_data_get_snapshot(
        dashboard_vehicle_data, &state, &last_receive_ms);
    (void)last_receive_ms;

    update_clock();
    set_connection_state(valid);
    debug_panel_update(dashboard_debug_panel, &state, valid);
    update_warning_icons();
    update_menu_navigation();
    if (!valid) {
        if (objects.lbl_speed != NULL) lv_label_set_text(objects.lbl_speed, "0");
        if (objects.lbl_gear != NULL) lv_label_set_text(objects.lbl_gear, "-");
        if (objects.bar_fuel != NULL) {
            lv_bar_set_value(objects.bar_fuel, 0, LV_ANIM_OFF);
            lv_obj_set_style_bg_color(objects.bar_fuel, lv_color_hex(0x59D6EA), LV_PART_INDICATOR);
        }
        if (objects.img_fuel != NULL) {
            lv_obj_set_style_opa(objects.img_fuel, LV_OPA_COVER, LV_PART_MAIN);
            lv_obj_set_style_image_recolor_opa(objects.img_fuel, LV_OPA_TRANSP, LV_PART_MAIN);
        }
        if (objects.lbl_range != NULL) lv_label_set_text(objects.lbl_range, "0 km");
        if (objects.lbl_load_current != NULL) lv_label_set_text(objects.lbl_load_current, "0.0");
        if (objects.lbl_load_max != NULL) lv_label_set_text(objects.lbl_load_max, "0.0");
        if (objects.bar_load != NULL) lv_bar_set_value(objects.bar_load, 0, LV_ANIM_OFF);
        if (objects.lbl_outside_temperature != NULL) lv_label_set_text(objects.lbl_outside_temperature, "0 °C");
        if (objects.lbl_trip_distance != NULL) lv_label_set_text(objects.lbl_trip_distance, "T1 0.0 km");

        for (int i = 0; i < 6; ++i) {
            if (warning_icons[i] != NULL) {
                lv_obj_set_style_opa(warning_icons[i], LV_OPA_20, LV_PART_MAIN);
                lv_obj_set_style_image_recolor_opa(warning_icons[i], LV_OPA_TRANSP, LV_PART_MAIN);
            }
        }

        if (objects.turn_left != NULL) lv_obj_set_style_opa(objects.turn_left, LV_OPA_20, LV_PART_MAIN);
        if (objects.turn_right != NULL) lv_obj_set_style_opa(objects.turn_right, LV_OPA_20, LV_PART_MAIN);
        if (objects.door_unlocked != NULL) lv_obj_set_style_opa(objects.door_unlocked, LV_OPA_20, LV_PART_MAIN);

        update_active_lines(0, 0);
        update_scale_ticks(0, 0);
        return;
    }

    int speed = state.speed_kph < 0 ? 0 : state.speed_kph;
    int soc = state.soc < 0 ? 0 : (state.soc > 100 ? 100 : state.soc);
    int load_max = state.load_max_tenths > 0 ? state.load_max_tenths : 1;
    int load_percent = state.load_tenths * 100 / load_max;
    if (load_percent < 0) load_percent = 0;
    if (load_percent > 100) load_percent = 100;

    static const char gear_chars[] = {'P', 'R', 'N', 'D'};
    char gear_str[2] = {(state.gear >= 0 && state.gear <= 3) ? gear_chars[state.gear] : '-', '\0'};

    lv_label_set_text_fmt(objects.lbl_speed, "%d", speed);
    lv_label_set_text(objects.lbl_gear, gear_str);
    lv_bar_set_value(objects.bar_fuel, soc, LV_ANIM_ON);
    lv_label_set_text_fmt(objects.lbl_range, "%d km", state.range_km);
    lv_label_set_text_fmt(objects.lbl_load_current, "%d.%d",
                          state.load_tenths / 10, state.load_tenths % 10);
    lv_label_set_text_fmt(objects.lbl_load_max, "%d.%d",
                          state.load_max_tenths / 10,
                          state.load_max_tenths % 10);
    lv_bar_set_value(objects.bar_load, load_percent, LV_ANIM_ON);
    lv_label_set_text_fmt(objects.lbl_outside_temperature, "%d °C",
                          state.outside_temp_c);
    lv_label_set_text_fmt(objects.lbl_trip_distance, "T1 %d.%d km",
                          state.trip_tenths / 10, state.trip_tenths % 10);

    /* Seat belt warning logic */
    if (warning_icons[0] != NULL) {
        bool belt_warn = debug_panel_get_warning(dashboard_debug_panel, 0) || state.seatbelt_warning;
        lv_obj_set_style_opa(warning_icons[0],
                             belt_warn ? LV_OPA_COVER : LV_OPA_20,
                             LV_PART_MAIN);
        if (belt_warn) {
            lv_obj_set_style_image_recolor_opa(warning_icons[0], LV_OPA_COVER, LV_PART_MAIN);
            lv_obj_set_style_image_recolor(warning_icons[0], lv_color_hex(0xFF3B30), LV_PART_MAIN);
        } else {
            lv_obj_set_style_image_recolor_opa(warning_icons[0], LV_OPA_TRANSP, LV_PART_MAIN);
        }
    }

    /* High beam light indicator logic */
    if (warning_icons[5] != NULL) {
        lv_obj_set_style_opa(warning_icons[5],
                             state.high_beam ? LV_OPA_COVER : LV_OPA_20,
                             LV_PART_MAIN);
        if (state.high_beam) {
            lv_obj_set_style_image_recolor_opa(warning_icons[5], LV_OPA_COVER, LV_PART_MAIN);
            lv_obj_set_style_image_recolor(warning_icons[5], lv_color_hex(0x2094FA), LV_PART_MAIN);
        } else {
            lv_obj_set_style_image_recolor_opa(warning_icons[5], LV_OPA_TRANSP, LV_PART_MAIN);
        }
    }

    /* Coolant / Engine temp warning logic */
    if (warning_icons[4] != NULL) {
        bool coolant_warn = state.coolant_warning || (state.engine_coolant_temp > 105.0);
        lv_obj_set_style_opa(warning_icons[4],
                             coolant_warn ? LV_OPA_COVER : LV_OPA_20,
                             LV_PART_MAIN);
        if (coolant_warn) {
            lv_obj_set_style_image_recolor_opa(warning_icons[4], LV_OPA_COVER, LV_PART_MAIN);
            lv_obj_set_style_image_recolor(warning_icons[4], lv_color_hex(0xFF3B30), LV_PART_MAIN);
        } else {
            lv_obj_set_style_image_recolor_opa(warning_icons[4], LV_OPA_TRANSP, LV_PART_MAIN);
        }
    }

    /* EV Battery / SOC warning logic (SOC <= 15%) */
    bool bat_low = state.battery_warning || (soc <= 15);
    if (objects.bar_fuel != NULL) {
        uint32_t fuel_color = bat_low ? 0xFF3B30 : 0x59D6EA;
        lv_obj_set_style_bg_color(objects.bar_fuel, lv_color_hex(fuel_color), LV_PART_INDICATOR);
    }
    if (objects.img_fuel != NULL) {
        lv_obj_set_style_opa(objects.img_fuel, bat_low ? LV_OPA_COVER : LV_OPA_COVER, LV_PART_MAIN);
        if (bat_low) {
            lv_obj_set_style_image_recolor_opa(objects.img_fuel, LV_OPA_COVER, LV_PART_MAIN);
            lv_obj_set_style_image_recolor(objects.img_fuel, lv_color_hex(0xFF3B30), LV_PART_MAIN);
        } else {
            lv_obj_set_style_image_recolor_opa(objects.img_fuel, LV_OPA_TRANSP, LV_PART_MAIN);
        }
    }

    /* Door lock logic: door_lock == false means unlocked (show warning icon) */
    if (objects.door_unlocked != NULL) {
        lv_obj_set_style_opa(objects.door_unlocked,
                             state.door_lock ? LV_OPA_20 : LV_OPA_COVER,
                             LV_PART_MAIN);
    }

    /* Turn signal logic: 0: NONE, 1: LEFT, 2: RIGHT, 3: HAZARD */
    static uint32_t turn_blink_count = 0;
    turn_blink_count++;
    bool blink_on = (turn_blink_count / 5) % 2 == 0;
    bool left_active = (state.turn_signal == 1 || state.turn_signal == 3);
    bool right_active = (state.turn_signal == 2 || state.turn_signal == 3);

    if (objects.turn_left != NULL) {
        uint8_t left_opa = (left_active && blink_on) ? LV_OPA_COVER : LV_OPA_20;
        lv_obj_set_style_opa(objects.turn_left, left_opa, LV_PART_MAIN);
    }
    if (objects.turn_right != NULL) {
        uint8_t right_opa = (right_active && blink_on) ? LV_OPA_COVER : LV_OPA_20;
        lv_obj_set_style_opa(objects.turn_right, right_opa, LV_PART_MAIN);
    }

    update_active_lines(speed, state.rpm < 0 ? 0 : state.rpm);
    update_scale_ticks(speed, state.rpm < 0 ? 0 : state.rpm);
}

void ui_bridge_init(vehicle_data_t *vehicle_data) {
    dashboard_vehicle_data = vehicle_data;
    update_clock();
    set_connection_state(false);
    create_scale_ticks();
    warning_icons[0] = lv_obj_get_child(objects.panel_left_warning, 0);
    warning_icons[1] = lv_obj_get_child(objects.panel_left_warning, 1);
    warning_icons[2] = lv_obj_get_child(objects.panel_left_warning, 2);
    warning_icons[3] = lv_obj_get_child(objects.panel_right_warning, 0);
    warning_icons[4] = lv_obj_get_child(objects.panel_right_warning, 1);
    warning_icons[5] = lv_obj_get_child(objects.panel_right_warning, 2);
    menu_items[0] = objects.menu_item_defult;
    menu_items[1] = objects.menu_item_vehicle;
    menu_items[2] = objects.menu_item_trip_meter;
    menu_items[3] = objects.menu_item_driving_assistance;
    menu_items[4] = objects.menu_item_fuel_level;
    menu_items[5] = objects.menu_item_settings;
    dashboard_debug_panel = debug_panel_create();
    update_warning_icons();
    select_menu_item(0);
    refresh_timer = lv_timer_create(refresh_cb, UI_REFRESH_PERIOD_MS, NULL);
}

void ui_bridge_deinit(void) {
    /* Display deletion can happen from SDL's event timer.  Remove this timer
       synchronously so the same lv_timer_handler() pass cannot update objects
       after LVGL has detached and deleted their screen. */
    if (refresh_timer != NULL) {
        lv_timer_delete(refresh_timer);
        refresh_timer = NULL;
    }

    dashboard_vehicle_data = NULL;
}
