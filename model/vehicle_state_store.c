#include "vehicle_state_store.h"

#include <pthread.h>
#include <string.h>
#include <time.h>

static pthread_mutex_t state_mutex =
    PTHREAD_MUTEX_INITIALIZER;

static vehicle_state_t current_state;
static uint64_t current_receive_ms;
static bool state_valid;

static uint64_t monotonic_time_ms(void)
{
    struct timespec time_value;

    clock_gettime(CLOCK_MONOTONIC, &time_value);

    return (uint64_t)time_value.tv_sec * 1000U
        + (uint64_t)time_value.tv_nsec / 1000000U;
}

void vehicle_state_store_init(void)
{
    pthread_mutex_lock(&state_mutex);

    memset(&current_state, 0, sizeof(current_state));
    current_state.gear = '-';
    current_receive_ms = 0;
    state_valid = false;

    pthread_mutex_unlock(&state_mutex);
}

void vehicle_state_store_update(
    const vehicle_state_t *state
)
{
    if(state == NULL) {
        return;
    }

    pthread_mutex_lock(&state_mutex);

    current_state = *state;
    current_receive_ms = monotonic_time_ms();
    state_valid = true;

    pthread_mutex_unlock(&state_mutex);
}

bool vehicle_state_store_get_snapshot(
    vehicle_state_t *state,
    uint64_t *last_receive_ms
)
{
    if(state == NULL || last_receive_ms == NULL) {
        return false;
    }

    pthread_mutex_lock(&state_mutex);

    bool valid = state_valid;

    if(valid) {
        *state = current_state;
        *last_receive_ms = current_receive_ms;
    }

    pthread_mutex_unlock(&state_mutex);

    return valid;
}
