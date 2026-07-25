#include "GUI_hardware_link.h"

#include "FreeRTOS.h"
#include "queue.h"

#include <stdio.h>
#include <string.h>
#include "SI4732.h"
#include "NS4160.h"
#include "GPS_module.h"
#include "arm_math.h"
#include "RP2040_pwm.h"
#include "RP2040_adc.h"
#include "adc_dma_capture.h"
#include "pico/critical_section.h"

typedef struct
{
  radio_gui_sd_status_t status;
  bool logging_active;
} radio_gui_sd_logging_state_t;

static QueueHandle_t sd_logging_request_queue;
static QueueHandle_t sd_logging_state_queue;
static critical_section_t rsq_snapshot_lock;
static sd_card_log_rsq_info_t latest_rsq_snapshot;
static bool rsq_snapshot_available;
static bool hardware_link_initialized;

bool radio_gui_hardware_link_init(void)
{
  return true;
}

static void radio_gui_band_changed_cb(void * context, radio_gui_band_t band)
{
  
}

static bool radio_gui_frequency_submitted_cb(void *context,
                                             const radio_gui_frequency_request_t *request,
                                             char *result_text,
                                             size_t result_text_size)
{
  
  return true;
}

static void radio_gui_volume_changed_cb(void *context, uint8_t volume)
{
  
}


static void radio_gui_channel_filter_changed_cb(void *context, radio_gui_band_t band, const char *filter_name)
{
  
}

static void radio_gui_audio_amp_enabled_changed_cb(void *context, radio_gui_amp_mode_t mode, bool enabled)
{
 
}

static void radio_gui_audio_amp_mode_changed_cb(void *context, radio_gui_amp_mode_t mode, bool enabled)
{
  
}

static void radio_gui_backlight_changed_cb(void *context, uint8_t level)
{
  
}

static void radio_gui_gps_enabled_changed_cb(void *context, bool enabled)
{
  
}

static void radio_gui_sd_logging_changed_cb(void *context, bool enabled)
{
  
}

bool radio_gui_take_sd_logging_request(bool *enabled)
{
  return true;
}

void radio_gui_report_sd_logging_state(radio_gui_sd_status_t status,
                                       bool logging_active)
{
  
}

void radio_gui_apply_sd_logging_state(radio_gui_t *gui)
{
  
}

radio_gui_config_t radio_gui_link_hardware_config(void)
{
    radio_gui_config_t config = {
        .callbacks = {
            .band_changed = radio_gui_band_changed_cb,
            .frequency_submitted = radio_gui_frequency_submitted_cb,
            .volume_changed = radio_gui_volume_changed_cb,
            .channel_filter_changed = radio_gui_channel_filter_changed_cb,
            .audio_amp_enabled_changed = radio_gui_audio_amp_enabled_changed_cb,
            .audio_amp_mode_changed = radio_gui_audio_amp_mode_changed_cb,
            .backlight_changed = radio_gui_backlight_changed_cb,
            .gps_enabled_changed = radio_gui_gps_enabled_changed_cb,
            .sd_logging_changed = radio_gui_sd_logging_changed_cb,
        },
        .callback_context = NULL,
    };
    return config;
}

static void radio_gui_store_rsq_snapshot(const sd_card_log_rsq_info_t *snapshot)
{
  
}

bool radio_gui_get_latest_si4732_rsq_info(sd_card_log_rsq_info_t *rsq_info)
{
  
}

static void radio_gui_clear_rsq_widgets(radio_gui_t *gui)
{

}

void radio_gui_sample_si4732_rsq_info(radio_gui_t *gui, bool update_widgets)
{
  
}

void radio_gui_update_si4732_rsq_info(radio_gui_t *gui)
{
  
}

void radio_gui_update_battery_voltage(radio_gui_t *gui)
{
 
}

void radio_gui_update_waveform(radio_gui_t *gui, const int8_t *time_domain_waveform_data, const int8_t *spec_waveform_data) // RADIO_GUI_WAVEFORM_POINT_COUNT
{

}

static void gps_systems_to_text(uint32_t system_mask, char *text, size_t text_size)
{
  
}

static const char *gps_fix_type_text(uint8_t fix_type)
{

}

static const char *gps_location_mode_text(char mode)
{

}


void radio_gui_update_gps_info(radio_gui_t *gui, const gps_info_t *gps_info)
{
  

}


void radio_gui_startup_hardware(radio_gui_t * gui)
{
  
}
