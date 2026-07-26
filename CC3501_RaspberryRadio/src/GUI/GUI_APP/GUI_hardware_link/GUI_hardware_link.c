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
  if (hardware_link_initialized)
    return true;

  sd_logging_request_queue = xQueueCreate(1U, sizeof(bool));
  if (sd_logging_request_queue == NULL)
  {
    printf("[SD GUI] Failed to create the SD logging request queue\n");
    return false;
  }

  sd_logging_state_queue = xQueueCreate(1U, sizeof(radio_gui_sd_logging_state_t));
  if (sd_logging_state_queue == NULL)
  {
    printf("[SD GUI] Failed to create the SD logging state queue\n");
    vQueueDelete(sd_logging_request_queue);
    sd_logging_request_queue = NULL;
    return false;
  }

  critical_section_init(&rsq_snapshot_lock);
  memset(&latest_rsq_snapshot, 0, sizeof(latest_rsq_snapshot));
  rsq_snapshot_available = false;
  hardware_link_initialized = true;
  printf("[SD GUI] Cross-core SD logging bridge initialized\n");
  return true;
}

static void radio_gui_band_changed_cb(void *context, radio_gui_band_t band)
{
  (void)context;
  lv_log("[GUI] band: %s\n", band == RADIO_GUI_BAND_FM ? "FM" : "SW/AM/LW");

  if (band == RADIO_GUI_BAND_FM)
  {
    if (SI4732_Power_Up_FM() != SI4732_SUCCESS)
    {
      lv_log("[GUI]: SI4732 Power Up FM failed!\n");
    }
  }
  else if (band == RADIO_GUI_BAND_SW_AM_LW)
  {
    if (SI4732_Power_Up_SW_AM_LW() != SI4732_SUCCESS)
    {
      lv_log("[GUI]: SI4732 Power Up SW/AM/LW failed!\n");
    }
  }
  else
  {
    lv_log("[GUI]: Unknown band value: %d\n", (int)band);
  }
}

static bool radio_gui_frequency_submitted_cb(void *context,
                                             const radio_gui_frequency_request_t *request,
                                             char *result_text,
                                             size_t result_text_size)
{
  (void)context;

  double real_freq = request->value * (request->unit == RADIO_GUI_FREQ_MHZ ? 1e6 : 1e3);

  // check if the frequency is valid for the selected band
  if (request->band == RADIO_GUI_BAND_FM)
  {
    if (real_freq < FM_BAND_BOTTOM_FREQ_10KHZ * 10000U)
    {
      snprintf(result_text, result_text_size, "Invalid FM frequency: %.3f MHz \n Frequency must be above 64 MHz", real_freq / 1e6);
      lv_log("[GUI] Invalid FM frequency: %.3f MHz \n Frequency must be above 64 MHz\n", real_freq / 1e6);
      return false;
    }
    else if (real_freq > FM_BAND_TOP_FREQ_10KHZ * 10000U)
    {
      snprintf(result_text, result_text_size, "Invalid FM frequency: %.3f MHz \n Frequency must be below 108 MHz", real_freq / 1e6);
      lv_log("[GUI] Invalid FM frequency: %.3f MHz \n Frequency must be below 108 MHz\n", real_freq / 1e6);
      return false;
    }
    else
    {
      lv_log("[GUI] FM frequency check passed\n");
    }
  }
  else if (request->band == RADIO_GUI_BAND_SW_AM_LW)
  {
    if (real_freq < SW_AM_LW_BAND_BOTTOM_FREQ_1KHZ * 1000U)
    {
      snprintf(result_text, result_text_size, "Invalid SW/AM/LW frequency: %.3f kHz \n Frequency must be above 149 kHz", real_freq / 1e3);
      lv_log("[GUI] SW/AM/LW frequency check failed: %.3f kHz \n Frequency must be above 149 kHz\n", real_freq / 1e3);
      return false;
    }
    else if (real_freq > SW_AM_LW_BAND_TOP_FREQ_1KHZ * 1000U)
    {
      snprintf(result_text, result_text_size, "Invalid SW/AM/LW frequency: %.3f kHz \n Frequency must be below 23 MHz", real_freq / 1e3);
      lv_log("[GUI] SW/AM/LW frequency check failed: %.3f kHz \n Frequency must be below 23 MHz\n", real_freq / 1e3);
      return false;
    }
    else
    {
      lv_log("[GUI] SW/AM/LW frequency check passed\n");
    }
  }
  else
  {
    {
      snprintf(result_text, result_text_size, "Unknown band value: %d", (int)request->band);
      lv_log("[GUI] Unknown band value: %d", (int)request->band);
      return false;
    }
  }

  // Send the frequency to the SI4732 driver for tuning
  if (request->band == RADIO_GUI_BAND_FM)
  {
    uint16_t freq_10khz = (uint16_t)round(real_freq / 10000.0);
    SI4732_FM_TuneStatus_t FM_tune_status_s;
    if (SI4732_Set_FM_Freq_Blocking_And_Read_Tune_Status(
            freq_10khz,
            SI4732_FREEZE_METRICS_TUNING,
            SI4732_NORMAL_TUNING,
            SI4732_TUNE_STATUS_CANCEL_SEEK,
            SI4732_INT_FLAG_CLEAR,
            &FM_tune_status_s,
            100) != SI4732_SUCCESS)
    {
      snprintf(result_text, result_text_size, "SI4732 FM tune failed for %.3f MHz", real_freq / 1e6);
      lv_log("[GUI] SI4732 FM tune failed for %.3f MHz\n", real_freq / 1e6);
      return false;
    }
    else
    {
      snprintf(result_text, result_text_size,
               "SI4732 FM tune success for %.3f MHz\nBLTF=%d\nAFCRL=%d\nVALID=%d\nFreq=%dHz\nRSSI=%ddBm\nSNR=%ddB\nMultipath=%d",
               real_freq / 1e6,
               FM_tune_status_s.BLTF,
               FM_tune_status_s.AFCRL,
               FM_tune_status_s.VALID,
               FM_tune_status_s.read_freq_10kHz * 10000U, // convert to Hz
               FM_tune_status_s.rssi_dbuv - 107,          // convert dBuV to dBm for a 50 ohm system
               FM_tune_status_s.snr_db,
               FM_tune_status_s.multipath);

      lv_log(result_text, result_text_size,
             "[GUI] SI4732 FM tune success for %.3f MHz\nBLTF=%d\nAFCRL=%d\nVALID=%d\nFreq=%dHz\nRSSI=%ddBm\nSNR=%ddB\nMultipath=%d\n",
             real_freq / 1e6,
             FM_tune_status_s.BLTF,
             FM_tune_status_s.AFCRL,
             FM_tune_status_s.VALID,
             FM_tune_status_s.read_freq_10kHz * 10000U, // convert to Hz
             FM_tune_status_s.rssi_dbuv - 107,          // convert dBuV to dBm for a 50 ohm system
             FM_tune_status_s.snr_db,
             FM_tune_status_s.multipath);
    }
  }
  else if (request->band == RADIO_GUI_BAND_SW_AM_LW)
  {
    uint16_t freq_1khz = (uint16_t)round(real_freq / 1000.0);
    SI4732_SW_AM_LW_TuneStatus_t SW_AM_LW_tune_status_s;
    if (SI4732_Set_SW_AM_LW_Freq_Blocking_And_Read_Tune_Status(
            freq_1khz,
            0, // automatic antenna tuning
            SI4732_NORMAL_TUNING,
            SI4732_TUNE_STATUS_CANCEL_SEEK,
            SI4732_INT_FLAG_CLEAR,
            &SW_AM_LW_tune_status_s,
            100) != SI4732_SUCCESS)
    {
      snprintf(result_text, result_text_size, "SI4732 SW/AM/LW tune failed for %.3f kHz", real_freq / 1e3);

      lv_log("[GUI] SI4732 SW/AM/LW tune failed for %.3f kHz\n", real_freq / 1e3);
      return false;
    }
    else
    {
      snprintf(result_text, result_text_size,
               "SI4732 SW/AM/LW tune success for %.3f kHz\nBLTF=%d\nAFCRL=%d\nVALID=%d\nFreq=%dHz\nRSSI=%ddBm\nSNR=%ddB\nAntCap=%dpF",
               real_freq / 1e3,
               SW_AM_LW_tune_status_s.BLTF,
               SW_AM_LW_tune_status_s.AFCRL,
               SW_AM_LW_tune_status_s.VALID,
               SW_AM_LW_tune_status_s.read_freq_1kHz * 1000U, // convert to Hz
               SW_AM_LW_tune_status_s.rssi_dbuv - 107,        // convert dBuV to dBm for a 50 ohm system
               SW_AM_LW_tune_status_s.snr_db,
               SW_AM_LW_tune_status_s.read_ant_cap_val / 4); // convert to pF
      lv_log(result_text, result_text_size,
             "[GUI] SI4732 SW/AM/LW tune success for %.3f kHz\nBLTF=%d\nAFCRL=%d\nVALID=%d\nFreq=%dHz\nRSSI=%ddBm\nSNR=%ddB\nAntCap=%dpF\n",
             real_freq / 1e3,
             SW_AM_LW_tune_status_s.BLTF,
             SW_AM_LW_tune_status_s.AFCRL,
             SW_AM_LW_tune_status_s.VALID,
             SW_AM_LW_tune_status_s.read_freq_1kHz * 1000U, // convert to Hz
             SW_AM_LW_tune_status_s.rssi_dbuv - 107,        // convert dBuV to dBm for a 50 ohm system
             SW_AM_LW_tune_status_s.snr_db,
             SW_AM_LW_tune_status_s.read_ant_cap_val / 4); // convert to pF
    }
  }

  return true;
}

static void radio_gui_volume_changed_cb(void *context, uint8_t volume)
{
  (void)context;

  // check if volume is within valid range
  if (volume > 63)
  {
    lv_log("[GUI] Invalid volume level: %u\n", (unsigned)volume);
    return;
  }
  else if (volume != 0)
  {
    SI4732_Set_Audio_Mute(false, false);
    SI4732_Set_Audio_Volume(volume);
    lv_log("[GUI] Volume level changed to: %u\n", (unsigned)volume);
    return;
  }
  else
  {
    SI4732_Set_Audio_Mute(true, true);
    lv_log("[GUI] Volume muted\n");
  }
}

static void radio_gui_channel_filter_changed_cb(void *context, radio_gui_band_t band, const char *filter_name)
{
  (void)context;
  if (band == RADIO_GUI_BAND_FM)
  {
    if (strcmp(filter_name, "BW_AUTO") == 0)
    {
      if (SI4732_Set_FM_Channel_Filter(SI4732_FM_BW_AUTO) != SI4732_SUCCESS)
      {
        lv_log("[GUI] Failed to set FM channel filter to AUTO\n");
        return;
      }
      else
      {
        lv_log("[GUI] FM channel filter set to AUTO\n");
      }
    }

    else if (strcmp(filter_name, "BW_110KHZ") == 0)
    {
      if (SI4732_Set_FM_Channel_Filter(SI4732_FM_BW_110KHZ) != SI4732_SUCCESS)
      {
        lv_log("[GUI] Failed to set FM channel filter to 110 kHz\n");
        return;
      }
      else
      {
        lv_log("[GUI] FM channel filter set to 110 kHz\n");
      }
    }

    else if (strcmp(filter_name, "BW_84KHZ") == 0)
    {
      if (SI4732_Set_FM_Channel_Filter(SI4732_FM_BW_84KHZ) != SI4732_SUCCESS)
      {
        lv_log("[GUI] Failed to set FM channel filter to 84 kHz\n");
        return;
      }
      else
      {
        lv_log("[GUI] FM channel filter set to 84 kHz\n");
      }
    }
    else if (strcmp(filter_name, "BW_60KHZ") == 0)
    {
      if (SI4732_Set_FM_Channel_Filter(SI4732_FM_BW_60KHZ) != SI4732_SUCCESS)
      {
        lv_log("[GUI] Failed to set FM channel filter to 60 kHz\n");
        return;
      }
      else
      {
        lv_log("[GUI] FM channel filter set to 60 kHz\n");
      }
    }
    else if (strcmp(filter_name, "BW_40KHZ") == 0)
    {
      if (SI4732_Set_FM_Channel_Filter(SI4732_FM_BW_40KHZ) != SI4732_SUCCESS)
      {
        lv_log("[GUI] Failed to set FM channel filter to 40 kHz\n");
        return;
      }
      else
      {
        lv_log("[GUI] FM channel filter set to 40 kHz\n");
      }
    }
    else
    {
      lv_log("[GUI] Unknown FM channel filter: %s\n", filter_name);
      return;
    }
  }
  else if (band == RADIO_GUI_BAND_SW_AM_LW)
  {
    if (strcmp(filter_name, "BW_6KHZ") == 0)
    {
      if (SI4732_Set_SW_AM_LW_Channel_Filter(SI4732_SW_AM_LW_BW_6KHZ, SI4732_SW_AM_LW_ENABLE_POWER_LINE_NOISE_REJECTION_FILTER) != SI4732_SUCCESS)
      {
        lv_log("[GUI] Failed to set SW/AM/LW channel filter to 6 kHz\n");
        return;
      }
      else
      {
        lv_log("[GUI] SW/AM/LW channel filter set to 6 kHz\n");
      }
    }
    else if (strcmp(filter_name, "BW_4KHZ") == 0)
    {
      if (SI4732_Set_SW_AM_LW_Channel_Filter(SI4732_SW_AM_LW_BW_4KHZ, SI4732_SW_AM_LW_ENABLE_POWER_LINE_NOISE_REJECTION_FILTER) != SI4732_SUCCESS)
      {
        lv_log("[GUI] Failed to set SW/AM/LW channel filter to 4 kHz\n");
        return;
      }
      else
      {
        lv_log("[GUI] SW/AM/LW channel filter set to 4 kHz\n");
      }
    }
    else if (strcmp(filter_name, "BW_3KHZ") == 0)
    {
      if (SI4732_Set_SW_AM_LW_Channel_Filter(SI4732_SW_AM_LW_BW_3KHZ, SI4732_SW_AM_LW_ENABLE_POWER_LINE_NOISE_REJECTION_FILTER) != SI4732_SUCCESS)
      {
        lv_log("[GUI] Failed to set SW/AM/LW channel filter to 3 kHz\n");
        return;
      }
      else
      {
        lv_log("[GUI] SW/AM/LW channel filter set to 3 kHz\n");
      }
    }
    else if (strcmp(filter_name, "BW_2KHZ") == 0)
    {
      if (SI4732_Set_SW_AM_LW_Channel_Filter(SI4732_SW_AM_LW_BW_2KHZ, SI4732_SW_AM_LW_ENABLE_POWER_LINE_NOISE_REJECTION_FILTER) != SI4732_SUCCESS)
      {
        lv_log("[GUI] Failed to set SW/AM/LW channel filter to 2 kHz\n");
        return;
      }
      else
      {
        lv_log("[GUI] SW/AM/LW channel filter set to 2 kHz\n");
      }
    }
    else if (strcmp(filter_name, "BW_1KHZ") == 0)
    {
      if (SI4732_Set_SW_AM_LW_Channel_Filter(SI4732_SW_AM_LW_BW_1KHZ, SI4732_SW_AM_LW_ENABLE_POWER_LINE_NOISE_REJECTION_FILTER) != SI4732_SUCCESS)
      {
        lv_log("[GUI] Failed to set SW/AM/LW channel filter to 1 kHz\n");
        return;
      }
      else
      {
        lv_log("[GUI] SW/AM/LW channel filter set to 1 kHz\n");
      }
    }
    else if (strcmp(filter_name, "BW_1K8HZ") == 0)
    {
      if (SI4732_Set_SW_AM_LW_Channel_Filter(SI4732_SW_AM_LW_BW_1K8HZ, SI4732_SW_AM_LW_ENABLE_POWER_LINE_NOISE_REJECTION_FILTER) != SI4732_SUCCESS)
      {
        lv_log("[GUI] Failed to set SW/AM/LW channel filter to 1.8 kHz\n");
        return;
      }
      else
      {
        lv_log("[GUI] SW/AM/LW channel filter set to 1.8 kHz\n");
      }
    }
    else if (strcmp(filter_name, "BW_2K5HZ") == 0)
    {
      if (SI4732_Set_SW_AM_LW_Channel_Filter(SI4732_SW_AM_LW_BW_2K5HZ, SI4732_SW_AM_LW_ENABLE_POWER_LINE_NOISE_REJECTION_FILTER) != SI4732_SUCCESS)
      {
        lv_log("[GUI] Failed to set SW/AM/LW channel filter to 2.5 kHz\n");
        return;
      }
      else
      {
        lv_log("[GUI] SW/AM/LW channel filter set to 2.5 kHz\n");
      }
    }
    else
    {
      lv_log("[GUI] Unknown SW/AM/LW channel filter: %s\n", filter_name);
      return;
    }
  }
  else
  {
    lv_log("[GUI] Unknown band for channel filter change: %d\n", (int)band);
    return;
  }
}

static void radio_gui_audio_amp_enabled_changed_cb(void *context, radio_gui_amp_mode_t mode, bool enabled)
{
  (void)context;

  if (enabled)
  {
    if (mode == RADIO_GUI_AMP_CLASS_D)
    {
      NS4160_powerup_class_D();
      lv_log("[GUI] Class-D amplifier powered up\n");
    }
    else if (mode == RADIO_GUI_AMP_CLASS_AB)
    {
      NS4160_powerup_class_AB();
      lv_log("[GUI] Class-AB amplifier powered up\n");
    }
    else
    {
      lv_log("[GUI] Unknown amplifier mode, cannot power up amplifier\n");
    }
  }
  else
  {
    NS4160_powerdown();
    lv_log("[GUI] Amplifier powered down\n");
  }
}

static void radio_gui_audio_amp_mode_changed_cb(void *context, radio_gui_amp_mode_t mode, bool enabled)
{
  (void)context;
  if (enabled)
  {
    if (mode == RADIO_GUI_AMP_CLASS_D)
    {
      NS4160_powerup_class_D();
      lv_log("[GUI] switched to Class-D amplifier\n");
    }
    else if (mode == RADIO_GUI_AMP_CLASS_AB)
    {
      NS4160_powerup_class_AB();
      lv_log("[GUI] switched to Class-AB amplifier\n");
    }
    else
    {
      lv_log("[GUI] Unknown amplifier mode\n");
    }
  }
  else
  {
    NS4160_powerdown();
  }
}

static void radio_gui_backlight_changed_cb(void *context, uint8_t level)
{
  (void)context;
  // check if level is within valid range
  if (level > SCREEN_BACKLIGHT_PWM_WARP)
  {
    lv_log("[GUI] Invalid backlight level: %u\n", (unsigned)level);
    return;
  }
  else
  {
    RP2040_screen_pwm_set_level(level);
    lv_log("[GUI] Backlight level changed to: %u\n", (unsigned)level);
  }
}

static void radio_gui_gps_enabled_changed_cb(void *context, bool enabled)
{
  (void)context;
  if (enabled)
  {
    GPS_module_powerup();
    lv_log("[GUI] GPS module powered up\n");
  }
  else
  {
    GPS_module_powerdown();
    lv_log("[GUI] GPS module powered down\n");
  }
}

static void radio_gui_sd_logging_changed_cb(void *context, bool enabled)
{
  (void)context;
  if (sd_logging_request_queue == NULL)
  {
    printf("[SD GUI] Cannot submit %s request: bridge is not initialized\n",
           enabled ? "start" : "stop");
    return;
  }
  if (xQueueOverwrite(sd_logging_request_queue, &enabled) != pdPASS)
  {
    printf("[SD GUI] Failed to submit SD logging %s request\n",
           enabled ? "start" : "stop");
    return;
  }
  printf("[SD GUI] SD logging %s requested from RADIO_GUI_PAGE_GPS\n",
         enabled ? "start" : "stop");
}

bool radio_gui_take_sd_logging_request(bool *enabled)
{
  if (enabled == NULL || sd_logging_request_queue == NULL)
    return false;
  return xQueueReceive(sd_logging_request_queue, enabled, 0U) == pdPASS;
}

void radio_gui_report_sd_logging_state(radio_gui_sd_status_t status,
                                       bool logging_active)
{
  radio_gui_sd_logging_state_t state = {
      .status = status,
      .logging_active = logging_active,
  };

  if (sd_logging_state_queue == NULL)
  {
    printf("[SD GUI] Cannot publish SD state: bridge is not initialized\n");
    return;
  }
  if (xQueueOverwrite(sd_logging_state_queue, &state) != pdPASS)
  {
    printf("[SD GUI] Failed to publish SD state\n");
  }
}

void radio_gui_apply_sd_logging_state(radio_gui_t *gui)
{
  radio_gui_sd_logging_state_t state;

  if (gui == NULL || sd_logging_state_queue == NULL)
    return;
  if (xQueueReceive(sd_logging_state_queue, &state, 0U) != pdPASS)
    return;
  radio_gui_set_sd_status(gui, state.status);
  radio_gui_set_sd_logging_enabled(gui, state.logging_active);
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
  if (snapshot == NULL || !hardware_link_initialized)
    return;
  critical_section_enter_blocking(&rsq_snapshot_lock);
  latest_rsq_snapshot = *snapshot;
  rsq_snapshot_available = true;
  critical_section_exit(&rsq_snapshot_lock);
}

bool radio_gui_get_latest_si4732_rsq_info(sd_card_log_rsq_info_t *rsq_info)
{
  bool available;

  if (rsq_info == NULL || !hardware_link_initialized)
    return false;
  critical_section_enter_blocking(&rsq_snapshot_lock);
  available = rsq_snapshot_available;
  if (available)
    *rsq_info = latest_rsq_snapshot;
  critical_section_exit(&rsq_snapshot_lock);
  return available;
}

static void radio_gui_clear_rsq_widgets(radio_gui_t *gui)
{
  if (gui == NULL)
    return;
  for (uint32_t field = 0U; field < RADIO_GUI_RX_FIELD_COUNT; field++)
  {
    radio_gui_set_rx_field(gui, (radio_gui_rx_field_t)field, "--");
  }
}

void radio_gui_sample_si4732_rsq_info(radio_gui_t *gui, bool update_widgets)
{
  sd_card_log_rsq_info_t snapshot;
  SI4732_FM_TuneStatus_t FM_tune_status_s;
  SI4732_FM_RSQ_Status_t FM_rsq_status_s;
  SI4732_SW_AM_LW_TuneStatus_t SW_AM_LW_tune_status_s;
  SI4732_SW_AM_LW_RSQ_Status_t SW_AM_LW_rsq_status_s;
  char text_buf[16];

  if (gui == NULL)
    return;
  memset(&snapshot, 0, sizeof(snapshot));

  if (radio_gui_get_band(gui) == RADIO_GUI_BAND_FM)
  {
    snapshot.band = SD_CARD_LOG_RADIO_FM;
    if (SI4732_Get_FM_Tune_Status(SI4732_TUNE_STATUS_CONTINUE_SEEK,
                                  SI4732_INT_FLAG_KEEP,
                                  &FM_tune_status_s) == SI4732_SUCCESS &&
        FM_tune_status_s.read_freq_10kHz >= FM_BAND_BOTTOM_FREQ_10KHZ &&
        FM_tune_status_s.read_freq_10kHz <= FM_BAND_TOP_FREQ_10KHZ)
    {
      snapshot.receive_frequency_hz =
          (uint32_t)FM_tune_status_s.read_freq_10kHz * 10000U;
      snapshot.receive_frequency_valid = true;
    }
    else
    {
      printf("[SD GUI] SI4732 FM receive frequency is unavailable\n");
    }

    snapshot.valid = SI4732_Get_FM_RSQ_Status(SI4732_INT_FLAG_CLEAR,
                                              &FM_rsq_status_s) == SI4732_SUCCESS;
    if (!snapshot.valid)
    {
      printf("[SD GUI] SI4732_Get_FM_RSQ_Status failed\n");
      radio_gui_store_rsq_snapshot(&snapshot);
      if (update_widgets)
        radio_gui_clear_rsq_widgets(gui);
      return;
    }

    snapshot.afcrl = FM_rsq_status_s.AFCRL;
    snapshot.station_valid = FM_rsq_status_s.VALID;
    snapshot.pilot = FM_rsq_status_s.PILOT;
    snapshot.stereo_blend = FM_rsq_status_s.STBLEND;
    snapshot.rssi_dbm = (int16_t)FM_rsq_status_s.rssi_dbuv - 107;
    snapshot.snr_db = FM_rsq_status_s.snr_db;
    snapshot.multipath = FM_rsq_status_s.multipath;
    snapshot.frequency_offset = (int8_t)FM_rsq_status_s.freq_offset;
    radio_gui_store_rsq_snapshot(&snapshot);
    if (!update_widgets)
      return;

    radio_gui_set_rx_field(gui, RADIO_GUI_RX_AFCRL, FM_rsq_status_s.AFCRL == true ? "TRUE" : "FALSE");
    radio_gui_set_rx_field(gui, RADIO_GUI_RX_VALID, FM_rsq_status_s.VALID == true ? "TRUE" : "FALSE");
    radio_gui_set_rx_field(gui, RADIO_GUI_RX_PILOT, FM_rsq_status_s.PILOT == true ? "TRUE" : "FALSE");
    snprintf(text_buf, sizeof(text_buf), "%d", FM_rsq_status_s.STBLEND);
    radio_gui_set_rx_field(gui, RADIO_GUI_RX_STBLEND, text_buf);
    snprintf(text_buf, sizeof(text_buf), "%ddBm", FM_rsq_status_s.rssi_dbuv - 107); // convert dBuV to dBm for a 50 ohm system
    radio_gui_set_rx_field(gui, RADIO_GUI_RX_RSSI, text_buf);                       // convert dBuV to dBm for a 50 ohm system
    snprintf(text_buf, sizeof(text_buf), "%ddB", FM_rsq_status_s.snr_db);
    radio_gui_set_rx_field(gui, RADIO_GUI_RX_SNR, text_buf);
    snprintf(text_buf, sizeof(text_buf), "%d", FM_rsq_status_s.multipath);
    radio_gui_set_rx_field(gui, RADIO_GUI_RX_MULTIPATH, text_buf);
    snprintf(text_buf, sizeof(text_buf), "%d", (int8_t)FM_rsq_status_s.freq_offset);
    radio_gui_set_rx_field(gui, RADIO_GUI_RX_FREQ_OFFSET, text_buf);

    lv_log("[GUI] FM RSQ updated: AFCRL=%s, VALID=%s, PILOT=%s, STBLEND=%d, RSSI=%ddBm, SNR=%ddB, Multipath=%d, FreqOffset=%d\n",
           FM_rsq_status_s.AFCRL == true ? "TRUE" : "FALSE",
           FM_rsq_status_s.VALID == true ? "TRUE" : "FALSE",
           FM_rsq_status_s.PILOT == true ? "TRUE" : "FALSE",
           FM_rsq_status_s.STBLEND,
           FM_rsq_status_s.rssi_dbuv - 107,
           FM_rsq_status_s.snr_db,
           FM_rsq_status_s.multipath,
           (int8_t)FM_rsq_status_s.freq_offset);
  }
  else if (radio_gui_get_band(gui) == RADIO_GUI_BAND_SW_AM_LW)
  {
    snapshot.band = SD_CARD_LOG_RADIO_SW_AM_LW;
    if (SI4732_Get_SW_AM_LW_Tune_Status(SI4732_TUNE_STATUS_CONTINUE_SEEK,
                                        SI4732_INT_FLAG_KEEP,
                                        &SW_AM_LW_tune_status_s) == SI4732_SUCCESS &&
        SW_AM_LW_tune_status_s.read_freq_1kHz >= SW_AM_LW_BAND_BOTTOM_FREQ_1KHZ &&
        SW_AM_LW_tune_status_s.read_freq_1kHz <= SW_AM_LW_BAND_TOP_FREQ_1KHZ)
    {
      snapshot.receive_frequency_hz =
          (uint32_t)SW_AM_LW_tune_status_s.read_freq_1kHz * 1000U;
      snapshot.receive_frequency_valid = true;
    }
    else
    {
      printf("[SD GUI] SI4732 SW/AM/LW receive frequency is unavailable\n");
    }

    snapshot.valid = SI4732_Get_SW_AM_LW_RSQ_Status(SI4732_INT_FLAG_CLEAR,
                                                    &SW_AM_LW_rsq_status_s) == SI4732_SUCCESS;
    if (!snapshot.valid)
    {
      printf("[SD GUI] SI4732_Get_SW_AM_LW_RSQ_Status failed\n");
      radio_gui_store_rsq_snapshot(&snapshot);
      if (update_widgets)
        radio_gui_clear_rsq_widgets(gui);
      return;
    }

    snapshot.afcrl = SW_AM_LW_rsq_status_s.AFCRL;
    snapshot.station_valid = SW_AM_LW_rsq_status_s.VALID;
    snapshot.rssi_dbm = (int16_t)SW_AM_LW_rsq_status_s.rssi_dbuv - 107;
    snapshot.snr_db = SW_AM_LW_rsq_status_s.snr_db;
    radio_gui_store_rsq_snapshot(&snapshot);
    if (!update_widgets)
      return;

    radio_gui_set_rx_field(gui, RADIO_GUI_RX_AFCRL, SW_AM_LW_rsq_status_s.AFCRL == true ? "TRUE" : "FALSE");
    radio_gui_set_rx_field(gui, RADIO_GUI_RX_VALID, SW_AM_LW_rsq_status_s.VALID == true ? "TRUE" : "FALSE");
    radio_gui_set_rx_field(gui, RADIO_GUI_RX_PILOT, "N/A");                               // SW/AM/LW does not have a pilot signal
    radio_gui_set_rx_field(gui, RADIO_GUI_RX_STBLEND, "N/A");                             // SW/AM/LW does not have stereo blend
    snprintf(text_buf, sizeof(text_buf), "%ddBm", SW_AM_LW_rsq_status_s.rssi_dbuv - 107); // convert dBuV to dBm for a 50 ohm system
    radio_gui_set_rx_field(gui, RADIO_GUI_RX_RSSI, text_buf);
    snprintf(text_buf, sizeof(text_buf), "%ddB", SW_AM_LW_rsq_status_s.snr_db);
    radio_gui_set_rx_field(gui, RADIO_GUI_RX_SNR, text_buf);
    radio_gui_set_rx_field(gui, RADIO_GUI_RX_MULTIPATH, "N/A"); // SW/AM/LW does not have multipath
    radio_gui_set_rx_field(gui, RADIO_GUI_RX_FREQ_OFFSET, "N/A");
    // SW/AM/LW does not have frequency offset
    lv_log("[GUI] SW/AM/LW RSQ updated: AFCRL=%s, VALID=%s, RSSI=%ddBm, SNR=%ddB\n",
           SW_AM_LW_rsq_status_s.AFCRL == true ? "TRUE" : "FALSE",
           SW_AM_LW_rsq_status_s.VALID == true ? "TRUE" : "FALSE",
           SW_AM_LW_rsq_status_s.rssi_dbuv - 107,
           SW_AM_LW_rsq_status_s.snr_db);
  }
  else
  {
    lv_log("[GUI] Unknown band value: %d\n", (int)radio_gui_get_band(gui));
  }
}

void radio_gui_update_si4732_rsq_info(radio_gui_t *gui)
{
  radio_gui_sample_si4732_rsq_info(gui, true);
}

void radio_gui_update_battery_voltage(radio_gui_t *gui)
{
  float battery_voltage = RP2040_adc_read_battery_voltage();
  radio_gui_set_battery_voltage(gui, battery_voltage);
  lv_log("[GUI] Battery voltage updated: %.2f V\n", battery_voltage);
}

void radio_gui_update_waveform(radio_gui_t *gui, const int8_t *time_domain_waveform_data, const int8_t *spec_waveform_data) // RADIO_GUI_WAVEFORM_POINT_COUNT
{
  radio_gui_set_time_domain_data(gui, time_domain_waveform_data, ADC_DMA_CAPTURE_SAMPLE_RATE_HZ);
  radio_gui_set_spectrum_data(gui,
                              spec_waveform_data,
                              ADC_DMA_CAPTURE_SAMPLE_RATE_HZ / 2,
                              RADIO_GUI_SPECTRUM_MIN_DB,
                              RADIO_GUI_SPECTRUM_MAX_DB);
}

static void gps_systems_to_text(uint32_t system_mask, char *text, size_t text_size)
{
  static const struct
  {
    uint32_t mask;
    const char *name;
  } systems[] = {
      {GPS_SYSTEM_GPS, "GPS"},
      {GPS_SYSTEM_BEIDOU, "BDS"},
      {GPS_SYSTEM_GLONASS, "GLO"},
      {GPS_SYSTEM_GALILEO, "GAL"},
      {GPS_SYSTEM_QZSS, "QZSS"},
      {GPS_SYSTEM_NAVIC, "NavIC"}};
  size_t used = 0U;

  if (text == NULL || text_size == 0U)
    return;
  text[0] = '\0';
  for (size_t i = 0U; i < sizeof(systems) / sizeof(systems[0]); i++)
  {
    if ((system_mask & systems[i].mask) == 0U)
      continue;
    int written = snprintf(text + used,
                           text_size - used,
                           "%s%s",
                           used == 0U ? "" : "+",
                           systems[i].name);
    if (written < 0 || (size_t)written >= text_size - used)
    {
      text[text_size - 1U] = '\0';
      return;
    }
    used += (size_t)written;
  }
  if (used == 0U)
    (void)snprintf(text, text_size, "--");
}

static const char *gps_fix_type_text(uint8_t fix_type)
{
  switch (fix_type)
  {
  case 3U:
    return "3D fix";
  case 2U:
    return "2D fix";
  default:
    return "No fix";
  }
}

static const char *gps_location_mode_text(char mode)
{
  switch (mode)
  {
  case 'A':
    return "Autonomous";
  case 'D':
    return "Differential";
  case 'E':
    return "Estimated";
  case 'M':
    return "Manual";
  case 'S':
    return "Simulator";
  case 'N':
    return "Invalid";
  default:
    return "--";
  }
}

void radio_gui_update_gps_info(radio_gui_t *gui, const gps_info_t *gps_info)
{
  char text_buf[64];
  double coordinate;

  if (gui == NULL || gps_info == NULL)
    return;

  if (gps_info->antenna_status == GPS_ANTENNA_OPEN ||
      gps_info->antenna_status == GPS_ANTENNA_SHORT ||
      (gps_info->valid_sentence_count == 0U && gps_info->checksum_error_count > 0U))
  {
    radio_gui_set_gps_status(gui, RADIO_GUI_GPS_ERROR);
  }
  else if (gps_info->position_valid)
  {
    radio_gui_set_gps_status(gui, RADIO_GUI_GPS_LOCKED);
  }
  else
  {
    radio_gui_set_gps_status(gui, RADIO_GUI_GPS_UNLOCKED);
  }

  if (gps_info->position.valid)
  {
    coordinate = gps_info->position.latitude_deg < 0.0 ? -gps_info->position.latitude_deg : gps_info->position.latitude_deg;
    (void)snprintf(text_buf,
                   sizeof(text_buf),
                   "%.6f %c",
                   coordinate,
                   gps_info->position.latitude_hemisphere);
    radio_gui_set_gps_field(gui, RADIO_GUI_GPS_LATITUDE, text_buf);

    coordinate = gps_info->position.longitude_deg < 0.0 ? -gps_info->position.longitude_deg : gps_info->position.longitude_deg;
    (void)snprintf(text_buf,
                   sizeof(text_buf),
                   "%.6f %c",
                   coordinate,
                   gps_info->position.longitude_hemisphere);
    radio_gui_set_gps_field(gui, RADIO_GUI_GPS_LONGITUDE, text_buf);
  }
  else
  {
    radio_gui_set_gps_field(gui, RADIO_GUI_GPS_LATITUDE, "--");
    radio_gui_set_gps_field(gui, RADIO_GUI_GPS_LONGITUDE, "--");
  }

  if (gps_info->has_altitude_msl)
  {
    (void)snprintf(text_buf, sizeof(text_buf), "%.1f m", gps_info->altitude_msl_m);
    radio_gui_set_gps_field(gui, RADIO_GUI_GPS_ALTITUDE_SEA, text_buf);
  }
  else
  {
    radio_gui_set_gps_field(gui, RADIO_GUI_GPS_ALTITUDE_SEA, "--");
  }
  if (gps_info->has_altitude_ellipsoid)
  {
    (void)snprintf(text_buf, sizeof(text_buf), "%.1f m", gps_info->altitude_ellipsoid_m);
    radio_gui_set_gps_field(gui, RADIO_GUI_GPS_ALTITUDE, text_buf);
  }
  else
  {
    radio_gui_set_gps_field(gui, RADIO_GUI_GPS_ALTITUDE, "--");
  }
  if (gps_info->has_speed_kmh)
  {
    (void)snprintf(text_buf, sizeof(text_buf), "%.2f km/h", gps_info->speed_kmh);
    radio_gui_set_gps_field(gui, RADIO_GUI_GPS_SPEED, text_buf);
  }
  else
  {
    radio_gui_set_gps_field(gui, RADIO_GUI_GPS_SPEED, "--");
  }
  if (gps_info->has_course_true)
  {
    (void)snprintf(text_buf, sizeof(text_buf), "%.2f deg T", gps_info->course_true_deg);
    radio_gui_set_gps_field(gui, RADIO_GUI_GPS_DIRECTION, text_buf);
  }
  else
  {
    radio_gui_set_gps_field(gui, RADIO_GUI_GPS_DIRECTION, "--");
  }
  if (gps_info->utc_time.valid)
  {
    (void)snprintf(text_buf,
                   sizeof(text_buf),
                   "%02u:%02u:%02u.%03u",
                   (unsigned)gps_info->utc_time.hour,
                   (unsigned)gps_info->utc_time.minute,
                   (unsigned)gps_info->utc_time.second,
                   (unsigned)gps_info->utc_time.millisecond);
    radio_gui_set_gps_field(gui, RADIO_GUI_GPS_UTC_TIME, text_buf);
  }
  else
  {
    radio_gui_set_gps_field(gui, RADIO_GUI_GPS_UTC_TIME, "--");
  }
  if (gps_info->utc_date.valid)
  {
    (void)snprintf(text_buf,
                   sizeof(text_buf),
                   "%04u-%02u-%02u",
                   (unsigned)gps_info->utc_date.year,
                   (unsigned)gps_info->utc_date.month,
                   (unsigned)gps_info->utc_date.day);
    radio_gui_set_gps_field(gui, RADIO_GUI_GPS_UTC_DATE, text_buf);
  }
  else
  {
    radio_gui_set_gps_field(gui, RADIO_GUI_GPS_UTC_DATE, "--");
  }
  if (gps_info->has_magnetic_variation)
  {
    coordinate = gps_info->magnetic_variation_deg < 0.0f ? -gps_info->magnetic_variation_deg : gps_info->magnetic_variation_deg;
    (void)snprintf(text_buf,
                   sizeof(text_buf),
                   "%.2f %c",
                   coordinate,
                   gps_info->magnetic_variation_direction);
    radio_gui_set_gps_field(gui, RADIO_GUI_GPS_MAGNETIC_DECLINATION, text_buf);
  }
  else
  {
    radio_gui_set_gps_field(gui, RADIO_GUI_GPS_MAGNETIC_DECLINATION, "--");
  }

  radio_gui_set_gps_field(gui, RADIO_GUI_GPS_LOCATE_MODE, gps_fix_type_text(gps_info->fix_type));
  gps_systems_to_text(gps_info->constellation_mask, text_buf, sizeof(text_buf));
  radio_gui_set_gps_field(gui, RADIO_GUI_GPS_LOCATION_SYSTEM, text_buf);
  radio_gui_set_gps_field(gui,
                          RADIO_GUI_GPS_LOCATION_TYPE,
                          gps_location_mode_text(gps_info->location_mode));
  if (gps_info->location_status == 'A')
  {
    radio_gui_set_gps_field(gui,
                            RADIO_GUI_GPS_LOCATION_STATUS,
                            gps_info->navigation_status == 'V' ? "Valid / nav invalid" : "Valid");
  }
  else
  {
    radio_gui_set_gps_field(gui, RADIO_GUI_GPS_LOCATION_STATUS, "Invalid");
  }
  (void)snprintf(text_buf,
                 sizeof(text_buf),
                 "%u used / %u view",
                 (unsigned)gps_info->satellites_used,
                 (unsigned)gps_info->satellites_in_view);
  radio_gui_set_gps_field(gui, RADIO_GUI_GPS_SATELLITES, text_buf);
}

void radio_gui_startup_hardware(radio_gui_t *gui)
{
  // for SI4732
  radio_gui_set_band(gui, RADIO_GUI_BAND_FM);
  radio_gui_set_frequency_text(gui, "101.5 MHz");
  radio_gui_set_volume(gui, 63);
  SI4732_Power_Up_FM();
  SI4732_Set_FM_Freq_Blocking_And_Read_Tune_Status(10150,
                                                   SI4732_FREEZE_METRICS_TUNING,
                                                   SI4732_NORMAL_TUNING,
                                                   SI4732_TUNE_STATUS_CANCEL_SEEK,
                                                   SI4732_INT_FLAG_CLEAR,
                                                   NULL,
                                                   100);
  SI4732_Set_FM_Channel_Filter(SI4732_FM_BW_AUTO);
  SI4732_Set_Audio_Mute(false, false);
  SI4732_Set_Audio_Volume(63);

  // for NS4160
  radio_gui_set_audio_amp_mode(gui, RADIO_GUI_AMP_CLASS_AB);
  radio_gui_set_audio_amp_enabled(gui, true);
  NS4160_powerup_class_AB();

  // for backlight
  radio_gui_set_backlight(gui, 49);
  RP2040_screen_pwm_set_level(49);

  // for battery voltage
  radio_gui_set_battery_voltage(gui, RP2040_adc_read_battery_voltage());

  // for GPS
  radio_gui_set_gps_enabled(gui, false);
  radio_gui_set_sd_logging_enabled(gui, false);
  radio_gui_set_sd_status(gui, RADIO_GUI_SD_OK);
  radio_gui_set_gps_status(gui, RADIO_GUI_GPS_UNLOCKED);
  for (uint32_t field = 0U; field < RADIO_GUI_GPS_FIELD_COUNT; field++)
  {
    radio_gui_set_gps_field(gui, (radio_gui_gps_field_t)field, "--");
  }
  GPS_module_powerdown();
}
