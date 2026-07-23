#include "dashboard.h"
#include <stdbool.h>
#include <stddef.h>
#define MAX_SPEED 200
#define SPEED_STEP 10

static lv_obj_t *speed_label, *gear_label, *status_icon, *speed_arc;
static lv_obj_t *mode_button, *mode_label;
static lv_timer_t *simulation_timer;
static int current_speed;
static bool manual_mode;

/* Screen 使用一列两行 Grid：上方仪表区固定 326 px，下方调试区占用剩余空间。 */
static int32_t screen_col_dsc[] = {LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
static int32_t screen_row_dsc[] = {326, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
LV_FONT_DECLARE(font_speed_128);
LV_FONT_DECLARE(font_speed_100);
LV_FONT_DECLARE(font_unit_28);
LV_FONT_DECLARE(font_gear_80);
LV_IMAGE_DECLARE(icon_status);

const struct dashboard_status_t dashboard_status[] = {
    {dashboard_status_normal, 0x38E07B},
    {dashboard_status_warning, 0xFF8C00},
    {dashboard_status_error, 0xDC143C},
    {dashboard_status_offline, 0x708090}};
typedef struct {
  uint32_t tick;
  int speed;
} sim_t;
static sim_t simulation;
typedef enum { CTRL_SPEED, CTRL_GEAR, CTRL_STATUS } ctrl_kind_t;
typedef struct {
  ctrl_kind_t kind;
  int value;
} ctrl_action_t;
static const ctrl_action_t speed_actions[] = {{CTRL_SPEED, -SPEED_STEP},
                                              {CTRL_SPEED, SPEED_STEP}};
static const ctrl_action_t gear_actions[] = {
    {CTRL_GEAR, 'P'}, {CTRL_GEAR, 'R'}, {CTRL_GEAR, 'N'}, {CTRL_GEAR, 'D'}};
static const ctrl_action_t status_actions[] = {
    {CTRL_STATUS, dashboard_status_normal},
    {CTRL_STATUS, dashboard_status_warning},
    {CTRL_STATUS, dashboard_status_error},
    {CTRL_STATUS, dashboard_status_offline}};

uint32_t dashboard_status_get_color(enum dashboard_status_enum status) {
  size_t n = sizeof(dashboard_status) / sizeof(dashboard_status[0]);
  for (size_t i = 0; i < n; i++)
    if (status == dashboard_status[i].status)
      return dashboard_status[i].color;
  return dashboard_status[0].color;
}

static void dashboard_apply_speed(int speed) {
  if (speed < 0)
    speed = 0;
  if (speed > MAX_SPEED)
    speed = MAX_SPEED;

  current_speed = speed;

  lv_label_set_text_fmt(speed_label, "%d", speed);

  lv_arc_set_value(speed_arc, speed);
}

static void speed_anim_exec_cb(void *obj, int32_t value) {
  LV_UNUSED(obj);
  dashboard_apply_speed((int)value);
}

void dashboard_set_speed(int target_speed) {
  if (target_speed < 0)
    target_speed = 0;
  if (target_speed > MAX_SPEED)
    target_speed = MAX_SPEED;

  lv_anim_delete(speed_arc, speed_anim_exec_cb);

  lv_anim_t animation;
  lv_anim_init(&animation);

  lv_anim_set_var(&animation, speed_arc);

  lv_anim_set_exec_cb(&animation, speed_anim_exec_cb);

  lv_anim_set_values(&animation, current_speed, target_speed);

  lv_anim_set_duration(&animation, 300);

  lv_anim_set_path_cb(&animation, lv_anim_path_ease_out);

  lv_anim_start(&animation);
}

void dashboard_set_gear(char gear) {
  char text[2] = {gear, '\0'};
  /* 为 gear_label 设置显示文本。 */
  lv_label_set_text(gear_label, text);
}
void dashboard_set_status(lv_obj_t *obj, enum dashboard_status_enum status) {
  /* 为 obj 设置图片重着色颜色。 */
  lv_obj_set_style_image_recolor(
      obj, lv_color_hex(dashboard_status_get_color(status)), LV_PART_MAIN);
}
static enum dashboard_status_enum status_for_speed(int speed) {
  return speed < 70 ? dashboard_status_normal
                    : (speed < 100 ? dashboard_status_warning
                                   : dashboard_status_error);
}

static void simulation_cb(lv_timer_t *timer) {
  LV_UNUSED(timer);
  uint32_t t = simulation.tick;
  int speed;
  char gear;
  enum dashboard_status_enum status;
  if (t < 10) {
    speed = 0;
    gear = 'P';
    status = dashboard_status_normal;
  } else if (t < 70) {
    speed = (int)(t - 10) * 2;
    gear = 'D';
    status = status_for_speed(speed);
  } else if (t < 130) {
    speed = (int)(129 - t) * 2;
    gear = 'D';
    status = status_for_speed(speed);
  } else if (t < 140) {
    speed = 0;
    gear = 'N';
    status = dashboard_status_warning;
  } else if (t < 160) {
    speed = (int)(t - 140);
    gear = 'R';
    status = dashboard_status_normal;
  } else if (t < 170) {
    speed = 0;
    gear = 'P';
    status = dashboard_status_offline;
  } else {
    simulation.tick = 0;
    return;
  }
  simulation.speed = speed;
  dashboard_set_speed(speed);
  dashboard_set_gear(gear);
  dashboard_set_status(status_icon, status);
  simulation.tick++;
}
void dashboard_start_simulation(void) {
  /* 创建 100 ms 自动模拟 Timer；保存指针以便暂停和恢复。 */
  if (simulation_timer == NULL)
    simulation_timer = lv_timer_create(simulation_cb, 100, NULL);
  lv_timer_resume(simulation_timer);
}
void dashboard_stop_simulation(void) {
  if (simulation_timer != NULL)
    lv_timer_pause(simulation_timer);
}

static lv_obj_t *button_create(lv_obj_t *parent, const char *text,
                               lv_event_cb_t cb, const void *data) {
  /* 创建可点击 Button，parent 决定按钮属于哪一行 Flex 容器。 */
  lv_obj_t *b = lv_button_create(parent);
  /* 为 b 设置组件宽度和高度。 */
  lv_obj_set_size(b, 44, 30);
  /* 为 b 设置圆角半径。 */
  lv_obj_set_style_radius(b, 6, 0);
  /* 为 b 设置背景颜色。 */
  lv_obj_set_style_bg_color(b, lv_color_hex(0x243044), 0);
  /* 为 b 设置四周内边距。 */
  lv_obj_set_style_pad_all(b, 0, 0);
  lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, (void *)data);
  /* 创建按钮文字 Label；以 Button 为父对象，使二者共享生命周期。 */
  lv_obj_t *l = lv_label_create(b);
  /* 为 l 设置显示文本。 */
  lv_label_set_text(l, text);
  /* 为 l 设置文字颜色。 */
  lv_obj_set_style_text_color(l, lv_color_white(), 0);
  lv_obj_center(l);
  return b;
}
static void control_cb(lv_event_t *e) {
  if (!manual_mode)
    return;
  const ctrl_action_t *a = lv_event_get_user_data(e);
  if (a->kind == CTRL_SPEED) {
    dashboard_set_speed(current_speed + a->value);
    dashboard_set_status(status_icon, status_for_speed(current_speed));
  } else if (a->kind == CTRL_GEAR)
    dashboard_set_gear((char)a->value);
  else
    dashboard_set_status(status_icon, (enum dashboard_status_enum)a->value);
}
static void mode_cb(lv_event_t *e) {
  LV_UNUSED(e);
  manual_mode = !manual_mode;
  if (manual_mode) {
    dashboard_stop_simulation();
    /* 为 mode_label 设置显示文本。 */
    lv_label_set_text(mode_label, "MANUAL");
    /* 为 mode_button 设置背景颜色。 */
    lv_obj_set_style_bg_color(mode_button, lv_color_hex(0xC47A16), 0);
  } else {
    dashboard_start_simulation();
    /* 为 mode_label 设置显示文本。 */
    lv_label_set_text(mode_label, "AUTO");
    /* 为 mode_button 设置背景颜色。 */
    lv_obj_set_style_bg_color(mode_button, lv_color_hex(0x176B45), 0);
  }
}
static lv_obj_t *row_create(lv_obj_t *parent) {
  /* 创建无背景行容器，用 Flex 横向排列同一组控制按钮。 */
  lv_obj_t *r = lv_obj_create(parent);
  lv_obj_remove_style_all(r);
  /* 为 r 设置组件宽度和高度。 */
  lv_obj_set_size(r, 294, 34);
  /* 为 r 设置 Flex 主轴排列方向。 */
  lv_obj_set_flex_flow(r, LV_FLEX_FLOW_ROW);
  /* 为 r 设置 Flex 主轴、交叉轴和轨道对齐方式。 */
  lv_obj_set_flex_align(r, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  return r;
}
static void debug_panel_create(lv_obj_t *screen) {
  /* 创建 PC 调试 Panel，承载模式、速度、档位和状态控制行。 */
  lv_obj_t *p = lv_obj_create(screen);
  /* 为 p 设置组件宽度和高度。 */
  lv_obj_set_size(p, 310, 146);
  /* 将调试 Panel 放入 Screen Grid 的第 2 行，并在单元格内居中。 */
  lv_obj_set_grid_cell(p, LV_GRID_ALIGN_CENTER, 0, 1,
                       LV_GRID_ALIGN_CENTER, 1, 1);
  /* 为 p 设置背景颜色。 */
  lv_obj_set_style_bg_color(p, lv_color_hex(0x111923), 0);
  /* 为 p 设置背景不透明度。 */
  lv_obj_set_style_bg_opa(p, LV_OPA_COVER, 0);
  /* 为 p 设置边框颜色。 */
  lv_obj_set_style_border_color(p, lv_color_hex(0x2B3A4E), 0);
  /* 为 p 设置边框宽度。 */
  lv_obj_set_style_border_width(p, 1, 0);
  /* 为 p 设置圆角半径。 */
  lv_obj_set_style_radius(p, 10, 0);
  /* 为 p 设置四周内边距。 */
  lv_obj_set_style_pad_all(p, 4, 0);
  /* 为 p 设置 Flex 主轴排列方向。 */
  lv_obj_set_flex_flow(p, LV_FLEX_FLOW_COLUMN);
  /* 为 p 设置 Flex 主轴、交叉轴和轨道对齐方式。 */
  lv_obj_set_flex_align(p, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  /* 创建第一行：减速、自动/手动模式、加速按钮。 */
  lv_obj_t *r = row_create(p);
  button_create(r, "-10", control_cb, &speed_actions[0]);
  mode_button = button_create(r, "AUTO", mode_cb, NULL);
  mode_label = lv_obj_get_child(mode_button, 0);
  /* 为 mode_button 设置组件宽度。 */
  lv_obj_set_width(mode_button, 88);
  /* 为 mode_button 设置背景颜色。 */
  lv_obj_set_style_bg_color(mode_button, lv_color_hex(0x176B45), 0);
  button_create(r, "+10", control_cb, &speed_actions[1]);
  /* 创建第二行：P、R、N、D 档位按钮。 */
  r = row_create(p);
  const char *g[] = {"P", "R", "N", "D"};
  for (size_t i = 0; i < 4; i++)
    button_create(r, g[i], control_cb, &gear_actions[i]);
  /* 创建第三行：正常、警告、故障、离线状态按钮。 */
  r = row_create(p);
  const char *s[] = {"OK", "WARN", "ERR", "OFF"};
  for (size_t i = 0; i < 4; i++)
    button_create(r, s[i], control_cb, &status_actions[i]);
}
void dashboard_create(void) {
  /* 创建仪表盘根 Screen；NULL 表示它没有普通父对象。 */
  lv_obj_t *screen = lv_obj_create(NULL);
  lv_obj_remove_style_all(screen);
  /* 为 screen 设置组件宽度和高度。 */
  lv_obj_set_size(screen, 320, 480);
  /* 为 screen 设置背景颜色。 */
  lv_obj_set_style_bg_color(screen, lv_color_hex(0x080C12), 0);
  /* 为 screen 设置背景不透明度。 */
  lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
  /* 为 screen 设置一列两行的 Grid 模板。 */
  lv_obj_set_grid_dsc_array(screen, screen_col_dsc, screen_row_dsc);

  /* 创建仪表区容器，集中承载 Scale、Arc、图标和文字。 */
  lv_obj_t *gauge_area = lv_obj_create(screen);
  lv_obj_remove_style_all(gauge_area);
  /* 将 gauge_area 拉伸填满 Screen Grid 的第 1 行。 */
  lv_obj_set_grid_cell(gauge_area, LV_GRID_ALIGN_STRETCH, 0, 1,
                       LV_GRID_ALIGN_STRETCH, 0, 1);
  /* 为 gauge_area 清除内边距，内部绝对对齐以容器左上角为基准。 */
  lv_obj_set_style_pad_all(gauge_area, 0, 0);

  /* 创建圆形 Scale，显示速度主刻度、次刻度和数字标签。 */
  lv_obj_t *scale = lv_scale_create(gauge_area);
  /* 为 scale 设置组件宽度和高度。 */
  lv_obj_set_size(scale, 260, 260);
  lv_obj_align(scale, LV_ALIGN_TOP_MID, 0, 14);
  /* 为 scale 设置为圆形内刻度模式。 */
  lv_scale_set_mode(scale, LV_SCALE_MODE_ROUND_INNER);
  /* 为 scale 设置刻度数值范围。 */
  lv_scale_set_range(scale, 0, MAX_SPEED);
  /* 为 scale 设置刻度覆盖角度。 */
  lv_scale_set_angle_range(scale, 240);
  /* 为 scale 设置刻度起始旋转角度。 */
  lv_scale_set_rotation(scale, 150);
  /* 为 scale 设置主刻度和次刻度的总数量。 */
  lv_scale_set_total_tick_count(scale, 41);
  /* 为 scale 设置每隔多少个刻度绘制一个主刻度。 */
  lv_scale_set_major_tick_every(scale, 5);
  /* 为 scale 设置是否显示主刻度数字。 */
  lv_scale_set_label_show(scale, true);

  /* 为 scale 设置当前 Part 的刻度线长度。 */
  lv_obj_set_style_length(scale, 5, LV_PART_ITEMS);
  /* 为 scale 设置当前 Part 的线条颜色。 */
  lv_obj_set_style_line_color(scale, lv_color_hex(0x617086), LV_PART_ITEMS);
  /* 为 scale 设置当前 Part 的刻度线长度。 */
  lv_obj_set_style_length(scale, 11, LV_PART_INDICATOR);
  /* 为 scale 设置当前 Part 的线条宽度。 */
  lv_obj_set_style_line_width(scale, 2, LV_PART_INDICATOR);
  /* 为 scale 设置当前 Part 的线条颜色。 */
  lv_obj_set_style_line_color(scale, lv_color_hex(0xC7D0DC), LV_PART_INDICATOR);
  /* 为 scale 设置文字颜色。 */
  lv_obj_set_style_text_color(scale, lv_color_hex(0x9AA8BA), LV_PART_INDICATOR);

  /* 创建只读 Arc，以彩色圆弧长度显示当前速度进度。 */
  speed_arc = lv_arc_create(gauge_area);
  /* 为 speed_arc 设置组件宽度和高度。 */
  lv_obj_set_size(speed_arc, 286, 286);
  lv_obj_align(speed_arc, LV_ALIGN_TOP_MID, 0, 2);
  /* 为 speed_arc 设置圆弧起始旋转角度。 */
  lv_arc_set_rotation(speed_arc, 150);
  /* 为 speed_arc 设置圆弧背景的起止角度。 */
  lv_arc_set_bg_angles(speed_arc, 0, 240);
  /* 为 speed_arc 设置圆弧数值范围。 */
  lv_arc_set_range(speed_arc, 0, MAX_SPEED);
  lv_obj_remove_style(speed_arc, NULL, LV_PART_KNOB);
  /* 为 speed_arc 设置组件是否响应点击。 */
  lv_obj_set_clickable(speed_arc, false);
  /* 为 speed_arc 设置圆弧线宽。 */
  lv_obj_set_style_arc_width(speed_arc, 10, LV_PART_MAIN);
  /* 为 speed_arc 设置圆弧颜色。 */
  lv_obj_set_style_arc_color(speed_arc, lv_color_hex(0x1A2431), LV_PART_MAIN);
  /* 为 speed_arc 设置圆弧线宽。 */
  lv_obj_set_style_arc_width(speed_arc, 10, LV_PART_INDICATOR);
  /* 为 speed_arc 设置圆弧颜色。 */
  lv_obj_set_style_arc_color(speed_arc, lv_color_hex(0x38E07B),
                             LV_PART_INDICATOR);

  /* 创建状态 Image，通过重着色表达正常、警告、故障和离线。 */
  status_icon = lv_image_create(gauge_area);
  /* 为 status_icon 设置图片资源。 */
  lv_image_set_src(status_icon, &icon_status);
  /* 为 status_icon 设置图片重着色不透明度。 */
  lv_obj_set_style_image_recolor_opa(status_icon, LV_OPA_COVER, 0);
  /* 为 status_icon 设置图片缩放比例。 */
  lv_image_set_scale(status_icon, 40);
  lv_obj_align(status_icon, LV_ALIGN_TOP_MID, 0, 75);

  /* 创建速度数字 Label，显示经过边界限制后的当前速度。 */
  speed_label = lv_label_create(gauge_area);
  /* 为 speed_label 设置显示文本。 */
  lv_label_set_text(speed_label, "0");
  /* 为 speed_label 设置组件宽度。 */
  lv_obj_set_width(speed_label, LV_SIZE_CONTENT);
  lv_label_set_long_mode(
    speed_label,
    LV_LABEL_LONG_CLIP
);
  /* 为 speed_label 设置文字对齐方式。 */
  lv_obj_set_style_text_align(speed_label, LV_TEXT_ALIGN_CENTER, 0);
  /* 为 speed_label 设置文字颜色。 */
  lv_obj_set_style_text_color(speed_label, lv_color_white(), 0);
  /* 为 speed_label 设置文字字体。 */
  lv_obj_set_style_text_font(speed_label, &font_speed_100, 0);
  lv_obj_align(speed_label, LV_ALIGN_TOP_MID, 0, 82);

  /* 创建速度单位 Label，以便单独设置字体和颜色。 */
  lv_obj_t *unit = lv_label_create(gauge_area);
  /* 为 unit 设置显示文本。 */
  lv_label_set_text(unit, "km/h");
  /* 为 unit 设置文字颜色。 */
  lv_obj_set_style_text_color(unit, lv_color_hex(0x8A96A8), 0);
  /* 为 unit 设置文字字体。 */
  lv_obj_set_style_text_font(unit, &font_unit_28, 0);
  lv_obj_align(unit, LV_ALIGN_TOP_MID, 0, 200);

  /* 创建档位 Label，显示 P、R、N、D 中的当前档位。 */
  gear_label = lv_label_create(gauge_area);
  /* 为 gear_label 设置显示文本。 */
  lv_label_set_text(gear_label, "P");
  /* 为 gear_label 设置文字字体。 */
  lv_obj_set_style_text_font(gear_label, &font_gear_80, 0);
  /* 为 gear_label 设置文字颜色。 */
  lv_obj_set_style_text_color(gear_label, lv_color_hex(0x38E07B), 0);
  lv_obj_align(gear_label, LV_ALIGN_TOP_MID, 0, 228);

  /* 创建屏幕底部的 PC 调试控制区。 */
  debug_panel_create(screen);
  lv_screen_load(screen);
}
