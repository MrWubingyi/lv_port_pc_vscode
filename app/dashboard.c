#include "dashboard.h"

static lv_obj_t *speed_label;
static lv_obj_t *gear_label;

void dashboard_create(void)
{
    lv_obj_t *screen = lv_obj_create(NULL);

    lv_obj_remove_style_all(screen);
    lv_obj_set_size(screen, 320, 480);
    lv_obj_set_style_bg_color(
        screen,
        lv_color_hex(0x080C12),
        LV_PART_MAIN
    );
    lv_obj_set_style_bg_opa(
        screen,
        LV_OPA_COVER,
        LV_PART_MAIN
    );

    /* 顶部状态图标，先用圆形代替 */
    lv_obj_t *status = lv_obj_create(screen);
    lv_obj_set_size(status, 24, 24);
    lv_obj_set_style_radius(
        status,
        LV_RADIUS_CIRCLE,
        LV_PART_MAIN
    );
    lv_obj_set_style_bg_color(
        status,
        lv_color_hex(0x38E07B),
        LV_PART_MAIN
    );
    lv_obj_set_style_border_width(status, 0, LV_PART_MAIN);
    lv_obj_align(status, LV_ALIGN_TOP_MID, 0, 35);

    /* 速度 */
    speed_label = lv_label_create(screen);
    lv_label_set_text(speed_label, "88");
    lv_obj_set_width(speed_label, 300);
    lv_obj_set_style_text_align(
        speed_label,
        LV_TEXT_ALIGN_CENTER,
        LV_PART_MAIN
    );
    lv_obj_set_style_text_color(
        speed_label,
        lv_color_white(),
        LV_PART_MAIN
    );
    lv_obj_set_style_text_font(
        speed_label,
        &lv_font_montserrat_48,
        LV_PART_MAIN
    );
    lv_obj_align(speed_label, LV_ALIGN_TOP_MID, 0, 130);

    /* 单位 */
    lv_obj_t *unit_label = lv_label_create(screen);
    lv_label_set_text(unit_label, "km/h");
    lv_obj_set_style_text_color(
        unit_label,
        lv_color_hex(0x8A96A8),
        LV_PART_MAIN
    );
    lv_obj_align(unit_label, LV_ALIGN_TOP_MID, 0, 230);

    /* 档位 */
    gear_label = lv_label_create(screen);
    lv_label_set_text(gear_label, "D");
    lv_obj_set_style_text_font(
        gear_label,
        &lv_font_montserrat_48,
        LV_PART_MAIN
    );
    lv_obj_set_style_text_color(
        gear_label,
        lv_color_hex(0x38E07B),
        LV_PART_MAIN
    );
    lv_obj_align(gear_label, LV_ALIGN_BOTTOM_MID, 0, -50);

    lv_screen_load(screen);
}

void dashboard_set_speed(int speed)
{
    if(speed < 0) speed = 0;
    if(speed > 999) speed = 999;

    lv_label_set_text_fmt(speed_label, "%d", speed);
}

void dashboard_set_gear(char gear)
{
    char text[2] = {gear, '\0'};
    lv_label_set_text(gear_label, text);
}
