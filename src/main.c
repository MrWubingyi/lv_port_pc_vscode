/**
 * @file main.c
 *
 */

/*********************
 *      INCLUDES
 *********************/

#ifndef _DEFAULT_SOURCE
  #define _DEFAULT_SOURCE /* needed for usleep() */
#endif

#include "ui.h"
#include "ui_bridge.h"
#include "vehicle_data.h"
#include "vehicle_tcp_server.h"

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#ifdef _MSC_VER
  #include <Windows.h>
#else
  #include <unistd.h>
  #include <pthread.h>
#endif
#include "lvgl/lvgl.h"
#include "lvgl/examples/lv_examples.h"
#include "lvgl/demos/lv_demos.h"
#include <SDL.h>

#include "hal/hal.h"

static volatile sig_atomic_t application_running = 1;

static void request_shutdown(int signal_number)
{
  (void)signal_number;
  application_running = 0;
}

static void main_display_delete_event(lv_event_t *event)
{
  LV_UNUSED(event);
  ui_bridge_deinit();
  application_running = 0;
}

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

#if LV_USE_OS != LV_OS_FREERTOS

int main(int argc, char **argv)
{
  (void)argc; /*Unused*/
  (void)argv; /*Unused*/

  /*Initialize LVGL*/
  lv_init();

  /*Initialize the HAL (display, input devices, tick) for LVGL*/
  lv_display_t *main_display = sdl_hal_init(800, 480);
  if (main_display == NULL) {
    fprintf(stderr, "Failed to create main display\n");
    return 1;
  }
  lv_display_add_event_cb(main_display, main_display_delete_event,
                          LV_EVENT_DELETE, NULL);

  /* Run the default demo */
  /* To try a different demo or example, replace this with one of: */
  /* - lv_demo_benchmark(); */
  /* - lv_demo_stress(); */
  /* - lv_example_label_1(); */
  /* - etc. */
  // lv_demo_widgets();
  vehicle_data_t *vehicle_data = vehicle_data_create();
  if (vehicle_data == NULL) {
    fprintf(stderr, "Failed to create vehicle data model\n");
    return 1;
  }
  signal(SIGINT, request_shutdown);
  signal(SIGTERM, request_shutdown);
  /* Create the EEZ Studio generated UI, then connect it to vehicle data. */
  ui_init();
  ui_bridge_init(vehicle_data);

  if (!vehicle_tcp_server_start(19090, vehicle_data)) {
    fprintf(stderr, "Failed to start TCP server\n");
  }
  while(application_running) {
    ui_tick();
    /* Periodically call the lv_task handler.
     * It could be done in a timer interrupt or an OS task too.*/
    uint32_t sleep_time_ms = lv_timer_handler();
    if(sleep_time_ms == LV_NO_TIMER_READY){
	sleep_time_ms =  LV_DEF_REFR_PERIOD;
    }
#ifdef _MSC_VER
    Sleep(sleep_time_ms);
#else
    usleep(sleep_time_ms * 1000);
#endif
  }

  vehicle_tcp_server_stop();
  vehicle_data_destroy(vehicle_data);

  return 0;
}


#endif

/**********************
 *   STATIC FUNCTIONS
 **********************/
