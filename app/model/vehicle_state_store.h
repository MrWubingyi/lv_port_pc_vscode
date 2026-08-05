#ifndef VEHICLE_STATE_STORE_H
#define VEHICLE_STATE_STORE_H

#include "vehicle_state.h"

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    VEHICLE_STATE_SOURCE_NONE = 0,
    VEHICLE_STATE_SOURCE_SIMULATED,
    VEHICLE_STATE_SOURCE_NETWORK
} vehicle_state_source_t;

void vehicle_state_store_init(void);

void vehicle_state_store_update(
    const vehicle_state_t *state
);

bool vehicle_state_store_update_simulated(
    const vehicle_state_t *state,
    uint64_t network_timeout_ms
);

bool vehicle_state_store_get_snapshot(
    vehicle_state_t *state,
    uint64_t *last_receive_ms
);

bool vehicle_state_store_get_snapshot_ex(
    vehicle_state_t *state,
    uint64_t *last_receive_ms,
    vehicle_state_source_t *source
);

#endif
