#include "radio_gui_internal.h"

#include <stdio.h>
#include <string.h>

static const char *const rx_names[RADIO_GUI_RX_FIELD_COUNT] = {
    "AFCRL",
    "VALID",
    "PILOT",
    "STBLEND",
    "RSSI",
    "SNR",
    "Multipath",
    "Freq_offset"};

static const char *const am_filters =
    "BW_6KHZ\n"
    "BW_4KHZ\n"
    "BW_3KHZ\n"
    "BW_2K5HZ\n"
    "BW_2KHZ\n"
    "BW_1K8HZ\n"
    "BW_1KHZ";

static const char *const fm_filters =
    "BW_AUTO\n"
    "BW_110KHZ\n"
    "BW_84KHZ\n"
    "BW_60KHZ\n"
    "BW_40KHZ";

static void band_event_cb(lv_event_t *event);
static void frequency_event_cb(lv_event_t *event);
static void volume_event_cb(lv_event_t *event);
static void filter_event_cb(lv_event_t *event);

static lv_obj_t *create_title_value_row(lv_obj_t *parent,
                                        const char *title,
                                        lv_obj_t **value_label,
                                        const char *initial,
                                        lv_color_t accent)
{
  lv_obj_t *row = lv_obj_create(parent);
  lv_obj_remove_style_all(row);
  lv_obj_set_size(row, LV_PCT(100), 18);
  lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  radio_gui_create_section_label(row, title);
  *value_label = radio_gui_create_value_pill(row, initial, accent);
  return row;
}

void radio_gui_build_radio_page(radio_gui_t *gui)
{
  static const char *const band_map[] = {"SW / AM / LW", "FM", ""};

  lv_obj_t *content = radio_gui_create_page_content(gui->radio_tile);
  radio_gui_create_header(content, "RECEIVER", "Radio dashboard", "02 / 04", RADIO_GUI_COLOR_CYAN);

  lv_obj_t *frequency_card = radio_gui_create_card(content);
  lv_obj_set_style_border_color(frequency_card, RADIO_GUI_COLOR_CYAN, 0);
  lv_obj_set_style_border_opa(frequency_card, LV_OPA_40, 0);
  lv_obj_add_flag(frequency_card, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(frequency_card, frequency_event_cb, LV_EVENT_CLICKED, gui);

  lv_obj_t *freq_top = lv_obj_create(frequency_card);
  lv_obj_remove_style_all(freq_top);
  lv_obj_set_size(freq_top, LV_PCT(100), 18);
  lv_obj_set_flex_flow(freq_top, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(freq_top, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  radio_gui_create_section_label(freq_top, "RX FREQUENCY");
  radio_gui_create_value_pill(freq_top, "TAP TO TUNE", RADIO_GUI_COLOR_CYAN);

  gui->frequency_value = lv_label_create(frequency_card);
  lv_label_set_text(gui->frequency_value, "Please Enter Frequency");
  lv_obj_set_style_text_color(gui->frequency_value, RADIO_GUI_COLOR_TEXT, 0);
  lv_obj_set_style_text_font(gui->frequency_value, &lv_font_montserrat_24, 0);

  lv_obj_t *frequency_hint = lv_label_create(frequency_card);
  lv_label_set_text(frequency_hint, "Digital tuning  |  SI4732 ready");
  lv_obj_set_style_text_color(frequency_hint, RADIO_GUI_COLOR_MUTED, 0);
  lv_obj_set_style_text_font(frequency_hint, &lv_font_montserrat_10, 0);

  lv_obj_t *band_card = radio_gui_create_card(content);
  radio_gui_create_section_label(band_card, "RECEIVER BAND");
  gui->band_buttons = lv_buttonmatrix_create(band_card);
  lv_buttonmatrix_set_map(gui->band_buttons, band_map);
  lv_obj_set_size(gui->band_buttons, LV_PCT(100), 34);
  lv_obj_set_style_bg_opa(gui->band_buttons, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(gui->band_buttons, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(gui->band_buttons, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_column(gui->band_buttons, 6, LV_PART_MAIN);
  lv_obj_set_style_bg_color(gui->band_buttons, RADIO_GUI_COLOR_CARD_ALT, LV_PART_ITEMS);
  lv_obj_set_style_bg_color(gui->band_buttons, RADIO_GUI_COLOR_CYAN, LV_PART_ITEMS | LV_STATE_CHECKED);
  lv_obj_set_style_text_color(gui->band_buttons, RADIO_GUI_COLOR_MUTED, LV_PART_ITEMS);
  lv_obj_set_style_text_color(gui->band_buttons, RADIO_GUI_COLOR_BG, LV_PART_ITEMS | LV_STATE_CHECKED);
  lv_obj_set_style_text_font(gui->band_buttons, &lv_font_montserrat_10, LV_PART_ITEMS);
  lv_obj_set_style_border_width(gui->band_buttons, 0, LV_PART_ITEMS);
  lv_obj_set_style_radius(gui->band_buttons, 8, LV_PART_ITEMS);
  lv_buttonmatrix_set_one_checked(gui->band_buttons, true);
  lv_buttonmatrix_set_button_ctrl(gui->band_buttons, 0, LV_BUTTONMATRIX_CTRL_CHECKABLE);
  lv_buttonmatrix_set_button_ctrl(gui->band_buttons, 1, LV_BUTTONMATRIX_CTRL_CHECKABLE);
  lv_obj_add_event_cb(gui->band_buttons, band_event_cb, LV_EVENT_VALUE_CHANGED, gui);

  lv_obj_t *audio_card = radio_gui_create_card(content);
  create_title_value_row(audio_card, "AUDIO VOLUME", &gui->volume_value, "63 / 63", RADIO_GUI_COLOR_AMBER);
  gui->volume_slider = lv_slider_create(audio_card);
  lv_obj_set_width(gui->volume_slider, LV_PCT(100));
  lv_slider_set_range(gui->volume_slider, 0, 63);
  lv_slider_set_value(gui->volume_slider, 63, LV_ANIM_OFF);
  radio_gui_style_slider(gui->volume_slider, RADIO_GUI_COLOR_AMBER);
  lv_obj_add_event_cb(gui->volume_slider, volume_event_cb, LV_EVENT_VALUE_CHANGED, gui);

  lv_obj_t *filter_card = radio_gui_create_card(content);
  radio_gui_create_section_label(filter_card, "CHANNEL FILTER");
  gui->channel_filter = lv_dropdown_create(filter_card);
  lv_obj_set_width(gui->channel_filter, LV_PCT(100));
  lv_dropdown_set_options(gui->channel_filter, fm_filters);
  lv_obj_set_style_bg_color(gui->channel_filter, RADIO_GUI_COLOR_CARD_ALT, 0);
  lv_obj_set_style_border_color(gui->channel_filter, RADIO_GUI_COLOR_BORDER, 0);
  lv_obj_set_style_border_width(gui->channel_filter, 1, 0);
  lv_obj_set_style_radius(gui->channel_filter, 8, 0);
  lv_obj_set_style_text_color(gui->channel_filter, RADIO_GUI_COLOR_TEXT, 0);
  lv_obj_set_style_text_font(gui->channel_filter, &lv_font_montserrat_12, 0);
  lv_obj_set_style_pad_all(gui->channel_filter, 8, 0);
  lv_obj_add_event_cb(gui->channel_filter, filter_event_cb, LV_EVENT_VALUE_CHANGED, gui);

  lv_obj_t *signal_card = radio_gui_create_card(content);
  lv_obj_t *signal_heading = lv_obj_create(signal_card);
  lv_obj_remove_style_all(signal_heading);
  lv_obj_set_size(signal_heading, LV_PCT(100), 18);
  lv_obj_set_flex_flow(signal_heading, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(signal_heading, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_t *signal_title = radio_gui_create_section_label(signal_heading, "RX SIGNAL INFO");
  lv_obj_set_style_text_color(signal_title, RADIO_GUI_COLOR_TEXT, 0);
  lv_obj_set_style_text_font(signal_title, &lv_font_montserrat_12, 0);
  radio_gui_create_value_pill(signal_heading, "LIVE", RADIO_GUI_COLOR_GREEN);
  gui->rx_table = radio_gui_create_compact_table(signal_card, rx_names, RADIO_GUI_RX_FIELD_COUNT);

  radio_gui_set_band(gui, RADIO_GUI_BAND_FM);
}

void radio_gui_set_band(radio_gui_t *gui, radio_gui_band_t band)
{
  if (gui == NULL || gui->band_buttons == NULL)
    return;
  gui->band = band;
  radio_gui_sync_band_button_state(gui, band);
  lv_dropdown_set_options(gui->channel_filter,
                          band == RADIO_GUI_BAND_FM ? fm_filters : am_filters);
  lv_dropdown_set_selected(gui->channel_filter, 0);
}

void radio_gui_sync_band_button_state(radio_gui_t *gui, radio_gui_band_t band)
{
  if (gui == NULL || gui->band_buttons == NULL)
    return;
  lv_buttonmatrix_clear_button_ctrl_all(gui->band_buttons, LV_BUTTONMATRIX_CTRL_CHECKED);
  lv_buttonmatrix_set_button_ctrl(gui->band_buttons,
                                  band == RADIO_GUI_BAND_FM ? 1U : 0U,
                                  LV_BUTTONMATRIX_CTRL_CHECKED);
  lv_obj_invalidate(gui->band_buttons);
}

void radio_gui_set_frequency_text(radio_gui_t *gui, const char *text)
{
  if (gui == NULL || gui->frequency_value == NULL)
    return;
  lv_label_set_text(gui->frequency_value, text != NULL ? text : "--");
}

void radio_gui_set_volume(radio_gui_t *gui, uint8_t volume)
{
  if (gui == NULL || gui->volume_slider == NULL)
    return;
  if (volume > 63U)
    volume = 63U;
  lv_slider_set_value(gui->volume_slider, volume, LV_ANIM_OFF);
  lv_label_set_text_fmt(gui->volume_value, "%u / 63", (unsigned)volume);
}

void radio_gui_set_rx_field(radio_gui_t *gui, radio_gui_rx_field_t field, const char *value)
{
  if (gui == NULL || gui->rx_table == NULL || field >= RADIO_GUI_RX_FIELD_COUNT)
    return;
  lv_table_set_cell_value(gui->rx_table, (uint32_t)field, 1, value != NULL ? value : "--");
}

radio_gui_band_t radio_gui_get_band(const radio_gui_t *gui)
{
  return gui != NULL ? gui->band : RADIO_GUI_BAND_SW_AM_LW;
}

const char *radio_gui_get_frequency_text(const radio_gui_t *gui)
{
  if (gui == NULL || gui->frequency_value == NULL)
    return NULL;
  return lv_label_get_text(gui->frequency_value);
}

uint8_t radio_gui_get_volume(const radio_gui_t *gui)
{
  if (gui == NULL || gui->volume_slider == NULL)
    return 0U;
  return (uint8_t)lv_slider_get_value(gui->volume_slider);
}

bool radio_gui_get_channel_filter(const radio_gui_t *gui,
                                  char *filter_name,
                                  size_t filter_name_size)
{
  if (gui == NULL || gui->channel_filter == NULL || filter_name == NULL || filter_name_size == 0U)
  {
    return false;
  }
  uint32_t size = filter_name_size > UINT32_MAX ? UINT32_MAX : (uint32_t)filter_name_size;
  lv_dropdown_get_selected_str(gui->channel_filter, filter_name, size);
  return true;
}

const char *radio_gui_get_rx_field(const radio_gui_t *gui, radio_gui_rx_field_t field)
{
  if (gui == NULL || gui->rx_table == NULL || field >= RADIO_GUI_RX_FIELD_COUNT)
    return NULL;
  return lv_table_get_cell_value(gui->rx_table, (uint32_t)field, 1);
}

static void band_event_cb(lv_event_t *event)
{
  radio_gui_t *gui = lv_event_get_user_data(event);
  lv_obj_t *button_matrix = lv_event_get_target_obj(event);
  uint32_t selected = lv_buttonmatrix_get_selected_button(button_matrix);
  if (selected > 1U)
    return;
  radio_gui_band_t band = selected == 1U ? RADIO_GUI_BAND_FM : RADIO_GUI_BAND_SW_AM_LW;
  radio_gui_sync_band_button_state(gui, gui->band);
  radio_gui_open_band_confirmation(gui, band);
}

static void frequency_event_cb(lv_event_t *event)
{
  radio_gui_open_keypad(lv_event_get_user_data(event));
}

static void volume_event_cb(lv_event_t *event)
{
  radio_gui_t *gui = lv_event_get_user_data(event);
  uint8_t volume = (uint8_t)lv_slider_get_value(gui->volume_slider);
  lv_label_set_text_fmt(gui->volume_value, "%u / 63", (unsigned)volume);
  if (gui->config.callbacks.volume_changed != NULL)
  {
    gui->config.callbacks.volume_changed(gui->config.callback_context, volume);
  }
}

static void filter_event_cb(lv_event_t *event)
{
  radio_gui_t *gui = lv_event_get_user_data(event);
  char selected[32];
  lv_dropdown_get_selected_str(gui->channel_filter, selected, sizeof(selected));
  if (gui->config.callbacks.channel_filter_changed != NULL)
  {
    gui->config.callbacks.channel_filter_changed(gui->config.callback_context, gui->band, selected);
  }
}
