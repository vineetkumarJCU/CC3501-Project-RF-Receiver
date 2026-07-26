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
#include "GUI_hardware_link.h"
#include "radio_gui.h"
#include "SD_card_log.h"
#include "signal_process.h"

////////////////////////////////////////////////////////////////

#define LVGL_TASK_STACK_WORDS 2048u
#define LVGL_TASK_PRIORITY (tskIDLE_PRIORITY + 4U)

#define GPS_TASK_STACK_WORDS 1024u
#define GPS_TASK_PRIORITY (tskIDLE_PRIORITY + 4U)

#define SD_CARD_TASK_STACK_WORDS 1024u
#define SD_CARD_TASK_PRIORITY (tskIDLE_PRIORITY + 2U)

#define SAMPLE_RX_SIGNAL_TASK_STACK_WORDS 1024u
#define SAMPLE_RX_SIGNAL_TASK_PRIORITY (tskIDLE_PRIORITY + 3U)

#define WAVEFORM_QUEUE_LENGTH 1U

typedef signal_process_result_t waveform_queue_item_t;

_Static_assert(SIGNAL_PROCESS_SAMPLE_COUNT == RADIO_GUI_WAVEFORM_POINT_COUNT,
               "Signal processing and GUI waveform lengths must match");

static QueueHandle_t waveform_queue;
static SemaphoreHandle_t sample_request_semaphore;
static SemaphoreHandle_t adc_access_mutex;
static SemaphoreHandle_t gps_page_active_semaphore;

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

  TickType_t last_rsq_update = xTaskGetTickCount();
  TickType_t last_battery_update = xTaskGetTickCount();
  TickType_t last_waveform_update = xTaskGetTickCount();
  TickType_t last_gps_update = xTaskGetTickCount();
  uint32_t displayed_gps_frame = 0U;
  bool gps_page_was_active = false;

  for (;;)
  {
    TickType_t now = xTaskGetTickCount();
    radio_gui_page_t current_page = radio_gui_get_current_page(gui);
    bool gps_page_is_active = current_page == RADIO_GUI_PAGE_GPS;

    radio_gui_apply_sd_logging_state(gui);

    if (gps_page_is_active != gps_page_was_active)
    {
      if (gps_page_is_active)
      {
        (void)xSemaphoreGive(gps_page_active_semaphore);
      }
      else
      {
        (void)xSemaphoreTake(gps_page_active_semaphore, 0);
      }
      gps_page_was_active = gps_page_is_active;
    }

    if ((now - last_rsq_update) >= pdMS_TO_TICKS(500))
    {
      last_rsq_update = now;
      radio_gui_sample_si4732_rsq_info(gui,
                                       current_page == RADIO_GUI_PAGE_RADIO);
    }

    if (current_page == RADIO_GUI_PAGE_HARDWARE)
    {
      if ((now - last_battery_update) >= pdMS_TO_TICKS(5000))
      {
        last_battery_update = now;
        if (xSemaphoreTake(adc_access_mutex, portMAX_DELAY) == pdTRUE)
        {
          radio_gui_update_battery_voltage(gui);
          xSemaphoreGive(adc_access_mutex);
        }
      }
    }
    else if (current_page == RADIO_GUI_PAGE_WAVEFORM)
    {
      if ((now - last_waveform_update) >= pdMS_TO_TICKS(20))
      {
        static waveform_queue_item_t waveform;

        last_waveform_update = now;

        /* Request one ADC DMA capture. A binary semaphore prevents requests
         * from accumulating if the previous capture is still in progress. */
        xSemaphoreGive(sample_request_semaphore);

        if (xQueueReceive(waveform_queue, &waveform, 0) == pdPASS)
        {
          radio_gui_update_waveform(gui,
                                    waveform.time_domain,
                                    waveform.log_magnitude_db);
        }
      }
    }
    else if (current_page == RADIO_GUI_PAGE_GPS)
    {
      if ((now - last_gps_update) >= pdMS_TO_TICKS(100))
      {
        static gps_info_t gps_info;

        last_gps_update = now;
        if (GPS_module_get_latest_info(&gps_info) &&
            gps_info.frame_number != displayed_gps_frame)
        {
          displayed_gps_frame = gps_info.frame_number;
          radio_gui_update_gps_info(gui, &gps_info);
        }
      }
    }
    else
    {
    }

    lv_timer_handler();
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

static void gps_task(void *parameters)
{
  (void)parameters;

  GPS_module_init();
  static gps_info_t gps_info;

  for (;;)
  {
    bool frame_published = GPS_module_process();
    if (frame_published &&
        uxSemaphoreGetCount(gps_page_active_semaphore) > 0U &&
        GPS_module_get_latest_info(&gps_info))
    {
      GPS_module_log_all_info(&gps_info);
    }

    vTaskDelay(pdMS_TO_TICKS(2));
  }
}

static void sd_card_report_state_if_changed(sd_card_log_state_t *last_state,
                                            bool *last_card_inserted)
{
  sd_card_log_state_t state = sd_card_log_get_state();
  bool card_inserted = sd_card_log_card_is_inserted();
  radio_gui_sd_status_t gui_status;

  if (last_state == NULL || last_card_inserted == NULL ||
      (state == *last_state && card_inserted == *last_card_inserted))
  {
    return;
  }

  if (!card_inserted)
  {
    gui_status = RADIO_GUI_SD_NO_CARD;
  }
  else if (state == SD_CARD_LOG_STATE_ERROR)
  {
    gui_status = RADIO_GUI_SD_ERROR;
  }
  else
  {
    gui_status = RADIO_GUI_SD_OK;
  }

  printf("[SD TASK] State=%s, card=%s, logging=%s\n",
         sd_card_log_state_string(state),
         card_inserted ? "inserted" : "not inserted",
         sd_card_log_is_active() ? "active" : "stopped");
  radio_gui_report_sd_logging_state(gui_status, sd_card_log_is_active());
  *last_state = state;
  *last_card_inserted = card_inserted;
}

static void sd_card_task(void *parameters)
{
  static gps_info_t gps_info;
  static sd_card_log_rsq_info_t rsq_info;
  sd_card_log_state_t last_state = (sd_card_log_state_t)-1;
  bool last_card_inserted = !sd_card_log_card_is_inserted();

  (void)parameters;
  printf("[SD TASK] Started on Core 0\n");

  for (;;)
  {
    bool logging_requested;

    if (radio_gui_take_sd_logging_request(&logging_requested))
    {
      if (logging_requested)
      {
        if (sd_card_log_is_active())
        {
          printf("[SD TASK] Start request ignored: logging is already active\n");
        }
        else
        {
          sd_card_log_result_t result = sd_card_log_start();
          printf("[SD TASK] Start request completed with result=%d\n", (int)result);
        }
      }
      else
      {
        sd_card_log_result_t result = sd_card_log_stop();
        printf("[SD TASK] Stop request completed with result=%d\n", (int)result);
      }
    }

    if (sd_card_log_record_due())
    {
      const gps_info_t *gps_snapshot =
          GPS_module_get_latest_info(&gps_info) ? &gps_info : NULL;
      const sd_card_log_rsq_info_t *rsq_snapshot =
          radio_gui_get_latest_si4732_rsq_info(&rsq_info) ? &rsq_info : NULL;
      sd_card_log_result_t result = sd_card_log_append(gps_snapshot, rsq_snapshot);
      if (result != SD_CARD_LOG_OK)
      {
        printf("[SD TASK] Periodic append failed with result=%d\n", (int)result);
      }
    }

    sd_card_report_state_if_changed(&last_state, &last_card_inserted);
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

static void sample_rx_signal_task(void *parameters)
{
  (void)parameters;
  static waveform_queue_item_t waveform_item;

  adc_dma_capture_init();
  for (;;)
  {
    if (xSemaphoreTake(sample_request_semaphore, portMAX_DELAY) != pdTRUE)
    {
      continue;
    }

    if (xSemaphoreTake(adc_access_mutex, portMAX_DELAY) != pdTRUE)
    {
      continue;
    }

    if (!adc_dma_capture_start_with_callback(NULL))
    {
      xSemaphoreGive(adc_access_mutex);
      continue;
    }

    while (!adc_dma_capture_is_done())
    {
      vTaskDelay(pdMS_TO_TICKS(1));
    }

    const adc_dma_capture_sample_t *captured_samples = adc_dma_capture_get_samples();
    xSemaphoreGive(adc_access_mutex);

    (void)signal_process_run(captured_samples,
                             RADIO_GUI_SPECTRUM_MIN_DB,
                             &waveform_item);

    /* Send the latest processed time-domain frame and logarithmic spectrum. */
    xQueueOverwrite(waveform_queue, &waveform_item);
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

  waveform_queue = xQueueCreate(WAVEFORM_QUEUE_LENGTH,
                                sizeof(waveform_queue_item_t));
  if (waveform_queue == NULL)
  {
    freertos_fatal_error("Waveform queue creation failed");
  }

  sample_request_semaphore = xSemaphoreCreateBinary();
  if (sample_request_semaphore == NULL)
  {
    freertos_fatal_error("Sample request semaphore creation failed");
  }

  gps_page_active_semaphore = xSemaphoreCreateBinary();
  if (gps_page_active_semaphore == NULL)
  {
    freertos_fatal_error("GPS page semaphore creation failed");
  }

  adc_access_mutex = xSemaphoreCreateMutex();
  if (adc_access_mutex == NULL)
  {
    freertos_fatal_error("ADC mutex creation failed");
  }

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

  result = xTaskCreateAffinitySet(gps_task,
                                  "gps_task",
                                  GPS_TASK_STACK_WORDS,
                                  NULL,
                                  GPS_TASK_PRIORITY,
                                  (1U << 0), // Core 0
                                  NULL);
  if (result != pdPASS)
  {
    freertos_fatal_error("gps task creation failed");
  }

  result = xTaskCreateAffinitySet(sd_card_task,
                                  "sd_card_task",
                                  SD_CARD_TASK_STACK_WORDS,
                                  NULL,
                                  SD_CARD_TASK_PRIORITY,
                                  (1U << 0), // Core 0
                                  NULL);
  if (result != pdPASS)
  {
    freertos_fatal_error("sd_card task creation failed");
  }

  result = xTaskCreateAffinitySet(sample_rx_signal_task,
                                  "sample_rx_signal_task",
                                  SAMPLE_RX_SIGNAL_TASK_STACK_WORDS,
                                  NULL,
                                  SAMPLE_RX_SIGNAL_TASK_PRIORITY,
                                  (1U << 0), // Core 0
                                  NULL);
  if (result != pdPASS)
  {
    freertos_fatal_error("sample_rx_signal task creation failed");
  }

  vTaskStartScheduler();
  freertos_fatal_error("scheduler returned");
  return 0;
}
