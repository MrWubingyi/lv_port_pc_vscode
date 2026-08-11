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

        int turn_sig = 0;
        if (cycle_tick >= 30U && cycle_tick < 80U) {
            turn_sig = 1; /* LEFT */
        } else if (cycle_tick >= 120U && cycle_tick < 170U) {
            turn_sig = 2; /* RIGHT */
        } else if (cycle_tick >= 210U && cycle_tick < 260U) {
            turn_sig = 3; /* HAZARD */
        }

        bool is_p_gear = cycle_tick < 20U;
        bool door_locked = !is_p_gear;
        bool high_beam_on = (cycle_tick >= 60U && cycle_tick < 120U);
        double coolant_temp = (cycle_tick >= 220U && cycle_tick < 280U) ? 108.5 : 88.0;

        vehicle_state_t state = {
            .version = 1,
            .sequence = tick,
            .timestamp_ms = (uint64_t)tick * FAKE_PERIOD_MS,
            .speed_kph = speed,
            .rpm = speed == 0 ? 0 : 500 + speed * 18,
            .gear = is_p_gear ? 0 : 3, /* 0: P, 3: D */
            .soc = soc,
            .turn_signal = turn_sig,
            .door_lock = door_locked,
            .parking_brake = is_p_gear,
            .warning = (coolant_temp > 105.0 || soc <= 15) ? 1 : 0,
            .validity = 0,
            .high_beam_lights_state = high_beam_on ? 1 : 0,
            .high_beam = high_beam_on,
            .engine_coolant_temp = coolant_temp,
            .coolant_warning = (coolant_temp > 105.0),
            .ev_battery_level = (double)soc,
            .battery_warning = (soc <= 15),
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
