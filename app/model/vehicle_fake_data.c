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
#define CYCLE_TICKS 1200  /* 120-second test cycle at 100ms per tick */

static pthread_t fake_thread;
static atomic_bool fake_running;

static int simulated_speed(unsigned int phase_tick)
{
    if(phase_tick < 40U) {
        return 0;
    }
    if(phase_tick < 240U) {
        return (int)((phase_tick - 40U) * 80U / 200U);
    }
    if(phase_tick < 360U) {
        return 80;
    }
    if(phase_tick < 560U) {
        return (int)((560U - phase_tick) * 80U / 200U);
    }
    return 0;
}

static void *fake_thread_main(void *unused)
{
    (void)unused;
    unsigned int tick = 0;
    uint64_t trip_milli_tenths = 0;
    const struct timespec period = {.tv_sec = 0, .tv_nsec = 100000000L};

    while(atomic_load(&fake_running)) {
        unsigned int cycle_tick = tick % CYCLE_TICKS;

        /* PROTOCOL.md Section 6 Mock Testing Scenarios (120s cycle):
         * 1. Normal simulation (0-60s, ticks 0-599): Normal speed, gear, lights.
         * 2. Illegal data test (61-70s, ticks 600-699): speed 255, rpm 9999, soc 150, dataStatus 1.
         * 3. Silence / Disconnection test (70-100s, ticks 700-999): No packets sent for 30s.
         * 4. Recovery simulation (100-120s, ticks 1000-1199): Packets resume, seq increments.
         */

        if (cycle_tick >= 700U && cycle_tick < 1000U) {
            /* Scenario 3: Silence test (70-100s) - do not send any updates */
            tick++;
            nanosleep(&period, NULL);
            continue;
        }

        if (cycle_tick >= 600U && cycle_tick < 700U) {
            /* Scenario 2: Illegal data test (61-70s) */
            vehicle_state_t invalid_state = {
                .version = 1,
                .sequence = tick,
                .timestamp_ms = (uint64_t)tick * FAKE_PERIOD_MS,
                .speed_kph = 255,
                .rpm = 9999,
                .gear = VEHICLE_GEAR_D,
                .soc = 150,
                .turn_signal = VEHICLE_TURN_SIGNAL_NONE,
                .door_lock = false,
                .parking_brake = false,
                .warning = VEHICLE_WARNING_CRITICAL,
                .validity = VEHICLE_VALIDITY_INVALID_SPEED,
                .high_beam_lights_state = 1,
                .high_beam = true,
                .engine_coolant_temp = -999.0,
                .coolant_warning = true,
                .ev_battery_level = 150.0,
                .battery_warning = true,
                .data_status = VEHICLE_DATA_STATUS_INVALID,
                .range_km = 0,
                .outside_temp_c = -5,
                .load_tenths = 64,
                .load_max_tenths = 220,
                .trip_tenths = (int)(trip_milli_tenths / 1000U)
            };
            vehicle_state_store_update_simulated(&invalid_state, NETWORK_TIMEOUT_MS);
            tick++;
            nanosleep(&period, NULL);
            continue;
        }

        /* Scenarios 1 & 4: Normal simulation */
        unsigned int norm_tick = (cycle_tick < 600U) ? cycle_tick : (cycle_tick - 1000U + 400U);
        int speed = simulated_speed(norm_tick % 600U);
        int soc = 100 - (int)((tick / 100U) % 81U);
        unsigned int load_phase = (norm_tick % 300U) < 150U ? (norm_tick % 300U) : 300U - (norm_tick % 300U);
        int load_tenths = 64 + (int)(load_phase * 90U / 150U);

        int turn_sig = VEHICLE_TURN_SIGNAL_NONE;
        if (norm_tick >= 60U && norm_tick < 160U) {
            turn_sig = VEHICLE_TURN_SIGNAL_LEFT;
        } else if (norm_tick >= 240U && norm_tick < 340U) {
            turn_sig = VEHICLE_TURN_SIGNAL_RIGHT;
        } else if (norm_tick >= 420U && norm_tick < 520U) {
            turn_sig = VEHICLE_TURN_SIGNAL_HAZARD;
        }

        bool is_p_gear = (speed == 0 && (norm_tick < 40U || norm_tick >= 560U));
        bool door_locked = !is_p_gear;
        bool high_beam_on = (norm_tick >= 120U && norm_tick < 240U);
        double coolant_temp = (norm_tick >= 440U && norm_tick < 560U) ? 108.5 : 88.0;

        vehicle_state_t state = {
            .version = 1,
            .sequence = tick,
            .timestamp_ms = (uint64_t)tick * FAKE_PERIOD_MS,
            .speed_kph = speed,
            .rpm = speed == 0 ? 0 : 500 + speed * 18,
            .gear = is_p_gear ? VEHICLE_GEAR_P : VEHICLE_GEAR_D,
            .soc = soc,
            .turn_signal = turn_sig,
            .door_lock = door_locked,
            .parking_brake = is_p_gear,
            .warning = (coolant_temp > 105.0 || soc <= 15) ? VEHICLE_WARNING_GENERAL : VEHICLE_WARNING_NONE,
            .validity = VEHICLE_VALIDITY_VALID,
            .data_status = VEHICLE_DATA_STATUS_NORMAL,
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
