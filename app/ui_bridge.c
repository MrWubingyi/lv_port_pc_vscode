#include "ui_bridge.h"

#include "screens.h"
#include "vehicle_state_store.h"

#include <stdio.h>
#include <time.h>

#define UI_REFRESH_PERIOD_MS 100
#define VEHICLE_TIMEOUT_MS 3000

static uint64_t monotonic_ms(void) {
    struct timespec value;
    clock_gettime(CLOCK_MONOTONIC, &value);
    return (uint64_t)value.tv_sec * 1000U + (uint64_t)value.tv_nsec / 1000000U;
}

static void update_clock(void) {
    time_t now = time(NULL);
    struct tm local_time;

    if (localtime_r(&now, &local_time) != NULL) {
        lv_label_set_text_fmt(objects.lbl_time, "%02d:%02d",
                              local_time.tm_hour, local_time.tm_min);
    }
}

static void set_connection_state(bool online) {
    lv_label_set_text(objects.lbl_status_title,
                      online ? "Vehicle online" : "Waiting for vehicle");
    lv_obj_set_style_text_color(
        objects.lbl_status_title,
        lv_color_hex(online ? 0x59D6EA : 0xFF9F43),
        LV_PART_MAIN);
}

static void refresh_cb(lv_timer_t *timer) {
    LV_UNUSED(timer);

    vehicle_state_t state;
    uint64_t last_receive_ms;
    bool valid = vehicle_state_store_get_snapshot(&state, &last_receive_ms);
    bool online = valid && monotonic_ms() - last_receive_ms <= VEHICLE_TIMEOUT_MS;

    update_clock();
    set_connection_state(online);
    if (!online) {
        return;
    }

    int speed = state.speed_kph < 0 ? 0 : state.speed_kph;
    int soc = state.soc < 0 ? 0 : (state.soc > 100 ? 100 : state.soc);
    char gear[2] = {state.gear ? state.gear : '-', '\0'};

    lv_label_set_text_fmt(objects.lbl_speed, "%d", speed);
    lv_label_set_text(objects.lbl_gear, gear);
    lv_bar_set_value(objects.bar_fuel, soc, LV_ANIM_ON);
    lv_label_set_text_fmt(objects.lbl_range, "%d%%", soc);
}

void ui_bridge_init(void) {
    update_clock();
    set_connection_state(false);
    lv_timer_create(refresh_cb, UI_REFRESH_PERIOD_MS, NULL);
}
