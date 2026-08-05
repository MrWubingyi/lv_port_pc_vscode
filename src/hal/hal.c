#include "hal.h"

#include <stdlib.h>

typedef struct {
  lv_indev_t *mouse;
  lv_indev_t *mousewheel;
  lv_indev_t *keyboard;
} sdl_hal_input_set_t;

static void sdl_hal_display_delete_cb(lv_event_t *event)
{
  sdl_hal_input_set_t *inputs = lv_event_get_user_data(event);
  if(inputs == NULL) return;
  if(inputs->mouse != NULL) lv_indev_delete(inputs->mouse);
  if(inputs->mousewheel != NULL) lv_indev_delete(inputs->mousewheel);
  if(inputs->keyboard != NULL) lv_indev_delete(inputs->keyboard);
  free(inputs);
}

lv_display_t * sdl_hal_add_display(int32_t w, int32_t h, const char *title)
{
  lv_display_t * previous = lv_display_get_default();
  lv_display_t * disp = lv_sdl_window_create(w, h);
  if(disp == NULL) return NULL;

  if(title != NULL) lv_sdl_window_set_title(disp, title);

  lv_group_t * group = lv_group_create();
  lv_indev_t * mouse = lv_sdl_mouse_create();
  lv_indev_set_group(mouse, group);
  lv_indev_set_display(mouse, disp);

  LV_IMAGE_DECLARE(mouse_cursor_icon);
  lv_obj_t * cursor_obj = lv_image_create(lv_display_get_screen_active(disp));
  lv_image_set_src(cursor_obj, &mouse_cursor_icon);
  lv_indev_set_cursor(mouse, cursor_obj);

  lv_indev_t * mousewheel = lv_sdl_mousewheel_create();
  lv_indev_set_display(mousewheel, disp);
  lv_indev_set_group(mousewheel, group);

  lv_indev_t * kb = lv_sdl_keyboard_create();
  lv_indev_set_display(kb, disp);
  lv_indev_set_group(kb, group);

  sdl_hal_input_set_t *inputs = malloc(sizeof(*inputs));
  if(inputs != NULL) {
    inputs->mouse = mouse;
    inputs->mousewheel = mousewheel;
    inputs->keyboard = kb;
    lv_display_add_event_cb(disp, sdl_hal_display_delete_cb,
                            LV_EVENT_DELETE, inputs);
  }

  if(previous != NULL) lv_display_set_default(previous);
  return disp;
}

lv_display_t * sdl_hal_init(int32_t w, int32_t h)
{
  lv_display_t * disp = sdl_hal_add_display(w, h, "Dashboard Simulator");
  if(disp == NULL) return NULL;
  lv_display_set_default(disp);
  return disp;
}
