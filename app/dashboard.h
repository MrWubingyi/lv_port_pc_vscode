#ifndef DASHBOARD_H
#define DASHBOARD_H
#include "lvgl/lvgl.h"
#include <stdint.h>
enum dashboard_status_enum {
  dashboard_status_normal,
  dashboard_status_warning,
  dashboard_status_error,
  dashboard_status_offline
};
struct dashboard_status_t {
  enum dashboard_status_enum status;
  uint32_t color;
};
void dashboard_create(void);
void dashboard_destroy(void);
void dashboard_start_simulation(void);
void dashboard_stop_simulation(void);
void dashboard_set_speed(int speed);
void dashboard_set_gear(char gear);
void dashboard_set_status(lv_obj_t *obj, enum dashboard_status_enum status);
uint32_t dashboard_status_get_color(enum dashboard_status_enum status);
#endif

