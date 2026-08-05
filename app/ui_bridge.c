#include "ui_bridge.h"

#include "screens.h"
#include "vehicle_state_store.h"

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

static void set_connection_state(vehicle_state_source_t source) {
    const char *title = "Waiting for vehicle";
    uint32_t color = 0xFF9F43;
    if (source == VEHICLE_STATE_SOURCE_NETWORK) {
        title = "Vehicle online";
        color = 0x59D6EA;
    } else if (source == VEHICLE_STATE_SOURCE_SIMULATED) {
        title = "Demo data";
        color = 0xA8D672;
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

    vehicle_state_t state;
    uint64_t last_receive_ms;
    vehicle_state_source_t source;
    bool valid = vehicle_state_store_get_snapshot_ex(
        &state, &last_receive_ms, &source);
    (void)last_receive_ms;

    update_clock();
    set_connection_state(valid ? source : VEHICLE_STATE_SOURCE_NONE);
    if (!valid) {
        return;
    }

    int speed = state.speed_kph < 0 ? 0 : state.speed_kph;
    int soc = state.soc < 0 ? 0 : (state.soc > 100 ? 100 : state.soc);
    int load_max = state.load_max_tenths > 0 ? state.load_max_tenths : 1;
    int load_percent = state.load_tenths * 100 / load_max;
    if (load_percent < 0) load_percent = 0;
    if (load_percent > 100) load_percent = 100;
    char gear[2] = {state.gear ? state.gear : '-', '\0'};

    lv_label_set_text_fmt(objects.lbl_speed, "%d", speed);
    lv_label_set_text(objects.lbl_gear, gear);
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
    update_active_lines(speed, state.rpm < 0 ? 0 : state.rpm);
    update_scale_ticks(speed, state.rpm < 0 ? 0 : state.rpm);
}

void ui_bridge_init(void) {
    update_clock();
    set_connection_state(VEHICLE_STATE_SOURCE_NONE);
    create_scale_ticks();
    lv_timer_create(refresh_cb, UI_REFRESH_PERIOD_MS, NULL);
}
