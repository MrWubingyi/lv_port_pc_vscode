#include "vehicle_fake_data.h"

#include "vehicle_state.h"
#include "vehicle_state_store.h"

#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#define FAKE_PERIOD_MS 100
#define NETWORK_TIMEOUT_MS 3000
#define CYCLE_TICKS 300

static pthread_t fake_thread;
static atomic_bool fake_running;

static int simulated_speed(unsigned int cycle_tick)
{
    if(cycle_tick < 20U) {
        return 0;
    }
    if(cycle_tick < 120U) {
        return (int)((cycle_tick - 20U) * 80U / 100U);
    }
    if(cycle_tick < 180U) {
        return 80;
    }
    return (int)((300U - cycle_tick) * 80U / 120U);
}

static void *fake_thread_main(void *unused)
{
    (void)unused;
    unsigned int tick = 0;
    uint64_t trip_milli_tenths = 0;
    const struct timespec period = {.tv_sec = 0, .tv_nsec = 100000000L};

    while(atomic_load(&fake_running)) {
        unsigned int cycle_tick = tick % CYCLE_TICKS;
        int speed = simulated_speed(cycle_tick);
        int soc = 100 - (int)((tick / 100U) % 81U);
        unsigned int load_phase = cycle_tick < 150U ? cycle_tick : 300U - cycle_tick;
        int load_tenths = 64 + (int)(load_phase * 90U / 150U);

        trip_milli_tenths += (uint64_t)speed * 1000U / 36000U;
        vehicle_state_t state = {
            .version = 1,
            .sequence = tick,
            .timestamp_ms = (uint64_t)tick * FAKE_PERIOD_MS,
            .speed_kph = speed,
            .rpm = speed == 0 ? 0 : 500 + speed * 18,
            .gear = cycle_tick < 20U ? 'P' : 'D',
            .soc = soc,
            .range_km = soc * 12,
            .outside_temp_c = -5,
            .load_tenths = load_tenths,
            .load_max_tenths = 220,
            .trip_tenths = (int)(trip_milli_tenths / 1000U)
        };
        vehicle_state_store_update_simulated(&state, NETWORK_TIMEOUT_MS);
        tick++;
        nanosleep(&period, NULL);
    }
    return NULL;
}

bool vehicle_fake_data_start(void)
{
    if(atomic_exchange(&fake_running, true)) {
        return true;
    }
    if(pthread_create(&fake_thread, NULL, fake_thread_main, NULL) != 0) {
        atomic_store(&fake_running, false);
        return false;
    }
    printf("Fake vehicle data enabled\n");
    fflush(stdout);
    return true;
}

void vehicle_fake_data_stop(void)
{
    if(!atomic_exchange(&fake_running, false)) {
        return;
    }
    pthread_join(fake_thread, NULL);
}
