#include "radio_gui_demo_backend.h"

#include <stdio.h>

static const int8_t demo_sine_lut[32] = {
    0, 25, 49, 71, 90, 106, 117, 125,
    127, 125, 117, 106, 90, 71, 49, 25,
    0, -25, -49, -71, -90, -106, -117, -125,
    -127, -125, -117, -106, -90, -71, -49, -25
};

static int32_t absolute_difference(uint32_t left, uint32_t right)
{
    return left > right ? (int32_t)(left - right) : (int32_t)(right - left);
}

static void populate_signal_waveforms(radio_gui_t * gui)
{
    int8_t time_samples[RADIO_GUI_WAVEFORM_POINT_COUNT];
    int8_t spectrum_db[RADIO_GUI_WAVEFORM_POINT_COUNT];

    for(uint32_t i = 0; i < RADIO_GUI_WAVEFORM_POINT_COUNT; ++i) {
        int32_t fundamental = demo_sine_lut[i & 31U];
        int32_t harmonic = demo_sine_lut[(i * 3U) & 31U];
        time_samples[i] = (int8_t)((fundamental * 3 + harmonic) / 4);

        int32_t magnitude = -108 + (int32_t)((i * 37U) % 9U);
        int32_t peak = -18 - absolute_difference(i, 32U) * 4;
        if(peak > magnitude) magnitude = peak;
        peak = -34 - absolute_difference(i, 84U) * 3;
        if(peak > magnitude) magnitude = peak;
        peak = -46 - absolute_difference(i, 158U) * 2;
        if(peak > magnitude) magnitude = peak;
        spectrum_db[i] = (int8_t)magnitude;
    }

    radio_gui_set_time_domain_data(gui, time_samples, 8000U);
    radio_gui_set_spectrum_data(gui, spectrum_db, 24000U, -120, 0);
}

static void band_changed(void * context, radio_gui_band_t band)
{
    (void)context;
    printf("[GUI] band: %s\n", band == RADIO_GUI_BAND_FM ? "FM" : "SW/AM/LW");
}

static bool frequency_submitted(void * context,
                                const radio_gui_frequency_request_t * request,
                                char * result_text,
                                size_t result_text_size)
{
    (void)context;
    const char * unit = request->unit == RADIO_GUI_FREQ_MHZ ? "MHz" : "kHz";
    (void)snprintf(result_text,
                   result_text_size,
                   "SI4732 FM tune success for %.3f MHz\nBLTF=1\nAFCRL=1\nVALID=1\nFreq=101500000Hz\nRSSI=-65dBm\nSNR=30dB\nMultipath=1",
                   request->value,
                   unit);
    printf("[GUI] tune: %.3f %s\n", request->value, unit);
    return true;
}

static void volume_changed(void * context, uint8_t volume)
{
    printf("[GUI] context: %p\n", context);
    printf("[GUI] volume: %u\n", (unsigned)volume);
}

static void channel_filter_changed(void * context, radio_gui_band_t band, const char * filter_name)
{
    (void)context;
    (void)band;
    printf("[GUI] channel filter: %s\n", filter_name);
}

static void audio_amp_enabled_changed(void * context, bool enabled)
{
    (void)context;
    printf("[GUI] audio amplifier: %s\n", enabled ? "on" : "off");
}

static void audio_amp_mode_changed(void * context, radio_gui_amp_mode_t mode)
{
    (void)context;
    printf("[GUI] amplifier mode: %s\n", mode == RADIO_GUI_AMP_CLASS_D ? "Class-D" : "Class-AB");
}

static void backlight_changed(void * context, uint8_t level)
{
    (void)context;
    printf("[GUI] backlight: %u\n", (unsigned)level);
}

static void gps_enabled_changed(void * context, bool enabled)
{
    (void)context;
    printf("[GUI] GPS: %s\n", enabled ? "enabled" : "disabled");
}

static void sd_logging_changed(void * context, bool enabled)
{
    (void)context;
    printf("[GUI] SD logging: %s\n", enabled ? "enabled" : "disabled");
}

radio_gui_config_t radio_gui_demo_backend_config(void)
{
    radio_gui_config_t config = {
        .callbacks = {
            .band_changed = band_changed,
            .frequency_submitted = frequency_submitted,
            .volume_changed = volume_changed,
            .channel_filter_changed = channel_filter_changed,
            .audio_amp_enabled_changed = audio_amp_enabled_changed,
            .audio_amp_mode_changed = audio_amp_mode_changed,
            .backlight_changed = backlight_changed,
            .gps_enabled_changed = gps_enabled_changed,
            .sd_logging_changed = sd_logging_changed,
        },
        .callback_context = NULL,
    };
    return config;
}

void radio_gui_demo_backend_populate(radio_gui_t * gui)
{
    radio_gui_set_band(gui, RADIO_GUI_BAND_FM);
    radio_gui_set_frequency_text(gui, "101.70 MHz");
    radio_gui_set_volume(gui, 32);
    radio_gui_set_rx_field(gui, RADIO_GUI_RX_AFCRL, "0");
    radio_gui_set_rx_field(gui, RADIO_GUI_RX_VALID, "YES");
    radio_gui_set_rx_field(gui, RADIO_GUI_RX_PILOT, "STEREO");
    radio_gui_set_rx_field(gui, RADIO_GUI_RX_STBLEND, "76%");
    radio_gui_set_rx_field(gui, RADIO_GUI_RX_RSSI, "48 dBuV");
    radio_gui_set_rx_field(gui, RADIO_GUI_RX_SNR, "31 dB");
    radio_gui_set_rx_field(gui, RADIO_GUI_RX_MULTIPATH, "4");
    radio_gui_set_rx_field(gui, RADIO_GUI_RX_FREQ_OFFSET, "+0.2 kHz");
    populate_signal_waveforms(gui);

    radio_gui_set_battery_voltage(gui, 3.97F);
    radio_gui_set_audio_amp_enabled(gui, true);
    radio_gui_set_audio_amp_mode(gui, RADIO_GUI_AMP_CLASS_D);
    radio_gui_set_backlight(gui, 38);

    radio_gui_set_gps_enabled(gui, true);
    radio_gui_set_sd_logging_enabled(gui, false);
    radio_gui_set_sd_status(gui, RADIO_GUI_SD_OK);
    radio_gui_set_gps_status(gui, RADIO_GUI_GPS_LOCKED);
    radio_gui_set_gps_field(gui, RADIO_GUI_GPS_LATITUDE, "27.4698 S");
    radio_gui_set_gps_field(gui, RADIO_GUI_GPS_LONGITUDE, "153.0251 E");
    radio_gui_set_gps_field(gui, RADIO_GUI_GPS_ALTITUDE_SEA, "18.4 m");
    radio_gui_set_gps_field(gui, RADIO_GUI_GPS_ALTITUDE, "42.1 m");
    radio_gui_set_gps_field(gui, RADIO_GUI_GPS_SPEED, "0.0 km/h");
    radio_gui_set_gps_field(gui, RADIO_GUI_GPS_DIRECTION, "--");
    radio_gui_set_gps_field(gui, RADIO_GUI_GPS_UTC_TIME, "06:42:18");
    radio_gui_set_gps_field(gui, RADIO_GUI_GPS_UTC_DATE, "2026-07-15");
    radio_gui_set_gps_field(gui, RADIO_GUI_GPS_MAGNETIC_DECLINATION, "11.2 E");
    radio_gui_set_gps_field(gui, RADIO_GUI_GPS_LOCATE_MODE, "3D FIX");
    radio_gui_set_gps_field(gui, RADIO_GUI_GPS_LOCATION_SYSTEM, "GPS + GLONASS");
    radio_gui_set_gps_field(gui, RADIO_GUI_GPS_LOCATION_TYPE, "Autonomous");
    radio_gui_set_gps_field(gui, RADIO_GUI_GPS_LOCATION_STATUS, "Valid");
    radio_gui_set_gps_field(gui, RADIO_GUI_GPS_SATELLITES, "12 / 18");
}
