#include "radio_gui_internal.h"

static const char *const gps_names[RADIO_GUI_GPS_FIELD_COUNT] = {
    "Latitude",
    "Longitude",
    "Altitude (Sea)",
    "Altitude",
    "Speed",
    "Direction",
    "UTC time",
    "UTC date",
    "Magnetic declination",
    "Locate mode",
    "Location system",
    "Location type",
    "Location status",
    "Satellites"};

static void gps_switch_event_cb(lv_event_t *event);
static void sd_log_switch_event_cb(lv_event_t *event);

static lv_obj_t *create_switch_row(lv_obj_t *parent,
                                   const char *title,
                                   const char *subtitle,
                                   lv_color_t accent,
                                   lv_obj_t **switch_out,
                                   lv_event_cb_t event_cb,
                                   radio_gui_t *gui)
{
  lv_obj_t *row = lv_obj_create(parent);
  lv_obj_remove_style_all(row);
  lv_obj_set_size(row, LV_PCT(100), 36);
  lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  lv_obj_t *labels = lv_obj_create(row);
  lv_obj_remove_style_all(labels);
  lv_obj_set_size(labels, 215, LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(labels, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_row(labels, 1, 0);

  lv_obj_t *title_label = lv_label_create(labels);
  lv_label_set_text(title_label, title);
  lv_obj_set_style_text_color(title_label, RADIO_GUI_COLOR_TEXT, 0);
  lv_obj_set_style_text_font(title_label, &lv_font_montserrat_12, 0);

  lv_obj_t *subtitle_label = lv_label_create(labels);
  lv_label_set_text(subtitle_label, subtitle);
  lv_obj_set_style_text_color(subtitle_label, RADIO_GUI_COLOR_MUTED, 0);
  lv_obj_set_style_text_font(subtitle_label, &lv_font_montserrat_10, 0);

  *switch_out = lv_switch_create(row);
  radio_gui_style_switch(*switch_out, accent);
  lv_obj_add_event_cb(*switch_out, event_cb, LV_EVENT_VALUE_CHANGED, gui);
  return row;
}

void radio_gui_build_gps_page(radio_gui_t *gui)
{
  lv_obj_t *content = radio_gui_create_page_content(gui->gps_tile);
  radio_gui_create_header(content, "NAVIGATION", "GPS & data log", "04 / 04", RADIO_GUI_COLOR_BLUE);

  lv_obj_t *status_card = radio_gui_create_card(content);
  radio_gui_create_section_label(status_card, "SYSTEM STATUS");
  lv_obj_t *status_row = lv_obj_create(status_card);
  lv_obj_remove_style_all(status_row);
  lv_obj_set_size(status_row, LV_PCT(100), 25);
  lv_obj_set_flex_flow(status_row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(status_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(status_row, 7, 0);
  gui->gps_status_label = radio_gui_create_value_pill(status_row, "GPS unlocked", RADIO_GUI_COLOR_AMBER);
  gui->sd_status_label = radio_gui_create_value_pill(status_row, "SD OK", RADIO_GUI_COLOR_GREEN);

  lv_obj_t *control_card = radio_gui_create_card(content);
  create_switch_row(control_card,
                    "Enable GPS",
                    "Power and acquire satellites",
                    RADIO_GUI_COLOR_BLUE,
                    &gui->gps_switch,
                    gps_switch_event_cb,
                    gui);
  lv_obj_t *separator = lv_obj_create(control_card);
  lv_obj_remove_style_all(separator);
  lv_obj_set_size(separator, LV_PCT(100), 1);
  lv_obj_set_style_bg_color(separator, RADIO_GUI_COLOR_BORDER, 0);
  lv_obj_set_style_bg_opa(separator, LV_OPA_COVER, 0);
  create_switch_row(control_card,
                    "Record log in SD card",
                    "Position, UTC and receiver signal",
                    RADIO_GUI_COLOR_CYAN,
                    &gui->sd_log_switch,
                    sd_log_switch_event_cb,
                    gui);

  lv_obj_t *position_card = radio_gui_create_card(content);
  lv_obj_set_style_border_color(position_card, RADIO_GUI_COLOR_BLUE, 0);
  lv_obj_set_style_border_opa(position_card, LV_OPA_40, 0);
  radio_gui_create_section_label(position_card, "CURRENT POSITION");
  gui->gps_hero_position = lv_label_create(position_card);
  lv_label_set_text(gui->gps_hero_position, "--\n--");
  lv_obj_set_style_text_color(gui->gps_hero_position, RADIO_GUI_COLOR_TEXT, 0);
  lv_obj_set_style_text_font(gui->gps_hero_position, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_line_space(gui->gps_hero_position, 3, 0);
  lv_obj_t *position_hint = lv_label_create(position_card);
  lv_label_set_text(position_hint, "Latitude / Longitude  |  live GNSS fix");
  lv_obj_set_style_text_color(position_hint, RADIO_GUI_COLOR_MUTED, 0);
  lv_obj_set_style_text_font(position_hint, &lv_font_montserrat_10, 0);

  lv_obj_t *gps_table_card = radio_gui_create_card(content);
  lv_obj_t *table_heading = lv_obj_create(gps_table_card);
  lv_obj_remove_style_all(table_heading);
  lv_obj_set_size(table_heading, LV_PCT(100), 18);
  lv_obj_set_flex_flow(table_heading, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(table_heading, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_t *table_title = radio_gui_create_section_label(table_heading, "GPS TELEMETRY");
  lv_obj_set_style_text_color(table_title, RADIO_GUI_COLOR_TEXT, 0);
  lv_obj_set_style_text_font(table_title, &lv_font_montserrat_12, 0);
  radio_gui_create_value_pill(table_heading, "14 FIELDS", RADIO_GUI_COLOR_BLUE);
  gui->gps_table = radio_gui_create_compact_table(gps_table_card, gps_names, RADIO_GUI_GPS_FIELD_COUNT);

  radio_gui_set_gps_enabled(gui, true);
  radio_gui_set_sd_logging_enabled(gui, false);
  radio_gui_set_sd_status(gui, RADIO_GUI_SD_OK);
  radio_gui_set_gps_status(gui, RADIO_GUI_GPS_UNLOCKED);
}

void radio_gui_set_gps_enabled(radio_gui_t *gui, bool enabled)
{
  if (gui == NULL || gui->gps_switch == NULL)
    return;
  if (enabled)
    lv_obj_add_state(gui->gps_switch, LV_STATE_CHECKED);
  else
    lv_obj_remove_state(gui->gps_switch, LV_STATE_CHECKED);
}

void radio_gui_set_sd_logging_enabled(radio_gui_t *gui, bool enabled)
{
  if (gui == NULL || gui->sd_log_switch == NULL)
    return;
  if (enabled)
    lv_obj_add_state(gui->sd_log_switch, LV_STATE_CHECKED);
  else
    lv_obj_remove_state(gui->sd_log_switch, LV_STATE_CHECKED);
}

void radio_gui_set_sd_status(radio_gui_t *gui, radio_gui_sd_status_t status)
{
  if (gui == NULL || gui->sd_status_label == NULL)
    return;
  gui->sd_status = status;
  switch (status)
  {
  case RADIO_GUI_SD_OK:
    radio_gui_set_status_pill(gui->sd_status_label, "SD OK", RADIO_GUI_COLOR_GREEN);
    break;
  case RADIO_GUI_SD_ERROR:
    radio_gui_set_status_pill(gui->sd_status_label, "SD error", RADIO_GUI_COLOR_RED);
    break;
  case RADIO_GUI_SD_NO_CARD:
  default:
    radio_gui_set_status_pill(gui->sd_status_label, "no SD", RADIO_GUI_COLOR_MUTED);
    break;
  }
}

void radio_gui_set_gps_status(radio_gui_t *gui, radio_gui_gps_status_t status)
{
  if (gui == NULL || gui->gps_status_label == NULL)
    return;
  gui->gps_status = status;
  switch (status)
  {
  case RADIO_GUI_GPS_LOCKED:
    radio_gui_set_status_pill(gui->gps_status_label, "GPS locked", RADIO_GUI_COLOR_GREEN);
    break;
  case RADIO_GUI_GPS_ERROR:
    radio_gui_set_status_pill(gui->gps_status_label, "GPS error", RADIO_GUI_COLOR_RED);
    break;
  case RADIO_GUI_GPS_UNLOCKED:
  default:
    radio_gui_set_status_pill(gui->gps_status_label, "GPS unlocked", RADIO_GUI_COLOR_AMBER);
    break;
  }
}

void radio_gui_set_gps_field(radio_gui_t *gui, radio_gui_gps_field_t field, const char *value)
{
  if (gui == NULL || gui->gps_table == NULL || field >= RADIO_GUI_GPS_FIELD_COUNT)
    return;
  lv_table_set_cell_value(gui->gps_table, (uint32_t)field, 1, value != NULL ? value : "--");

  if (field == RADIO_GUI_GPS_LATITUDE || field == RADIO_GUI_GPS_LONGITUDE)
  {
    const char *latitude = lv_table_get_cell_value(gui->gps_table, RADIO_GUI_GPS_LATITUDE, 1);
    const char *longitude = lv_table_get_cell_value(gui->gps_table, RADIO_GUI_GPS_LONGITUDE, 1);
    lv_label_set_text_fmt(gui->gps_hero_position, "%s\n%s", latitude, longitude);
  }
}

bool radio_gui_get_gps_enabled(const radio_gui_t *gui)
{
  return gui != NULL && gui->gps_switch != NULL && lv_obj_has_state(gui->gps_switch, LV_STATE_CHECKED);
}

bool radio_gui_get_sd_logging_enabled(const radio_gui_t *gui)
{
  return gui != NULL && gui->sd_log_switch != NULL && lv_obj_has_state(gui->sd_log_switch, LV_STATE_CHECKED);
}

radio_gui_sd_status_t radio_gui_get_sd_status(const radio_gui_t *gui)
{
  return gui != NULL ? gui->sd_status : RADIO_GUI_SD_NO_CARD;
}

radio_gui_gps_status_t radio_gui_get_gps_status(const radio_gui_t *gui)
{
  return gui != NULL ? gui->gps_status : RADIO_GUI_GPS_UNLOCKED;
}

const char *radio_gui_get_gps_field(const radio_gui_t *gui, radio_gui_gps_field_t field)
{
  if (gui == NULL || gui->gps_table == NULL || field >= RADIO_GUI_GPS_FIELD_COUNT)
    return NULL;
  return lv_table_get_cell_value(gui->gps_table, (uint32_t)field, 1);
}

static void gps_switch_event_cb(lv_event_t *event)
{
  radio_gui_t *gui = lv_event_get_user_data(event);
  bool enabled = lv_obj_has_state(gui->gps_switch, LV_STATE_CHECKED);
  if (gui->config.callbacks.gps_enabled_changed != NULL)
  {
    gui->config.callbacks.gps_enabled_changed(gui->config.callback_context, enabled);
  }
}

static void sd_log_switch_event_cb(lv_event_t *event)
{
  radio_gui_t *gui = lv_event_get_user_data(event);
  bool enabled = lv_obj_has_state(gui->sd_log_switch, LV_STATE_CHECKED);
  if (gui->config.callbacks.sd_logging_changed != NULL)
  {
    gui->config.callbacks.sd_logging_changed(gui->config.callback_context, enabled);
  }
}
