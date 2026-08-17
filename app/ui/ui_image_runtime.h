#ifndef UI_IMAGE_RUNTIME_H
#define UI_IMAGE_RUNTIME_H

#include <lvgl/lvgl.h>
#include <stdbool.h>
#include <stddef.h>

#define UI_IMAGE_RUNTIME_PATH_MAX 1024

typedef struct {
    char name[64];
    char path[UI_IMAGE_RUNTIME_PATH_MAX];
    char pending_path[UI_IMAGE_RUNTIME_PATH_MAX];
    char last_error[128];
} ui_image_runtime_info_t;

void ui_image_runtime_init(void);
void ui_image_runtime_deinit(void);
void ui_image_runtime_bind(lv_obj_t *image, const void *default_source);
void ui_image_runtime_process_pending(void);

bool ui_image_runtime_request_change(const char *name, const char *path,
                                     char *normalized_path,
                                     size_t normalized_path_size,
                                     char *error, size_t error_size);
size_t ui_image_runtime_count(void);
bool ui_image_runtime_get(size_t index, ui_image_runtime_info_t *info);

#endif
