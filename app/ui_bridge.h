#ifndef UI_BRIDGE_H
#define UI_BRIDGE_H

typedef struct vehicle_data vehicle_data_t;

/* Connects the generated EEZ Studio widgets to the vehicle data model. */
void ui_bridge_init(vehicle_data_t *vehicle_data);

#endif
