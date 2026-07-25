/**
 * @file radio_gui.h
 * @brief Portable LVGL radio dashboard interface.
 *
 * This module owns only LVGL objects and UI state. Hardware access is exposed
 * through callbacks so the same GUI can run with SDL or on the RP2040 target.
 */

#ifndef RADIO_GUI_H
#define RADIO_GUI_H

#include "lvgl.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct radio_gui radio_gui_t;

#define RADIO_GUI_WAVEFORM_POINT_COUNT 256U

typedef enum {
    RADIO_GUI_PAGE_HARDWARE = 0,
    RADIO_GUI_PAGE_RADIO,
    RADIO_GUI_PAGE_WAVEFORM,
    RADIO_GUI_PAGE_GPS
} radio_gui_page_t;

typedef enum {
    RADIO_GUI_BAND_SW_AM_LW = 0,
    RADIO_GUI_BAND_FM
} radio_gui_band_t;

typedef enum {
    RADIO_GUI_FREQ_KHZ = 0,
    RADIO_GUI_FREQ_MHZ
} radio_gui_frequency_unit_t;

typedef enum {
    RADIO_GUI_AMP_CLASS_AB = 0,
    RADIO_GUI_AMP_CLASS_D
} radio_gui_amp_mode_t;

typedef enum {
    RADIO_GUI_SD_NO_CARD = 0,
    RADIO_GUI_SD_ERROR,
    RADIO_GUI_SD_OK
} radio_gui_sd_status_t;

typedef enum {
    RADIO_GUI_GPS_LOCKED = 0,
    RADIO_GUI_GPS_UNLOCKED,
    RADIO_GUI_GPS_ERROR
} radio_gui_gps_status_t;

typedef enum {
    RADIO_GUI_RX_AFCRL = 0,
    RADIO_GUI_RX_VALID,
    RADIO_GUI_RX_PILOT,
    RADIO_GUI_RX_STBLEND,
    RADIO_GUI_RX_RSSI,
    RADIO_GUI_RX_SNR,
    RADIO_GUI_RX_MULTIPATH,
    RADIO_GUI_RX_FREQ_OFFSET,
    RADIO_GUI_RX_FIELD_COUNT
} radio_gui_rx_field_t;

typedef enum {
    RADIO_GUI_GPS_LATITUDE = 0,
    RADIO_GUI_GPS_LONGITUDE,
    RADIO_GUI_GPS_ALTITUDE_SEA,
    RADIO_GUI_GPS_ALTITUDE,
    RADIO_GUI_GPS_SPEED,
    RADIO_GUI_GPS_DIRECTION,
    RADIO_GUI_GPS_UTC_TIME,
    RADIO_GUI_GPS_UTC_DATE,
    RADIO_GUI_GPS_MAGNETIC_DECLINATION,
    RADIO_GUI_GPS_LOCATE_MODE,
    RADIO_GUI_GPS_LOCATION_SYSTEM,
    RADIO_GUI_GPS_LOCATION_TYPE,
    RADIO_GUI_GPS_LOCATION_STATUS,
    RADIO_GUI_GPS_SATELLITES,
    RADIO_GUI_GPS_FIELD_COUNT
} radio_gui_gps_field_t;

typedef struct {
    double value;
    radio_gui_frequency_unit_t unit;
    radio_gui_band_t band;
} radio_gui_frequency_request_t;

typedef struct {
    void (*band_changed)(void * context, radio_gui_band_t band);
    bool (*frequency_submitted)(void * context,
                                const radio_gui_frequency_request_t * request,
                                char * result_text,
                                size_t result_text_size);
    void (*volume_changed)(void * context, uint8_t volume);
    void (*channel_filter_changed)(void * context, radio_gui_band_t band, const char * filter_name);
    void (*audio_amp_enabled_changed)(void * context, bool enabled);
    void (*audio_amp_mode_changed)(void * context, radio_gui_amp_mode_t mode);
    void (*backlight_changed)(void * context, uint8_t level);
    void (*gps_enabled_changed)(void * context, bool enabled);
    void (*sd_logging_changed)(void * context, bool enabled);
} radio_gui_callbacks_t;

typedef struct {
    radio_gui_callbacks_t callbacks;
    void * callback_context;
} radio_gui_config_t;

/** Create the complete four-page GUI below parent. */
radio_gui_t * radio_gui_create(lv_obj_t * parent, const radio_gui_config_t * config);

/** Delete the GUI and release its small context allocation. */
void radio_gui_destroy(radio_gui_t * gui);

/** Select the central Radio dashboard page. */
void radio_gui_show_radio_page(radio_gui_t * gui, lv_anim_enable_t animated);

/** Select any page from software without invoking a swipe callback. */
void radio_gui_show_page(radio_gui_t * gui, radio_gui_page_t page, lv_anim_enable_t animated);

/** Open the full-screen frequency keypad. */
void radio_gui_show_frequency_keypad(radio_gui_t * gui);

/** Show an accepted/rejected SI4732 tuning result and close the keypad. */
void radio_gui_show_tune_result(radio_gui_t * gui, bool accepted, const char * result_text);

/* Software-to-GUI state update API. These functions do not invoke callbacks. */
void radio_gui_set_band(radio_gui_t * gui, radio_gui_band_t band);
void radio_gui_set_frequency_text(radio_gui_t * gui, const char * text);
void radio_gui_set_volume(radio_gui_t * gui, uint8_t volume);
void radio_gui_set_rx_field(radio_gui_t * gui, radio_gui_rx_field_t field, const char * value);

/**
 * Copy one complete signed 8-bit time-domain frame into the plot.
 * sample_rate_hz controls the time-axis labels; pass 0 to show sample indices.
 */
void radio_gui_set_time_domain_data(radio_gui_t * gui,
                                    const int8_t samples[RADIO_GUI_WAVEFORM_POINT_COUNT],
                                    uint32_t sample_rate_hz);

/**
 * Copy one complete signed 8-bit integer-dB FFT log-magnitude frame into the plot.
 * max_frequency_hz is the frequency represented by the final point; pass 0
 * to show FFT-bin indices. min_db/max_db define the visible amplitude range.
 */
void radio_gui_set_spectrum_data(radio_gui_t * gui,
                                 const int8_t log_magnitude_db[RADIO_GUI_WAVEFORM_POINT_COUNT],
                                 uint32_t max_frequency_hz,
                                 int8_t min_db,
                                 int8_t max_db);

void radio_gui_set_battery_voltage(radio_gui_t * gui, float voltage);
void radio_gui_set_audio_amp_enabled(radio_gui_t * gui, bool enabled);
void radio_gui_set_audio_amp_mode(radio_gui_t * gui, radio_gui_amp_mode_t mode);
void radio_gui_set_backlight(radio_gui_t * gui, uint8_t level);

void radio_gui_set_gps_enabled(radio_gui_t * gui, bool enabled);
void radio_gui_set_sd_logging_enabled(radio_gui_t * gui, bool enabled);
void radio_gui_set_sd_status(radio_gui_t * gui, radio_gui_sd_status_t status);
void radio_gui_set_gps_status(radio_gui_t * gui, radio_gui_gps_status_t status);
void radio_gui_set_gps_field(radio_gui_t * gui, radio_gui_gps_field_t field, const char * value);

/*
 * GUI state query API. These functions do not invoke callbacks.
 * Call them from the same task/thread that owns LVGL.
 * Returned string pointers remain owned by LVGL and are valid until that
 * widget's text/value is changed or the GUI is destroyed.
 */
radio_gui_page_t radio_gui_get_current_page(const radio_gui_t * gui);
radio_gui_band_t radio_gui_get_band(const radio_gui_t * gui);
radio_gui_frequency_unit_t radio_gui_get_frequency_unit(const radio_gui_t * gui);
const char * radio_gui_get_frequency_text(const radio_gui_t * gui);
uint8_t radio_gui_get_volume(const radio_gui_t * gui);
bool radio_gui_get_channel_filter(const radio_gui_t * gui,
                                  char * filter_name,
                                  size_t filter_name_size);
const char * radio_gui_get_rx_field(const radio_gui_t * gui, radio_gui_rx_field_t field);

float radio_gui_get_battery_voltage(const radio_gui_t * gui);
bool radio_gui_get_audio_amp_enabled(const radio_gui_t * gui);
radio_gui_amp_mode_t radio_gui_get_audio_amp_mode(const radio_gui_t * gui);
uint8_t radio_gui_get_backlight(const radio_gui_t * gui);

bool radio_gui_get_gps_enabled(const radio_gui_t * gui);
bool radio_gui_get_sd_logging_enabled(const radio_gui_t * gui);
radio_gui_sd_status_t radio_gui_get_sd_status(const radio_gui_t * gui);
radio_gui_gps_status_t radio_gui_get_gps_status(const radio_gui_t * gui);
const char * radio_gui_get_gps_field(const radio_gui_t * gui, radio_gui_gps_field_t field);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* RADIO_GUI_H */
