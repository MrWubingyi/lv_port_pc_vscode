#ifndef UI_BRIDGE_H
#define UI_BRIDGE_H

typedef struct vehicle_data vehicle_data_t;

/* Connects the generated EEZ Studio widgets to the vehicle data model. */
void ui_bridge_init(vehicle_data_t *vehicle_data);

/* Stop callbacks that access LVGL objects before their display is deleted. */
void ui_bridge_deinit(void);

#endif
