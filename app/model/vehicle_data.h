#ifndef VEHICLE_DATA_H
#define VEHICLE_DATA_H

#include "vehicle_state.h"

#include <stdbool.h>
#include <stdint.h>

/* Thread-safe data model: TCP is the base state; UART may override fields. */
typedef struct vehicle_data vehicle_data_t;
typedef uint32_t vehicle_data_fields_t;

enum {
    VEHICLE_DATA_FIELD_SPEED          = 1U << 0,
    VEHICLE_DATA_FIELD_RPM            = 1U << 1,
    VEHICLE_DATA_FIELD_GEAR           = 1U << 2,
    VEHICLE_DATA_FIELD_SOC            = 1U << 3,
    VEHICLE_DATA_FIELD_RANGE          = 1U << 4,
    VEHICLE_DATA_FIELD_OUTSIDE_TEMP   = 1U << 5,
    VEHICLE_DATA_FIELD_LOAD           = 1U << 6,
    VEHICLE_DATA_FIELD_LOAD_MAX       = 1U << 7,
    VEHICLE_DATA_FIELD_TRIP           = 1U << 8,
    VEHICLE_DATA_FIELD_ENGINE_WARNING = 1U << 9,
    VEHICLE_DATA_FIELD_TURN_SIGNAL    = 1U << 10,
    VEHICLE_DATA_FIELD_DOOR_LOCK      = 1U << 11,
    VEHICLE_DATA_FIELD_COOLANT_WARNING = 1U << 12,
    VEHICLE_DATA_FIELD_HIGH_BEAM        = 1U << 13,
    VEHICLE_DATA_FIELD_BATTERY_WARNING  = 1U << 14,
    VEHICLE_DATA_FIELD_SEATBELT_WARNING = 1U << 15
};

vehicle_data_t *vehicle_data_create(void);
void vehicle_data_destroy(vehicle_data_t *data);
void vehicle_data_update_from_tcp(vehicle_data_t *data,
                                  const vehicle_state_t *state);
void vehicle_data_apply_uart_override(vehicle_data_t *data,
                                      const vehicle_state_t *values,
                                      vehicle_data_fields_t fields);
void vehicle_data_clear_uart_override(vehicle_data_t *data,
                                      vehicle_data_fields_t fields);
bool vehicle_data_get_snapshot(vehicle_data_t *data, vehicle_state_t *state,
                               uint64_t *last_tcp_receive_ms);
void vehicle_data_set_tcp_disconnected(vehicle_data_t *data);

#endif
