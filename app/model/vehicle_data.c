#include "vehicle_data.h"

#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define VEHICLE_TIMEOUT_MS 3000U

struct vehicle_data {
    pthread_mutex_t mutex;
    vehicle_state_t tcp_state;
    vehicle_state_t uart_values;
    vehicle_data_fields_t uart_fields;
    uint64_t last_tcp_receive_ms;
    uint64_t last_seq;
    bool seq_initialized;
    bool tcp_valid;
};

static uint64_t monotonic_time_ms(void)
{
    struct timespec value;
    clock_gettime(CLOCK_MONOTONIC, &value);
    return (uint64_t)value.tv_sec * 1000U +
           (uint64_t)value.tv_nsec / 1000000U;
}

vehicle_data_t *vehicle_data_create(void)
{
    vehicle_data_t *data = calloc(1, sizeof(*data));
    if(data == NULL) return NULL;
    if(pthread_mutex_init(&data->mutex, NULL) != 0) {
        free(data);
        return NULL;
    }
    data->tcp_state.gear = VEHICLE_GEAR_P;
    data->tcp_state.door_lock = true;
    return data;
}

void vehicle_data_destroy(vehicle_data_t *data)
{
    if(data == NULL) return;
    pthread_mutex_destroy(&data->mutex);
    free(data);
}

void vehicle_data_update_from_tcp(vehicle_data_t *data,
                                  const vehicle_state_t *state)
{
    if(data == NULL || state == NULL) return;
    pthread_mutex_lock(&data->mutex);

    /* Check dataStatus: If explicit disconnect or no data reported, invalidate state */
    if(state->data_status == VEHICLE_DATA_STATUS_SOURCE_DISCONNECTED ||
       state->data_status == VEHICLE_DATA_STATUS_TRANSPORT_DISCONNECTED ||
       state->data_status == VEHICLE_DATA_STATUS_NO_DATA) {
        if(data->tcp_valid) {
            data->tcp_valid = false;
            printf("[VehicleData] Data status reported offline (status=%d)\n", state->data_status);
            fflush(stdout);
        }
        pthread_mutex_unlock(&data->mutex);
        return;
    }

    /* Monotonic sequence number check & packet loss detection */
    if(data->seq_initialized) {
        if(state->sequence < data->last_seq) {
            printf("[VehicleData] Warning: Sequence decreased from %" PRIu64 " to %" PRIu64 " (reordered packet or reset)\n",
                   data->last_seq, state->sequence);
        } else if(state->sequence > data->last_seq + 1) {
            printf("[VehicleData] Warning: Sequence gap detected: missed %" PRIu64 " packets (expected %" PRIu64 ", got %" PRIu64 ")\n",
                   state->sequence - data->last_seq - 1, data->last_seq + 1, state->sequence);
        }
    }

    if(!data->tcp_valid) {
        printf("[VehicleData] Stream active, vehicle online (seq=%" PRIu64 ")\n", state->sequence);
        fflush(stdout);
    }

    data->last_seq = state->sequence;
    data->seq_initialized = true;
    data->tcp_state = *state;
    data->last_tcp_receive_ms = monotonic_time_ms();
    data->tcp_valid = true;
    pthread_mutex_unlock(&data->mutex);
}

void vehicle_data_apply_uart_override(vehicle_data_t *data,
                                      const vehicle_state_t *values,
                                      vehicle_data_fields_t fields)
{
    if(data == NULL || values == NULL) return;
    pthread_mutex_lock(&data->mutex);
    if((fields & VEHICLE_DATA_FIELD_SPEED) != 0U) data->uart_values.speed_kph = values->speed_kph;
    if((fields & VEHICLE_DATA_FIELD_RPM) != 0U) data->uart_values.rpm = values->rpm;
    if((fields & VEHICLE_DATA_FIELD_GEAR) != 0U) data->uart_values.gear = values->gear;
    if((fields & VEHICLE_DATA_FIELD_SOC) != 0U) data->uart_values.soc = values->soc;
    if((fields & VEHICLE_DATA_FIELD_RANGE) != 0U) data->uart_values.range_km = values->range_km;
    if((fields & VEHICLE_DATA_FIELD_OUTSIDE_TEMP) != 0U) data->uart_values.outside_temp_c = values->outside_temp_c;
    if((fields & VEHICLE_DATA_FIELD_LOAD) != 0U) data->uart_values.load_tenths = values->load_tenths;
    if((fields & VEHICLE_DATA_FIELD_LOAD_MAX) != 0U) data->uart_values.load_max_tenths = values->load_max_tenths;
    if((fields & VEHICLE_DATA_FIELD_TRIP) != 0U) data->uart_values.trip_tenths = values->trip_tenths;
    if((fields & VEHICLE_DATA_FIELD_ENGINE_WARNING) != 0U) data->uart_values.engine_warning = values->engine_warning;
    if((fields & VEHICLE_DATA_FIELD_TURN_SIGNAL) != 0U) data->uart_values.turn_signal = values->turn_signal;
    if((fields & VEHICLE_DATA_FIELD_DOOR_LOCK) != 0U) data->uart_values.door_lock = values->door_lock;
    if((fields & VEHICLE_DATA_FIELD_COOLANT_WARNING) != 0U) data->uart_values.coolant_warning = values->coolant_warning;
    if((fields & VEHICLE_DATA_FIELD_HIGH_BEAM) != 0U) data->uart_values.high_beam = values->high_beam;
    if((fields & VEHICLE_DATA_FIELD_BATTERY_WARNING) != 0U) data->uart_values.battery_warning = values->battery_warning;
    if((fields & VEHICLE_DATA_FIELD_SEATBELT_WARNING) != 0U) data->uart_values.seatbelt_warning = values->seatbelt_warning;
    data->uart_fields |= fields;
    pthread_mutex_unlock(&data->mutex);
}

void vehicle_data_clear_uart_override(vehicle_data_t *data,
                                      vehicle_data_fields_t fields)
{
    if(data == NULL) return;
    pthread_mutex_lock(&data->mutex);
    data->uart_fields &= ~fields;
    pthread_mutex_unlock(&data->mutex);
}

void vehicle_data_set_tcp_disconnected(vehicle_data_t *data)
{
    if(data == NULL) return;
    pthread_mutex_lock(&data->mutex);
    if(data->tcp_valid) {
        data->tcp_valid = false;
        printf("[VehicleData] TCP disconnected, vehicle state offline\n");
        fflush(stdout);
    }
    data->seq_initialized = false;
    pthread_mutex_unlock(&data->mutex);
}

void vehicle_data_report_invalid_frame(vehicle_data_t *data)
{
    if(data == NULL) return;
    pthread_mutex_lock(&data->mutex);
    data->tcp_state.data_status = VEHICLE_DATA_STATUS_INVALID;
    data->tcp_state.validity = VEHICLE_VALIDITY_INVALID_SPEED;
    data->last_tcp_receive_ms = monotonic_time_ms();
    data->tcp_valid = true;
    printf("[VehicleData] Invalid/corrupted frame reported, state marked invalid\n");
    fflush(stdout);
    pthread_mutex_unlock(&data->mutex);
}

#define APPLY_OVERRIDE(field_mask, member) \
    do { if((fields & (field_mask)) != 0U) state->member = uart.member; } while(0)

bool vehicle_data_get_snapshot(vehicle_data_t *data, vehicle_state_t *state,
                               uint64_t *last_tcp_receive_ms)
{
    if(data == NULL || state == NULL || last_tcp_receive_ms == NULL) return false;
    pthread_mutex_lock(&data->mutex);
    if(data->tcp_valid) {
        uint64_t now_ms = monotonic_time_ms();
        if(now_ms - data->last_tcp_receive_ms > VEHICLE_TIMEOUT_MS) {
            data->tcp_valid = false;
            printf("[VehicleData] Watchdog: TCP receive timeout (> %u ms), vehicle state offline\n", VEHICLE_TIMEOUT_MS);
            fflush(stdout);
        }
    }
    bool valid = data->tcp_valid;
    if(valid) {
        *state = data->tcp_state;
        *last_tcp_receive_ms = data->last_tcp_receive_ms;
        vehicle_state_t uart = data->uart_values;
        vehicle_data_fields_t fields = data->uart_fields;
        APPLY_OVERRIDE(VEHICLE_DATA_FIELD_SPEED, speed_kph);
        APPLY_OVERRIDE(VEHICLE_DATA_FIELD_RPM, rpm);
        APPLY_OVERRIDE(VEHICLE_DATA_FIELD_GEAR, gear);
        APPLY_OVERRIDE(VEHICLE_DATA_FIELD_SOC, soc);
        APPLY_OVERRIDE(VEHICLE_DATA_FIELD_RANGE, range_km);
        APPLY_OVERRIDE(VEHICLE_DATA_FIELD_OUTSIDE_TEMP, outside_temp_c);
        APPLY_OVERRIDE(VEHICLE_DATA_FIELD_LOAD, load_tenths);
        APPLY_OVERRIDE(VEHICLE_DATA_FIELD_LOAD_MAX, load_max_tenths);
        APPLY_OVERRIDE(VEHICLE_DATA_FIELD_TRIP, trip_tenths);
        APPLY_OVERRIDE(VEHICLE_DATA_FIELD_ENGINE_WARNING, engine_warning);
        APPLY_OVERRIDE(VEHICLE_DATA_FIELD_TURN_SIGNAL, turn_signal);
        APPLY_OVERRIDE(VEHICLE_DATA_FIELD_DOOR_LOCK, door_lock);
        APPLY_OVERRIDE(VEHICLE_DATA_FIELD_COOLANT_WARNING, coolant_warning);
        APPLY_OVERRIDE(VEHICLE_DATA_FIELD_HIGH_BEAM, high_beam);
        APPLY_OVERRIDE(VEHICLE_DATA_FIELD_BATTERY_WARNING, battery_warning);
        APPLY_OVERRIDE(VEHICLE_DATA_FIELD_SEATBELT_WARNING, seatbelt_warning);
    }
    pthread_mutex_unlock(&data->mutex);
    return valid;
}

#undef APPLY_OVERRIDE
