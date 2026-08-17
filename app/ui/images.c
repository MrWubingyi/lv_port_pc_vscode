#include "images.h"

#if UI_USE_PNG_ASSETS

#ifndef UI_ASSET_ROOT
#define UI_ASSET_ROOT "A:assets"
#endif

#define UI_PNG(name, path) const char name[] = UI_ASSET_ROOT "/" path

UI_PNG(img_dashboard_background, "lvgl-cluster-blue-gradient-800-480.png");
UI_PNG(img_icon_abs_16, "dashboard-warning-lights/16x16/abs.png");
UI_PNG(img_icon_airbag_16, "dashboard-warning-lights/16x16/airbag.png");
UI_PNG(img_icon_battery_16, "dashboard-warning-lights/16x16/battery.png");
UI_PNG(img_icon_brake_warning_16, "dashboard-warning-lights/16x16/brake-warning.png");
UI_PNG(img_icon_check_engine_16, "dashboard-warning-lights/16x16/check-engine.png");
UI_PNG(img_icon_coolant_16, "dashboard-warning-lights/16x16/coolant-temperature.png");
UI_PNG(img_icon_high_beam_16, "dashboard-warning-lights/16x16/high-beam.png");
UI_PNG(img_icon_oil_pressure_16, "dashboard-warning-lights/16x16/oil-pressure.png");
UI_PNG(img_icon_rear_fog_16, "dashboard-warning-lights/16x16/rear-fog.png");
UI_PNG(img_icon_seat_belt_16, "dashboard-warning-lights/16x16/seat-belt.png");
UI_PNG(img_icon_tire_pressure_16, "dashboard-warning-lights/16x16/tire-pressure.png");
UI_PNG(img_icon_traction_control_16, "dashboard-warning-lights/16x16/traction-control.png");
UI_PNG(img_menu_defult_16, "dashboard-ui-icons-white/16x16/01-warning-triangle.png");
UI_PNG(img_menu_vehicle_16, "dashboard-ui-icons-white/16x16/02-vehicle.png");
UI_PNG(img_menu_trip_meter_16, "dashboard-ui-icons-white/16x16/03-steering-wheel.png");
UI_PNG(img_menu_driving_assistance_16, "dashboard-ui-icons-white/16x16/04-seat-comfort.png");
UI_PNG(img_menu_fuel_level_16, "dashboard-ui-icons-white/16x16/05-brightness.png");
UI_PNG(img_meunu_settings_16, "dashboard-ui-icons-white/16x16/06-settings.png");
UI_PNG(img_icon_seat_belt_24, "dashboard-warning-lights/24x24/seat-belt.png");
UI_PNG(img_icon_brake_warning_24, "dashboard-warning-lights/24x24/brake-warning.png");
UI_PNG(img_icon_parking_break_24, "parking-brake-red/24x24/parking-brake.png");
UI_PNG(img_icon_coolant_24, "dashboard-warning-lights/24x24/coolant-temperature.png");
UI_PNG(img_icon_high_beam_24, "dashboard-warning-lights/24x24/high-beam.png");
UI_PNG(img_icon_check_engine_24, "dashboard-warning-lights/24x24/check-engine.png");
UI_PNG(img_vehicle_truck, "truck-blue/truck-120x90.png");
UI_PNG(img_bar_fuel, "fuel-icon/fuel-icon-32x32.png");
UI_PNG(img_icon_turn_right_32, "generated/turn-right-32.png");
UI_PNG(img_icon_turn_left_32, "generated/turn-left-32.png");
UI_PNG(img_icon_door_unlocked_24, "generated/door-unlocked-24.png");

#endif

const ext_img_desc_t images[30] = {
    { "dashboard_background", &img_dashboard_background },
    { "icon_abs_16", &img_icon_abs_16 },
    { "icon_airbag_16", &img_icon_airbag_16 },
    { "icon_battery_16", &img_icon_battery_16 },
    { "icon_brake_warning_16", &img_icon_brake_warning_16 },
    { "icon_check_engine_16", &img_icon_check_engine_16 },
    { "icon_coolant_16", &img_icon_coolant_16 },
    { "icon_high_beam_16", &img_icon_high_beam_16 },
    { "icon_oil_pressure_16", &img_icon_oil_pressure_16 },
    { "icon_rear_fog_16", &img_icon_rear_fog_16 },
    { "icon_seat_belt_16", &img_icon_seat_belt_16 },
    { "icon_tire_pressure_16", &img_icon_tire_pressure_16 },
    { "icon_traction_control_16", &img_icon_traction_control_16 },
    { "menu_defult_16", &img_menu_defult_16 },
    { "menu_vehicle_16", &img_menu_vehicle_16 },
    { "menu_trip_meter_16", &img_menu_trip_meter_16 },
    { "menu_driving_assistance_16", &img_menu_driving_assistance_16 },
    { "menu_fuel_level_16", &img_menu_fuel_level_16 },
    { "meunu_settings_16", &img_meunu_settings_16 },
    { "icon_seat_belt_24", &img_icon_seat_belt_24 },
    { "icon_brake_warning_24", &img_icon_brake_warning_24 },
    { "icon_parking_break_24", &img_icon_parking_break_24 },
    { "icon_coolant_24", &img_icon_coolant_24 },
    { "icon_high_beam_24", &img_icon_high_beam_24 },
    { "icon_check_engine_24", &img_icon_check_engine_24 },
    { "vehicle_truck", &img_vehicle_truck },
    { "bar_fuel", &img_bar_fuel },
    { "icon_turn_right_32", &img_icon_turn_right_32 },
    { "icon_turn_left_32", &img_icon_turn_left_32 },
    { "icon_door_unlocked_24", &img_icon_door_unlocked_24 },
};
