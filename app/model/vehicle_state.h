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
    char gear;
    int soc;
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
} vehicle_state_t;

bool vehicle_state_parse_json(
    const char *json_text,
    vehicle_state_t *state
);

void vehicle_state_print(const vehicle_state_t *state);

#endif
