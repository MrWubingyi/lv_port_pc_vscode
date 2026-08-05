#ifndef VEHICLE_TCP_SERVER_H
#define VEHICLE_TCP_SERVER_H

#include <stdbool.h>
#include <stdint.h>

typedef struct vehicle_data vehicle_data_t;

bool vehicle_tcp_server_start(uint16_t port, vehicle_data_t *vehicle_data);
void vehicle_tcp_server_stop(void);

#endif
