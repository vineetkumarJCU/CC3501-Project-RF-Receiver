#include "main.h"

/* from FreeRTOS */
#include "FreeRTOS.h"
#include "freertos_hooks.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"

/* from LVGL */
#include "lv_port.h"
#include "lvgl.h"

/* from SDK */
#include "pico/stdlib.h"
#include "stdio.h"

/* from user */
#include "board_init.h"
#include "RP2040_clk_init.h"
#include "RP2040_pwm.h"
#include "RP2040_adc.h"
#include "GPS_module.h"
#include "radio_gui.h"
#include "GUI_hardware_link.h"

////////////////////////////////////////////////////////////////

#define LVGL_TASK_STACK_WORDS 2048u
#define LVGL_TASK_PRIORITY (tskIDLE_PRIORITY + 4U)


static void lvgl_task(void *parameters)
{

  (void)parameters;

  lv_init();
  lv_port_init();

  radio_gui_config_t gui_config = radio_gui_link_hardware_config();
  radio_gui_t *gui = radio_gui_create(lv_screen_active(), &gui_config);
  if (gui == NULL)
  {
    freertos_fatal_error("GUI creation failed");
  }
  printf("GUI created\n");
  radio_gui_startup_hardware(gui);

  

  for (;;)
  {
    lv_timer_handler();
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}


int main(void)
{
  rp2040_clocks_init();
  stdio_init_all();
  board_init();
  RP2040_screen_pwm_init();
  rp2040_clocks_test();
  RP2040_adc_init();

  printf("CC3501 Raspberry Radio Started\n");

  BaseType_t result;

  if (!radio_gui_hardware_link_init())
  {
    freertos_fatal_error("GUI hardware link initialization failed");
  }

  result = xTaskCreateAffinitySet(lvgl_task,
                                  "lvgl",
                                  LVGL_TASK_STACK_WORDS,
                                  NULL,
                                  LVGL_TASK_PRIORITY,
                                  (1U << 1), // Core 1
                                  NULL);
  if (result != pdPASS)
  {
    freertos_fatal_error("LVGL task creation failed");
  }

  vTaskStartScheduler();
  freertos_fatal_error("scheduler returned");
  return 0;
}
