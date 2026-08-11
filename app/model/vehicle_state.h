#ifndef VEHICLE_STATE_H
#define VEHICLE_STATE_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    int version;
    uint64_t sequence;
    uint64_t timestamp_ms;
    int speed_kph;
    int rpm;
    int gear;               /* 0: P, 1: R, 2: N, 3: D */
    int soc;
    int turn_signal;        /* 0: NONE, 1: LEFT, 2: RIGHT, 3: HAZARD */
    bool parking_brake;     /* true: 开启, false: 关闭 */
    int warning;            /* 0: NONE, 1: GENERAL, 2: CRITICAL */
    int validity;           /* 0: VALID, 1: INVALID_SPEED, 2: INCOMPLETE, 3: STALE */
    bool door_lock;         /* true: 已锁定, false: 未锁定 */
    int headlights_state;
    int high_beam_lights_state;
    double engine_coolant_temp;
    double ev_battery_level;
    int data_status;        /* 0: NORMAL, 1: INVALID, 2: NO_DATA, 3: DISCONNECTED, 4: TRANSPORT_ERR */

    int range_km;
    int outside_temp_c;
    int load_tenths;
    int load_max_tenths;
    int trip_tenths;
    bool engine_warning;
    uint32_t steering_buttons;
    bool seatbelt_warning;
    bool handbrake;
    bool braking_warning;
    bool coolant_warning;
    bool high_beam;
    bool low_beam;
    bool battery_warning;
} vehicle_state_t;

bool vehicle_state_parse_json(
    const char *json_text,
    vehicle_state_t *state
);

void vehicle_state_print(const vehicle_state_t *state);

#endif

