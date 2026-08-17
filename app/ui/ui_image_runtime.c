#include "ui_image_runtime.h"
#include "images.h"
#include "lvgl/src/misc/cache/instance/lv_image_cache.h"

#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

#ifndef UI_ASSET_FS_ROOT
#define UI_ASSET_FS_ROOT "assets"
#endif

#define IMAGE_COUNT 30
#define BINDING_COUNT_MAX 64

typedef struct {
    char current[UI_IMAGE_RUNTIME_PATH_MAX];
    char pending[UI_IMAGE_RUNTIME_PATH_MAX];
    char last_error[128];
    bool has_pending;
} runtime_entry_t;

typedef struct {
    lv_obj_t *object;
    size_t image_index;
} image_binding_t;

static pthread_mutex_t runtime_mutex = PTHREAD_MUTEX_INITIALIZER;
static runtime_entry_t runtime_entries[IMAGE_COUNT];
static image_binding_t bindings[BINDING_COUNT_MAX];
static size_t binding_count;
static bool initialized;

static void image_delete_event(lv_event_t *event)
{
    lv_obj_t *object = lv_event_get_target(event);
    pthread_mutex_lock(&runtime_mutex);
    for (size_t i = 0; i < binding_count;) {
        if (bindings[i].object == object) {
            bindings[i] = bindings[--binding_count];
        } else {
            ++i;
        }
    }
    pthread_mutex_unlock(&runtime_mutex);
}

static void set_text(char *dst, size_t size, const char *text)
{
    if (size == 0) return;
    snprintf(dst, size, "%s", text != NULL ? text : "");
}

static int source_index(const void *source)
{
    for (size_t i = 0; i < IMAGE_COUNT; ++i) {
#if UI_USE_PNG_ASSETS
        if (source != NULL && strcmp((const char *)source,
                                     (const char *)images[i].img_dsc) == 0)
            return (int)i;
#else
        if (source == images[i].img_dsc) return (int)i;
#endif
    }
    return -1;
}

void ui_image_runtime_init(void)
{
    pthread_mutex_lock(&runtime_mutex);
    if (!initialized) {
        memset(runtime_entries, 0, sizeof(runtime_entries));
#if UI_USE_PNG_ASSETS
        for (size_t i = 0; i < IMAGE_COUNT; ++i)
            set_text(runtime_entries[i].current,
                     sizeof(runtime_entries[i].current),
                     (const char *)images[i].img_dsc);
#endif
        binding_count = 0;
        initialized = true;
    }
    pthread_mutex_unlock(&runtime_mutex);
}

void ui_image_runtime_deinit(void)
{
    pthread_mutex_lock(&runtime_mutex);
    binding_count = 0;
    pthread_mutex_unlock(&runtime_mutex);
}

void ui_image_runtime_bind(lv_obj_t *image, const void *default_source)
{
    int index = source_index(default_source);
    const void *source = default_source;
    if (index >= 0) {
        pthread_mutex_lock(&runtime_mutex);
        if (binding_count < BINDING_COUNT_MAX) {
            bindings[binding_count++] = (image_binding_t){image, (size_t)index};
            lv_obj_add_event_cb(image, image_delete_event, LV_EVENT_DELETE, NULL);
        }
#if UI_USE_PNG_ASSETS
        source = runtime_entries[index].current;
#endif
        pthread_mutex_unlock(&runtime_mutex);
    }
    lv_image_set_src(image, source);
}

static bool is_path_inside(const char *path, const char *root)
{
    size_t length = strlen(root);
    return strncmp(path, root, length) == 0 &&
           (path[length] == '/' || path[length] == '\0');
}

static bool normalize_png_path(const char *input, char *output,
                               size_t output_size, char *error,
                               size_t error_size)
{
    char root[PATH_MAX];
    char candidate[PATH_MAX];
    char resolved[PATH_MAX];
    const char *plain = input;
    if (input == NULL || input[0] == '\0') {
        set_text(error, error_size, "path is required");
        return false;
    }
    if (strncmp(plain, "A:", 2) == 0) plain += 2;
    if (realpath(UI_ASSET_FS_ROOT, root) == NULL) {
        set_text(error, error_size, "asset root is unavailable");
        return false;
    }
    if (plain[0] == '/') {
        set_text(candidate, sizeof(candidate), plain);
    } else if (snprintf(candidate, sizeof(candidate), "%s/%s", root, plain) >=
               (int)sizeof(candidate)) {
        set_text(error, error_size, "path is too long");
        return false;
    }
    if (realpath(candidate, resolved) == NULL) {
        snprintf(error, error_size, "file not found: %s", strerror(errno));
        return false;
    }
    if (!is_path_inside(resolved, root)) {
        set_text(error, error_size, "path must be inside the assets directory");
        return false;
    }
    struct stat file_stat;
    if (stat(resolved, &file_stat) != 0 || !S_ISREG(file_stat.st_mode)) {
        set_text(error, error_size, "path is not a regular file");
        return false;
    }
    const char *extension = strrchr(resolved, '.');
    if (extension == NULL || strcasecmp(extension, ".png") != 0) {
        set_text(error, error_size, "only PNG files are supported");
        return false;
    }
    unsigned char signature[8];
    FILE *file = fopen(resolved, "rb");
    bool png = file != NULL && fread(signature, 1, sizeof(signature), file) ==
                               sizeof(signature) &&
               memcmp(signature, "\x89PNG\r\n\x1a\n", sizeof(signature)) == 0;
    if (file != NULL) fclose(file);
    if (!png) {
        set_text(error, error_size, "file does not have a valid PNG signature");
        return false;
    }
    if (snprintf(output, output_size, "A:%s", resolved) >= (int)output_size) {
        set_text(error, error_size, "path is too long");
        return false;
    }
    return true;
}

bool ui_image_runtime_request_change(const char *name, const char *path,
                                     char *normalized_path,
                                     size_t normalized_path_size,
                                     char *error, size_t error_size)
{
#if !UI_USE_PNG_ASSETS
    (void)name; (void)path; (void)normalized_path; (void)normalized_path_size;
    set_text(error, error_size, "runtime replacement requires UI_USE_PNG_ASSETS=ON");
    return false;
#else
    size_t index = IMAGE_COUNT;
    for (size_t i = 0; i < IMAGE_COUNT; ++i) {
        if (name != NULL && strcmp(name, images[i].name) == 0) {
            index = i;
            break;
        }
    }
    if (index == IMAGE_COUNT) {
        set_text(error, error_size, "unknown image name");
        return false;
    }
    char normalized[UI_IMAGE_RUNTIME_PATH_MAX];
    if (!normalize_png_path(path, normalized, sizeof(normalized), error,
                            error_size)) return false;
    pthread_mutex_lock(&runtime_mutex);
    set_text(runtime_entries[index].pending,
             sizeof(runtime_entries[index].pending), normalized);
    runtime_entries[index].has_pending = true;
    runtime_entries[index].last_error[0] = '\0';
    pthread_mutex_unlock(&runtime_mutex);
    set_text(normalized_path, normalized_path_size, normalized);
    return true;
#endif
}

void ui_image_runtime_process_pending(void)
{
#if UI_USE_PNG_ASSETS
    for (size_t i = 0; i < IMAGE_COUNT; ++i) {
        char next[UI_IMAGE_RUNTIME_PATH_MAX];
        char previous[UI_IMAGE_RUNTIME_PATH_MAX];
        pthread_mutex_lock(&runtime_mutex);
        bool pending = runtime_entries[i].has_pending;
        if (pending) {
            set_text(next, sizeof(next), runtime_entries[i].pending);
            set_text(previous, sizeof(previous), runtime_entries[i].current);
            runtime_entries[i].has_pending = false;
            runtime_entries[i].pending[0] = '\0';
        }
        pthread_mutex_unlock(&runtime_mutex);
        if (!pending) continue;

        lv_image_header_t header;
        lv_result_t result = lv_image_decoder_get_info(next, &header);
        if (result != LV_RESULT_OK) {
            pthread_mutex_lock(&runtime_mutex);
            set_text(runtime_entries[i].last_error,
                     sizeof(runtime_entries[i].last_error),
                     "LVGL could not decode the PNG");
            pthread_mutex_unlock(&runtime_mutex);
            continue;
        }
        if (previous[0] != '\0') lv_image_cache_drop(previous);
        for (size_t j = 0; j < binding_count; ++j) {
            if (bindings[j].image_index == i && bindings[j].object != NULL)
                lv_image_set_src(bindings[j].object, next);
        }
        pthread_mutex_lock(&runtime_mutex);
        set_text(runtime_entries[i].current,
                 sizeof(runtime_entries[i].current), next);
        runtime_entries[i].last_error[0] = '\0';
        pthread_mutex_unlock(&runtime_mutex);
    }
#endif
}

size_t ui_image_runtime_count(void) { return IMAGE_COUNT; }

bool ui_image_runtime_get(size_t index, ui_image_runtime_info_t *info)
{
    if (index >= IMAGE_COUNT || info == NULL) return false;
    pthread_mutex_lock(&runtime_mutex);
    memset(info, 0, sizeof(*info));
    set_text(info->name, sizeof(info->name), images[index].name);
    set_text(info->path, sizeof(info->path), runtime_entries[index].current);
    if (runtime_entries[index].has_pending)
        set_text(info->pending_path, sizeof(info->pending_path),
                 runtime_entries[index].pending);
    set_text(info->last_error, sizeof(info->last_error),
             runtime_entries[index].last_error);
    pthread_mutex_unlock(&runtime_mutex);
    return true;
}
