#ifndef VEHICLE_TCP_SERVER_H
#define VEHICLE_TCP_SERVER_H

#include <stdbool.h>
#include <stdint.h>

bool vehicle_tcp_server_start(uint16_t port);
void vehicle_tcp_server_stop(void);

#endif
