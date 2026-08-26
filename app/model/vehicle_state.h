#ifndef VEHICLE_STATE_H
#define VEHICLE_STATE_H

#include <stdbool.h>
#include <stdint.h>

/* Protocol Enums defined in PROTOCOL.md */
typedef enum {
    VEHICLE_GEAR_P = 0,
    VEHICLE_GEAR_R = 1,
    VEHICLE_GEAR_N = 2,
    VEHICLE_GEAR_D = 3
} vehicle_gear_t;

typedef enum {
    VEHICLE_TURN_SIGNAL_NONE   = 0,
    VEHICLE_TURN_SIGNAL_LEFT   = 1,
    VEHICLE_TURN_SIGNAL_RIGHT  = 2,
    VEHICLE_TURN_SIGNAL_HAZARD = 3
} vehicle_turn_signal_t;

typedef enum {
    VEHICLE_WARNING_NONE     = 0,
    VEHICLE_WARNING_GENERAL  = 1,
    VEHICLE_WARNING_CRITICAL = 2
} vehicle_warning_t;

typedef enum {
    VEHICLE_VALIDITY_VALID         = 0,
    VEHICLE_VALIDITY_INVALID_SPEED = 1,
    VEHICLE_VALIDITY_INCOMPLETE    = 2,
    VEHICLE_VALIDITY_STALE         = 3
} vehicle_validity_t;

typedef enum {
    VEHICLE_DATA_STATUS_NORMAL                 = 0,
    VEHICLE_DATA_STATUS_INVALID                = 1,
    VEHICLE_DATA_STATUS_NO_DATA                = 2,
    VEHICLE_DATA_STATUS_SOURCE_DISCONNECTED    = 3,
    VEHICLE_DATA_STATUS_TRANSPORT_DISCONNECTED = 4
} vehicle_data_status_t;

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

