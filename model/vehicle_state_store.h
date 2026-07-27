#ifndef VEHICLE_STATE_STORE_H
#define VEHICLE_STATE_STORE_H

#include "vehicle_state.h"

#include <stdbool.h>
#include <stdint.h>

void vehicle_state_store_init(void);

void vehicle_state_store_update(
    const vehicle_state_t *state
);

bool vehicle_state_store_get_snapshot(
    vehicle_state_t *state,
    uint64_t *last_receive_ms
);

#endif
