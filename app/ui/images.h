#ifndef EEZ_LVGL_UI_IMAGES_H
#define EEZ_LVGL_UI_IMAGES_H

#include <lvgl/lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const lv_img_dsc_t img_dashboard_background;
extern const lv_img_dsc_t img_icon_abs_16;
extern const lv_img_dsc_t img_icon_airbag_16;
extern const lv_img_dsc_t img_icon_battery_16;
extern const lv_img_dsc_t img_icon_brake_warning_16;
extern const lv_img_dsc_t img_icon_check_engine_16;
extern const lv_img_dsc_t img_icon_coolant_16;
extern const lv_img_dsc_t img_icon_high_beam_16;
extern const lv_img_dsc_t img_icon_oil_pressure_16;
extern const lv_img_dsc_t img_icon_rear_fog_16;
extern const lv_img_dsc_t img_icon_seat_belt_16;
extern const lv_img_dsc_t img_icon_tire_pressure_16;
extern const lv_img_dsc_t img_icon_traction_control_16;
extern const lv_img_dsc_t img_menu_defult_16;
extern const lv_img_dsc_t img_menu_vehicle_16;
extern const lv_img_dsc_t img_menu_trip_meter_16;
extern const lv_img_dsc_t img_menu_driving_assistance_16;
extern const lv_img_dsc_t img_menu_fuel_level_16;
extern const lv_img_dsc_t img_meunu_settings_16;
extern const lv_img_dsc_t img_icon_seat_belt_24;
extern const lv_img_dsc_t img_icon_brake_warning_24;
extern const lv_img_dsc_t img_icon_parking_break_24;
extern const lv_img_dsc_t img_icon_coolant_24;
extern const lv_img_dsc_t img_icon_high_beam_24;
extern const lv_img_dsc_t img_icon_check_engine_24;
extern const lv_img_dsc_t img_vehicle_truck;
extern const lv_img_dsc_t img_bar_fuel;
extern const lv_img_dsc_t img_icon_turn_right_32;
extern const lv_img_dsc_t img_icon_turn_left_32;
extern const lv_img_dsc_t img_icon_door_unlocked_24;

#ifndef EXT_IMG_DESC_T
#define EXT_IMG_DESC_T
typedef struct _ext_img_desc_t {
    const char *name;
    const lv_img_dsc_t *img_dsc;
} ext_img_desc_t;
#endif

extern const ext_img_desc_t images[30];

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_IMAGES_H*/