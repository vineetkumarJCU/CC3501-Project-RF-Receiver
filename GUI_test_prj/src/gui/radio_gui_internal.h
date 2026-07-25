#ifndef RADIO_GUI_INTERNAL_H
#define RADIO_GUI_INTERNAL_H

#include "radio_gui.h"

#define RADIO_GUI_PAGE_WIDTH 320
#define RADIO_GUI_PAGE_HEIGHT 240

#define RADIO_GUI_COLOR_BG          lv_color_hex(0x0A0E14)
#define RADIO_GUI_COLOR_CARD        lv_color_hex(0x131A23)
#define RADIO_GUI_COLOR_CARD_ALT    lv_color_hex(0x18222D)
#define RADIO_GUI_COLOR_BORDER      lv_color_hex(0x263443)
#define RADIO_GUI_COLOR_TEXT        lv_color_hex(0xECF4FA)
#define RADIO_GUI_COLOR_MUTED       lv_color_hex(0x7F91A2)
#define RADIO_GUI_COLOR_CYAN        lv_color_hex(0x42D9C8)
#define RADIO_GUI_COLOR_BLUE        lv_color_hex(0x65AFFF)
#define RADIO_GUI_COLOR_AMBER       lv_color_hex(0xFFB45B)
#define RADIO_GUI_COLOR_GREEN       lv_color_hex(0x5AD69B)
#define RADIO_GUI_COLOR_RED         lv_color_hex(0xFF6B72)

struct radio_gui {
    radio_gui_config_t config;
    lv_obj_t * root;
    lv_obj_t * tileview;
    lv_obj_t * hardware_tile;
    lv_obj_t * radio_tile;
    lv_obj_t * waveform_tile;
    lv_obj_t * gps_tile;

    radio_gui_band_t band;
    radio_gui_band_t pending_band;
    radio_gui_frequency_unit_t frequency_unit;
    float battery_voltage;
    radio_gui_sd_status_t sd_status;
    radio_gui_gps_status_t gps_status;

    lv_obj_t * band_buttons;
    lv_obj_t * frequency_value;
    lv_obj_t * volume_slider;
    lv_obj_t * volume_value;
    lv_obj_t * channel_filter;
    lv_obj_t * rx_table;

    lv_obj_t * time_chart;
    lv_chart_series_t * time_series;
    lv_obj_t * time_x_labels[3];
    lv_obj_t * spectrum_chart;
    lv_chart_series_t * spectrum_series;
    lv_obj_t * spectrum_x_labels[5];
    lv_obj_t * spectrum_y_labels[3];

    lv_obj_t * battery_value;
    lv_obj_t * battery_bar;
    lv_obj_t * amp_switch;
    lv_obj_t * amp_state_label;
    lv_obj_t * amp_mode_dropdown;
    lv_obj_t * backlight_slider;
    lv_obj_t * backlight_value;

    lv_obj_t * gps_switch;
    lv_obj_t * sd_log_switch;
    lv_obj_t * sd_status_label;
    lv_obj_t * gps_status_label;
    lv_obj_t * gps_hero_position;
    lv_obj_t * gps_table;

    lv_obj_t * keypad_overlay;
    lv_obj_t * keypad_textarea;
    lv_obj_t * keypad_units;
    lv_obj_t * result_overlay;
    lv_obj_t * result_title;
    lv_obj_t * result_text;
    lv_obj_t * band_confirm_overlay;
    lv_obj_t * band_confirm_target;
};

lv_obj_t * radio_gui_create_page_content(lv_obj_t * tile);
lv_obj_t * radio_gui_create_header(lv_obj_t * parent,
                                   const char * eyebrow,
                                   const char * title,
                                   const char * page_number,
                                   lv_color_t accent);
lv_obj_t * radio_gui_create_card(lv_obj_t * parent);
lv_obj_t * radio_gui_create_section_label(lv_obj_t * parent, const char * text);
lv_obj_t * radio_gui_create_value_pill(lv_obj_t * parent, const char * text, lv_color_t color);
lv_obj_t * radio_gui_create_compact_table(lv_obj_t * parent,
                                          const char * const * names,
                                          uint32_t row_count);
void radio_gui_style_slider(lv_obj_t * slider, lv_color_t accent);
void radio_gui_style_switch(lv_obj_t * sw, lv_color_t accent);
void radio_gui_set_status_pill(lv_obj_t * label,
                               const char * text,
                               lv_color_t color);

void radio_gui_build_radio_page(radio_gui_t * gui);
void radio_gui_build_waveform_page(radio_gui_t * gui);
void radio_gui_build_hardware_page(radio_gui_t * gui);
void radio_gui_build_gps_page(radio_gui_t * gui);
void radio_gui_build_overlays(radio_gui_t * gui);
void radio_gui_open_keypad(radio_gui_t * gui);
void radio_gui_open_band_confirmation(radio_gui_t * gui, radio_gui_band_t band);
void radio_gui_sync_band_button_state(radio_gui_t * gui, radio_gui_band_t band);

#endif /* RADIO_GUI_INTERNAL_H */
