#ifndef EEZ_LVGL_UI_IMAGES_H
#define EEZ_LVGL_UI_IMAGES_H

#include <lvgl/lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef UI_USE_PNG_ASSETS
#define UI_USE_PNG_ASSETS 0
#endif

#if UI_USE_PNG_ASSETS
#define UI_IMAGE_DECL(name) extern const char name[]
#else
#define UI_IMAGE_DECL(name) extern const lv_img_dsc_t name
#endif

UI_IMAGE_DECL(img_dashboard_background);
UI_IMAGE_DECL(img_icon_abs_16);
UI_IMAGE_DECL(img_icon_airbag_16);
UI_IMAGE_DECL(img_icon_battery_16);
UI_IMAGE_DECL(img_icon_brake_warning_16);
UI_IMAGE_DECL(img_icon_check_engine_16);
UI_IMAGE_DECL(img_icon_coolant_16);
UI_IMAGE_DECL(img_icon_high_beam_16);
UI_IMAGE_DECL(img_icon_oil_pressure_16);
UI_IMAGE_DECL(img_icon_rear_fog_16);
UI_IMAGE_DECL(img_icon_seat_belt_16);
UI_IMAGE_DECL(img_icon_tire_pressure_16);
UI_IMAGE_DECL(img_icon_traction_control_16);
UI_IMAGE_DECL(img_menu_defult_16);
UI_IMAGE_DECL(img_menu_vehicle_16);
UI_IMAGE_DECL(img_menu_trip_meter_16);
UI_IMAGE_DECL(img_menu_driving_assistance_16);
UI_IMAGE_DECL(img_menu_fuel_level_16);
UI_IMAGE_DECL(img_meunu_settings_16);
UI_IMAGE_DECL(img_icon_seat_belt_24);
UI_IMAGE_DECL(img_icon_brake_warning_24);
UI_IMAGE_DECL(img_icon_parking_break_24);
UI_IMAGE_DECL(img_icon_coolant_24);
UI_IMAGE_DECL(img_icon_high_beam_24);
UI_IMAGE_DECL(img_icon_check_engine_24);
UI_IMAGE_DECL(img_vehicle_truck);
UI_IMAGE_DECL(img_bar_fuel);
UI_IMAGE_DECL(img_icon_turn_right_32);
UI_IMAGE_DECL(img_icon_turn_left_32);
UI_IMAGE_DECL(img_icon_door_unlocked_24);

#ifndef EXT_IMG_DESC_T
#define EXT_IMG_DESC_T
typedef struct _ext_img_desc_t {
    const char *name;
    const void *img_dsc;
} ext_img_desc_t;
#endif

extern const ext_img_desc_t images[30];

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_IMAGES_H*/
