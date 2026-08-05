#include "vehicle_data.h"

#include <pthread.h>
#include <stdlib.h>
#include <time.h>

struct vehicle_data {
    pthread_mutex_t mutex;
    vehicle_state_t tcp_state;
    vehicle_state_t uart_values;
    vehicle_data_fields_t uart_fields;
    uint64_t last_tcp_receive_ms;
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
    data->tcp_state.gear = '-';
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

#define APPLY_OVERRIDE(field_mask, member) \
    do { if((fields & (field_mask)) != 0U) state->member = uart.member; } while(0)

bool vehicle_data_get_snapshot(vehicle_data_t *data, vehicle_state_t *state,
                               uint64_t *last_tcp_receive_ms)
{
    if(data == NULL || state == NULL || last_tcp_receive_ms == NULL) return false;
    pthread_mutex_lock(&data->mutex);
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
    }
    pthread_mutex_unlock(&data->mutex);
    return valid;
}

#undef APPLY_OVERRIDE
