#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl/lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// Screens

enum ScreensEnum {
    _SCREEN_ID_FIRST = 1,
    SCREEN_ID_MAIN = 1,
    _SCREEN_ID_LAST = 1
};

typedef struct _objects_t {
    lv_obj_t *main;
    lv_obj_t *img_background;
    lv_obj_t *panel_top_menu;
    lv_obj_t *menu_item_defult;
    lv_obj_t *menu_item_vehicle;
    lv_obj_t *menu_item_trip_meter;
    lv_obj_t *menu_item_driving_assistance;
    lv_obj_t *menu_item_fuel_level;
    lv_obj_t *menu_item_settings;
    lv_obj_t *panel_left_warning;
    lv_obj_t *panel_right_warning;
    lv_obj_t *obj0;
    lv_obj_t *panel_left_info;
    lv_obj_t *panel_left_gauge;
    lv_obj_t *line_left_main;
    lv_obj_t *line_left_active;
    lv_obj_t *line_left_tick_80;
    lv_obj_t *line_left_tick_60;
    lv_obj_t *line_left_tick_40;
    lv_obj_t *line_left_tick_20;
    lv_obj_t *line_left_tick_0;
    lv_obj_t *lbl_left_80;
    lv_obj_t *lbl_left_60;
    lv_obj_t *lbl_left_40;
    lv_obj_t *lbl_left_20;
    lv_obj_t *lbl_left_0;
    lv_obj_t *dot_left_outer;
    lv_obj_t *dot_left_inner;
    lv_obj_t *lbl_left_unit;
    lv_obj_t *panel_center;
    lv_obj_t *lbl_speed;
    lv_obj_t *lbl_speed_unit;
    lv_obj_t *lbl_gear;
    lv_obj_t *lbl_status_title;
    lv_obj_t *img_vehicle;
    lv_obj_t *lbl_load_current;
    lv_obj_t *lbl_load_max;
    lv_obj_t *bar_load;
    lv_obj_t *lbl_load_unit;
    lv_obj_t *panel_right_gauge;
    lv_obj_t *line_right_main;
    lv_obj_t *line_right_red_zone;
    lv_obj_t *line_rigth_active;
    lv_obj_t *line_right_tick_20;
    lv_obj_t *line_right_tick_15;
    lv_obj_t *line_right_tick_10;
    lv_obj_t *line_right_tick_5;
    lv_obj_t *line_right_tick_0;
    lv_obj_t *lbl_right_20;
    lv_obj_t *lbl_right_15;
    lv_obj_t *lbl_right_10;
    lv_obj_t *lbl_right_5;
    lv_obj_t *lbl_right_0;
    lv_obj_t *dot_right_outer;
    lv_obj_t *obj1;
    lv_obj_t *lbl_right_unit;
    lv_obj_t *panel_right_info;
    lv_obj_t *panel_bottom;
    lv_obj_t *img_fuel;
    lv_obj_t *bar_fuel;
    lv_obj_t *lbl_range;
    lv_obj_t *lbl_time;
    lv_obj_t *lbl_outside_temperature;
    lv_obj_t *lbl_trip_distance;
    lv_obj_t *img_bottom_status_1;
    lv_obj_t *img_bottom_status_2;
    lv_obj_t *panel_warning_overlay;
} objects_t;

extern objects_t objects;

void create_screen_main();
void tick_screen_main();

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/