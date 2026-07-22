#include "dashboard.h"
#include <lvgl/core/lv_obj_style.h>
#include <lvgl/core/lv_obj_style_gen.h>
#include <lvgl/draw/lv_color.h>
#include <lvgl/widgets/lv_image.h>
#include <stdint.h>

static lv_obj_t *speed_label;
static lv_obj_t *gear_label;
static lv_obj_t *status_icon;
static lv_timer_t *simulation_timer;

LV_FONT_DECLARE(font_speed_128);
LV_FONT_DECLARE(font_unit_28);
LV_FONT_DECLARE(font_gear_80);
LV_IMAGE_DECLARE(icon_status);

const struct dashboard_status_t dashboard_status[] = {
    {dashboard_status_normal, 0x38e07b},
    {dashboard_status_warning, 0xFF8C00},
    {dashboard_status_error, 0xDC143C},
    {dashboard_status_offline, 0x708090},
};

typedef struct {
    uint32_t tick;
    int speed;
} dashboard_simulation_t;

static dashboard_simulation_t simulation = {
    .tick = 0,
    .speed = 0
};

static void dashboard_simulation_timer_cb(lv_timer_t *timer)
{
    LV_UNUSED(timer);

    uint32_t tick = simulation.tick;
    int speed = 0;
    char gear = 'P';
    enum dashboard_status_enum status;

    /*
     * 0～9：P 挡静止
     * 持续约 1 秒
     */
    if(tick < 10) {
        speed = 0;
        gear = 'P';
        status = dashboard_status_normal;
    }

    /*
     * 10～69：D 挡加速
     * 速度从 0 增加到约 118 km/h
     */
    else if(tick < 70) {
        speed = (int)(tick - 10) * 2;
        gear = 'D';

        if(speed < 70) {
            status = dashboard_status_normal;
        }
        else if(speed < 100) {
            status = dashboard_status_warning;
        }
        else {
            status = dashboard_status_error;
        }
    }

    /*
     * 70～129：D 挡减速
     * 速度从约 118 降低到 0
     */
    else if(tick < 130) {
        speed = (int)(129 - tick) * 2;
        gear = 'D';

        if(speed < 70) {
            status = dashboard_status_normal;
        }
        else if(speed < 100) {
            status = dashboard_status_warning;
        }
        else {
            status = dashboard_status_error;
        }
    }

    /*
     * 130～139：N 挡等待
     */
    else if(tick < 140) {
        speed = 0;
        gear = 'N';
        status = dashboard_status_warning;
    }

    /*
     * 140～159：R 挡倒车
     * 速度从 0 增加到约 19 km/h
     */
    else if(tick < 160) {
        speed = (int)(tick - 140);
        gear = 'R';
        status = dashboard_status_normal;
    }

    /*
     * 160～169：P 挡，模拟离线
     */
    else if(tick < 170) {
        speed = 0;
        gear = 'P';
        status = dashboard_status_offline;
    }

    /*
     * 一轮结束，重新开始
     */
    else {
        simulation.tick = 0;
        simulation.speed = 0;
        return;
    }

    simulation.speed = speed;

    dashboard_set_speed(speed);
    dashboard_set_gear(gear);
    dashboard_set_status(status_icon, status);

    simulation.tick++;
}


void dashboard_start_simulation(void)
{
    if(simulation_timer != NULL) {
        return;
    }

    simulation.tick = 0;
    simulation.speed = 0;

    simulation_timer = lv_timer_create(
        dashboard_simulation_timer_cb,
        100,
        NULL
    );
}
void dashboard_stop_simulation(void)
{
    if(simulation_timer == NULL) {
        return;
    }

    lv_timer_delete(simulation_timer);
    simulation_timer = NULL;
}
uint32_t dashboard_status_get_color(enum dashboard_status_enum status) {
  for (size_t i = 0;
       i < sizeof(dashboard_status) / sizeof(struct dashboard_status_t); i++) {
    if (status == dashboard_status[i].status)
      return dashboard_status[i].color;
  }
  return 0x38e07b;
}

void dashboard_create(void)
{
    lv_obj_t *screen = lv_obj_create(NULL);

    //移除默认样式
    lv_obj_remove_style_all(screen);
    //设置尺寸
    lv_obj_set_size(screen, 320, 480);
    // 设置背景颜色
    lv_obj_set_style_bg_color(
        screen,
        lv_color_hex(0x080C12),
        LV_PART_MAIN
    );
    //设置背景不透明度
    lv_obj_set_style_bg_opa(
        screen,
        LV_OPA_COVER,
        LV_PART_MAIN
    );

    /* 顶部状态图标，先用圆形代替 */
    status_icon = lv_image_create(screen);
    lv_image_set_src(status_icon, &icon_status);
    // lv_obj_set_size(status, 24, 24);
    // // 设置圆角半径
    // lv_obj_set_style_radius(
    //     status,
    //     LV_RADIUS_CIRCLE,
    //     LV_PART_MAIN
    // );
    lv_obj_set_style_image_recolor(status_icon, lv_color_hex(0x38e07b), LV_PART_MAIN);
    lv_obj_set_style_image_recolor_opa(status_icon, LV_OPA_COVER, LV_PART_MAIN);
    lv_image_set_scale(status_icon, 61);
    // lv_obj_set_style_bg_color(
    //     status,
    //     lv_color_hex(0x38E07B),
    //     LV_PART_MAIN
    // );

    // 设置边框宽度
    // lv_obj_set_style_border_width(status, 0, LV_PART_MAIN);
    // 非常重要的定位函数：
    // lv_obj_align(对象, 对齐基准, x偏移, y偏移);
    lv_obj_align(status_icon, LV_ALIGN_TOP_MID, 0, 15);


    /* 速度 */
    speed_label = lv_label_create(screen);
    lv_label_set_text(speed_label, "0");
    lv_obj_set_width(speed_label, 300);

    // 设置 Label 内部的文字对齐方式。
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
        &font_speed_128,
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
    lv_obj_set_style_text_font(
      unit_label,
      &font_unit_28,
      LV_PART_MAIN
    );
    lv_obj_align(unit_label, LV_ALIGN_TOP_MID, 0, 230);

    /* 档位 */
    gear_label = lv_label_create(screen);
    lv_label_set_text(gear_label, "D");
    lv_obj_set_style_text_font(
        gear_label,
        &font_gear_80,
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

void dashboard_set_status(lv_obj_t *obj, enum dashboard_status_enum status) {

  lv_obj_set_style_image_recolor(
      obj, lv_color_hex(dashboard_status_get_color(status)), LV_PART_MAIN);
}
