#include "radio_gui_internal.h"

#include <stdio.h>

static void amp_switch_event_cb(lv_event_t *event);
static void amp_mode_event_cb(lv_event_t *event);
static void backlight_event_cb(lv_event_t *event);

static lv_obj_t *create_compact_card(lv_obj_t *parent, int32_t width, int32_t height)
{
  lv_obj_t *card = radio_gui_create_card(parent);
  lv_obj_set_size(card, width, height);
  lv_obj_set_style_pad_all(card, 9, 0);
  lv_obj_set_style_pad_row(card, 6, 0);
  return card;
}

void radio_gui_build_hardware_page(radio_gui_t *gui)
{
  lv_obj_t *content = radio_gui_create_page_content(gui->hardware_tile);
  lv_obj_set_style_pad_row(content, 7, 0);
  radio_gui_create_header(content, "SYSTEM", "Hardware settings", "01 / 04", RADIO_GUI_COLOR_AMBER);

  lv_obj_t *feature_row = lv_obj_create(content);
  lv_obj_remove_style_all(feature_row);
  lv_obj_set_size(feature_row, LV_PCT(100), 105);
  lv_obj_set_flex_flow(feature_row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(feature_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);

  lv_obj_t *battery_card = create_compact_card(feature_row, 137, 105);
  radio_gui_create_section_label(battery_card, "BATTERY / ADC0");
  gui->battery_value = lv_label_create(battery_card);
  lv_label_set_text(gui->battery_value, "3.97 V");
  lv_obj_set_style_text_color(gui->battery_value, RADIO_GUI_COLOR_TEXT, 0);
  lv_obj_set_style_text_font(gui->battery_value, &lv_font_montserrat_22, 0);
  radio_gui_create_value_pill(battery_card, "Battery Volume", RADIO_GUI_COLOR_GREEN);
  gui->battery_bar = lv_bar_create(battery_card);
  lv_obj_set_size(gui->battery_bar, LV_PCT(100), 7);
  lv_bar_set_range(gui->battery_bar, 3300, 4200);
  lv_bar_set_value(gui->battery_bar, 3970, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(gui->battery_bar, RADIO_GUI_COLOR_CARD_ALT, LV_PART_MAIN);
  lv_obj_set_style_bg_color(gui->battery_bar, RADIO_GUI_COLOR_GREEN, LV_PART_INDICATOR);
  lv_obj_set_style_radius(gui->battery_bar, 6, LV_PART_MAIN);
  lv_obj_set_style_radius(gui->battery_bar, 6, LV_PART_INDICATOR);

  lv_obj_t *amp_card = create_compact_card(feature_row, 137, 105);
  radio_gui_create_section_label(amp_card, "AUDIO AMPLIFIER");
  lv_obj_t *amp_row = lv_obj_create(amp_card);
  lv_obj_remove_style_all(amp_row);
  lv_obj_set_size(amp_row, LV_PCT(100), 25);
  lv_obj_set_flex_flow(amp_row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(amp_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  gui->amp_state_label = lv_label_create(amp_row);
  lv_label_set_text(gui->amp_state_label, "Audio AMP ON");
  lv_obj_set_style_text_color(gui->amp_state_label, RADIO_GUI_COLOR_GREEN, 0);
  lv_obj_set_style_text_font(gui->amp_state_label, &lv_font_montserrat_10, 0);
  gui->amp_switch = lv_switch_create(amp_row);
  radio_gui_style_switch(gui->amp_switch, RADIO_GUI_COLOR_AMBER);
  lv_obj_add_state(gui->amp_switch, LV_STATE_CHECKED);
  lv_obj_add_event_cb(gui->amp_switch, amp_switch_event_cb, LV_EVENT_VALUE_CHANGED, gui);

  gui->amp_mode_dropdown = lv_dropdown_create(amp_card);
  lv_dropdown_set_options(gui->amp_mode_dropdown, "Class-AB\nClass-D");
  lv_obj_set_width(gui->amp_mode_dropdown, LV_PCT(100));
  lv_obj_set_style_bg_color(gui->amp_mode_dropdown, RADIO_GUI_COLOR_CARD_ALT, 0);
  lv_obj_set_style_border_width(gui->amp_mode_dropdown, 0, 0);
  lv_obj_set_style_radius(gui->amp_mode_dropdown, 7, 0);
  lv_obj_set_style_text_color(gui->amp_mode_dropdown, RADIO_GUI_COLOR_TEXT, 0);
  lv_obj_set_style_text_font(gui->amp_mode_dropdown, &lv_font_montserrat_10, 0);
  lv_obj_set_style_pad_all(gui->amp_mode_dropdown, 7, 0);
  lv_obj_add_event_cb(gui->amp_mode_dropdown, amp_mode_event_cb, LV_EVENT_VALUE_CHANGED, gui);

  lv_obj_t *backlight_card = create_compact_card(content, LV_PCT(100), 56);
  lv_obj_t *backlight_row = lv_obj_create(backlight_card);
  lv_obj_remove_style_all(backlight_row);
  lv_obj_set_size(backlight_row, LV_PCT(100), 18);
  lv_obj_set_flex_flow(backlight_row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(backlight_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  radio_gui_create_section_label(backlight_row, "SCREEN BACKLIGHT");
  lv_obj_remove_flag(backlight_row, LV_OBJ_FLAG_SCROLLABLE);
  gui->backlight_value = radio_gui_create_value_pill(backlight_row, "49 / 49", RADIO_GUI_COLOR_BLUE);
  gui->backlight_slider = lv_slider_create(backlight_card);
  lv_obj_set_width(gui->backlight_slider, LV_PCT(100));
  lv_slider_set_range(gui->backlight_slider, 0, 49);
  lv_slider_set_value(gui->backlight_slider, 38, LV_ANIM_OFF);
  radio_gui_style_slider(gui->backlight_slider, RADIO_GUI_COLOR_BLUE);
  lv_obj_add_event_cb(gui->backlight_slider, backlight_event_cb, LV_EVENT_VALUE_CHANGED, gui);
}

void radio_gui_set_battery_voltage(radio_gui_t *gui, float voltage)
{
  if (gui == NULL || gui->battery_value == NULL)
    return;
  gui->battery_voltage = voltage;
  int32_t millivolts = (int32_t)(voltage * 1000.0F + 0.5F);
  if (millivolts < 2700)
    millivolts = 2700;
  if (millivolts > 4200)
    millivolts = 4200;
  int32_t whole_volts = millivolts / 1000;
  int32_t hundredths = (millivolts % 1000) / 10;
  lv_label_set_text_fmt(gui->battery_value,
                        "%ld.%02ld V",
                        (long)whole_volts,
                        (long)hundredths);
  lv_bar_set_value(gui->battery_bar, millivolts, LV_ANIM_ON);
}

void radio_gui_set_audio_amp_enabled(radio_gui_t *gui, bool enabled)
{
  if (gui == NULL || gui->amp_switch == NULL)
    return;
  if (enabled)
    lv_obj_add_state(gui->amp_switch, LV_STATE_CHECKED);
  else
    lv_obj_remove_state(gui->amp_switch, LV_STATE_CHECKED);
  lv_label_set_text(gui->amp_state_label, enabled ? "Audio AMP ON" : "Audio AMP OFF");
  lv_obj_set_style_text_color(gui->amp_state_label,
                              enabled ? RADIO_GUI_COLOR_GREEN : RADIO_GUI_COLOR_MUTED,
                              0);
}

void radio_gui_set_audio_amp_mode(radio_gui_t *gui, radio_gui_amp_mode_t mode)
{
  if (gui == NULL || gui->amp_mode_dropdown == NULL)
    return;
  lv_dropdown_set_selected(gui->amp_mode_dropdown, mode == RADIO_GUI_AMP_CLASS_D ? 1U : 0U);
}

void radio_gui_set_backlight(radio_gui_t *gui, uint8_t level)
{
  if (gui == NULL || gui->backlight_slider == NULL)
    return;
  if (level > 49U)
    level = 49U;
  lv_slider_set_value(gui->backlight_slider, level, LV_ANIM_OFF);
  lv_label_set_text_fmt(gui->backlight_value, "%u / 49", (unsigned)level);
}

float radio_gui_get_battery_voltage(const radio_gui_t *gui)
{
  return gui != NULL ? gui->battery_voltage : 0.0F;
}

bool radio_gui_get_audio_amp_enabled(const radio_gui_t *gui)
{
  return gui != NULL && gui->amp_switch != NULL && lv_obj_has_state(gui->amp_switch, LV_STATE_CHECKED);
}

radio_gui_amp_mode_t radio_gui_get_audio_amp_mode(const radio_gui_t *gui)
{
  if (gui == NULL || gui->amp_mode_dropdown == NULL)
    return RADIO_GUI_AMP_CLASS_AB;
  return lv_dropdown_get_selected(gui->amp_mode_dropdown) == 0U
             ? RADIO_GUI_AMP_CLASS_AB
             : RADIO_GUI_AMP_CLASS_D;
}

uint8_t radio_gui_get_backlight(const radio_gui_t *gui)
{
  if (gui == NULL || gui->backlight_slider == NULL)
    return 0U;
  return (uint8_t)lv_slider_get_value(gui->backlight_slider);
}

static void amp_switch_event_cb(lv_event_t *event)
{
  radio_gui_t *gui = lv_event_get_user_data(event);
  bool enabled = lv_obj_has_state(gui->amp_switch, LV_STATE_CHECKED);
  radio_gui_set_audio_amp_enabled(gui, enabled);
  if (gui->config.callbacks.audio_amp_enabled_changed != NULL)
  {
    gui->config.callbacks.audio_amp_enabled_changed(gui->config.callback_context, lv_dropdown_get_selected(gui->amp_mode_dropdown) == 0U ? RADIO_GUI_AMP_CLASS_AB : RADIO_GUI_AMP_CLASS_D, enabled);
  }
}

static void amp_mode_event_cb(lv_event_t *event)
{
  radio_gui_t *gui = lv_event_get_user_data(event);
  radio_gui_amp_mode_t mode = lv_dropdown_get_selected(gui->amp_mode_dropdown) == 0U
                                  ? RADIO_GUI_AMP_CLASS_AB
                                  : RADIO_GUI_AMP_CLASS_D;
  if (gui->config.callbacks.audio_amp_mode_changed != NULL)
  {
    gui->config.callbacks.audio_amp_mode_changed(gui->config.callback_context, mode, lv_obj_has_state(gui->amp_switch, LV_STATE_CHECKED));
  }
}

static void backlight_event_cb(lv_event_t *event)
{
  radio_gui_t *gui = lv_event_get_user_data(event);
  uint8_t level = (uint8_t)lv_slider_get_value(gui->backlight_slider);
  lv_label_set_text_fmt(gui->backlight_value, "%u / 49", (unsigned)level);
  if (gui->config.callbacks.backlight_changed != NULL)
  {
    gui->config.callbacks.backlight_changed(gui->config.callback_context, level);
  }
}
