#include "radio_gui_internal.h"

#include <stdio.h>

enum {
    WAVEFORM_HEADER_HEIGHT = 22,
    PLOT_CARD_HEIGHT = 104,
    PLOT_TITLE_HEIGHT = 14,
    PLOT_BODY_HEIGHT = 83,
    PLOT_Y_LABEL_WIDTH = 27,
    PLOT_CHART_WIDTH = 270,
    PLOT_CHART_HEIGHT = 71,
    PLOT_X_AXIS_HEIGHT = 12,
    PLOT_X_LABEL_WIDTH = 54,
    TIME_X_TICK_COUNT = 3,
    SPECTRUM_X_TICK_COUNT = 5
};

static lv_obj_t * create_axis_label(lv_obj_t * parent, const char * text)
{
    lv_obj_t * label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, RADIO_GUI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_10, 0);
    return label;
}

static void create_waveform_header(lv_obj_t * parent)
{
    lv_obj_t * header = lv_obj_create(parent);
    lv_obj_remove_style_all(header);
    lv_obj_set_size(header, LV_PCT(100), WAVEFORM_HEADER_HEIGHT);
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header,
                          LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);

    lv_obj_t * title = lv_label_create(header);
    lv_label_set_text(title, "RX signal waveform");
    lv_obj_set_style_text_color(title, RADIO_GUI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);

    lv_obj_t * page = radio_gui_create_value_pill(header, "03 / 04", RADIO_GUI_COLOR_GREEN);
    lv_obj_set_style_text_font(page, &lv_font_montserrat_10, 0);
    lv_obj_set_style_pad_hor(page, 7, 0);
    lv_obj_set_style_pad_ver(page, 1, 0);
}

static void create_plot_card(lv_obj_t * parent,
                             const char * title,
                             const char * format_text,
                             lv_color_t accent,
                             const char * y_top,
                             const char * y_middle,
                             const char * y_bottom,
                             lv_obj_t ** chart_out,
                             lv_chart_series_t ** series_out,
                             lv_obj_t ** x_labels,
                             uint32_t x_tick_count,
                             lv_obj_t * y_labels[3])
{
    lv_obj_t * card = radio_gui_create_card(parent);
    lv_obj_set_height(card, PLOT_CARD_HEIGHT);
    lv_obj_set_style_pad_all(card, 2, 0);
    lv_obj_set_style_pad_row(card, 1, 0);
    lv_obj_set_style_border_color(card, accent, 0);
    lv_obj_set_style_border_opa(card, LV_OPA_40, 0);

    lv_obj_t * title_row = lv_obj_create(card);
    lv_obj_remove_style_all(title_row);
    lv_obj_set_size(title_row, LV_PCT(100), PLOT_TITLE_HEIGHT);
    lv_obj_set_flex_flow(title_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(title_row,
                          LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);

    lv_obj_t * title_label = lv_label_create(title_row);
    lv_label_set_text(title_label, title);
    lv_obj_set_style_text_color(title_label, RADIO_GUI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_12, 0);

    lv_obj_t * format_label = radio_gui_create_value_pill(title_row, format_text, accent);
    lv_obj_set_style_text_font(format_label, &lv_font_montserrat_10, 0);
    lv_obj_set_style_pad_hor(format_label, 5, 0);
    lv_obj_set_style_pad_ver(format_label, 1, 0);

    lv_obj_t * plot_body = lv_obj_create(card);
    lv_obj_remove_style_all(plot_body);
    lv_obj_set_size(plot_body, LV_PCT(100), PLOT_BODY_HEIGHT);
    lv_obj_remove_flag(plot_body, LV_OBJ_FLAG_SCROLLABLE);

    y_labels[0] = create_axis_label(plot_body, y_top);
    lv_obj_set_width(y_labels[0], PLOT_Y_LABEL_WIDTH);
    lv_obj_set_style_text_align(y_labels[0], LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_align(y_labels[0], LV_ALIGN_TOP_LEFT, 0, -2);

    y_labels[1] = create_axis_label(plot_body, y_middle);
    lv_obj_set_width(y_labels[1], PLOT_Y_LABEL_WIDTH);
    lv_obj_set_style_text_align(y_labels[1], LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_align(y_labels[1], LV_ALIGN_LEFT_MID, 0, -6);

    y_labels[2] = create_axis_label(plot_body, y_bottom);
    lv_obj_set_width(y_labels[2], PLOT_Y_LABEL_WIDTH);
    lv_obj_set_style_text_align(y_labels[2], LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_align(y_labels[2], LV_ALIGN_BOTTOM_LEFT, 0, 1);

    lv_obj_t * chart = lv_chart_create(plot_body);
    lv_obj_set_pos(chart, PLOT_Y_LABEL_WIDTH + 1, 0);
    lv_obj_set_size(chart, PLOT_CHART_WIDTH, PLOT_CHART_HEIGHT);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(chart, RADIO_GUI_WAVEFORM_POINT_COUNT);
    lv_chart_set_div_line_count(chart, 3, 5);
    lv_obj_set_style_bg_color(chart, RADIO_GUI_COLOR_CARD_ALT, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(chart, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(chart, accent, LV_PART_MAIN);
    lv_obj_set_style_border_opa(chart, LV_OPA_60, LV_PART_MAIN);
    lv_obj_set_style_border_width(chart, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(chart, 4, LV_PART_MAIN);
    lv_obj_set_style_pad_all(chart, 0, LV_PART_MAIN);
    lv_obj_set_style_line_color(chart, RADIO_GUI_COLOR_BORDER, LV_PART_MAIN);
    lv_obj_set_style_line_opa(chart, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_line_width(chart, 1, LV_PART_MAIN);
    lv_obj_set_style_line_dash_width(chart, 3, LV_PART_MAIN);
    lv_obj_set_style_line_dash_gap(chart, 3, LV_PART_MAIN);
    lv_obj_set_style_line_width(chart, 2, LV_PART_ITEMS);
    lv_obj_set_style_line_opa(chart, LV_OPA_COVER, LV_PART_ITEMS);
    lv_obj_set_style_width(chart, 0, LV_PART_INDICATOR);
    lv_obj_set_style_height(chart, 0, LV_PART_INDICATOR);
    lv_obj_remove_flag(chart, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(chart, LV_OBJ_FLAG_CLICKABLE);

    lv_chart_series_t * series = lv_chart_add_series(chart, accent, LV_CHART_AXIS_PRIMARY_Y);

    lv_obj_t * x_axis = lv_obj_create(plot_body);
    lv_obj_remove_style_all(x_axis);
    lv_obj_set_pos(x_axis, PLOT_Y_LABEL_WIDTH + 1, PLOT_CHART_HEIGHT);
    lv_obj_set_size(x_axis, PLOT_CHART_WIDTH, PLOT_X_AXIS_HEIGHT);
    for(uint32_t i = 0; i < x_tick_count; ++i) {
        uint32_t point_index = (i * (RADIO_GUI_WAVEFORM_POINT_COUNT - 1U) +
                                (x_tick_count - 1U) / 2U) /
                               (x_tick_count - 1U);
        x_labels[i] = create_axis_label(x_axis, "");
        lv_label_set_text_fmt(x_labels[i], "%lu", (unsigned long)point_index);
        lv_obj_set_width(x_labels[i], PLOT_X_LABEL_WIDTH);

        int32_t tick_x = (int32_t)(((PLOT_CHART_WIDTH - 1) * i) / (x_tick_count - 1U));
        int32_t label_x = tick_x - PLOT_X_LABEL_WIDTH / 2;
        if(label_x < 0) label_x = 0;
        if(i == 0U) label_x = 2;
        if(label_x > PLOT_CHART_WIDTH - PLOT_X_LABEL_WIDTH) {
            label_x = PLOT_CHART_WIDTH - PLOT_X_LABEL_WIDTH;
        }
        lv_obj_set_pos(x_labels[i], label_x, 0);
        lv_obj_set_style_text_align(x_labels[i],
                                    i == 0U ? LV_TEXT_ALIGN_LEFT :
                                    (i + 1U == x_tick_count ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_CENTER),
                                    0);
    }

    *chart_out = chart;
    *series_out = series;
}

static void format_time_tick(char * text,
                             size_t text_size,
                             uint32_t sample_index,
                             uint32_t sample_rate_hz)
{
    if(sample_rate_hz == 0U) {
        (void)snprintf(text, text_size, "%lu", (unsigned long)sample_index);
        return;
    }

    uint64_t microseconds = ((uint64_t)sample_index * 1000000ULL) / sample_rate_hz;
    if(microseconds < 1000ULL) {
        (void)snprintf(text, text_size, "%lu us", (unsigned long)microseconds);
        return;
    }

    uint64_t tenths_ms = (microseconds + 50ULL) / 100ULL;
    if((tenths_ms % 10ULL) == 0ULL) {
        (void)snprintf(text, text_size, "%lu ms", (unsigned long)(tenths_ms / 10ULL));
    }
    else {
        (void)snprintf(text,
                       text_size,
                       "%lu.%lu ms",
                       (unsigned long)(tenths_ms / 10ULL),
                       (unsigned long)(tenths_ms % 10ULL));
    }
}

static void format_frequency_tick(char * text, size_t text_size, uint32_t frequency_hz, bool show_unit)
{
    if(frequency_hz >= 1000000U) {
        uint32_t tenths_mhz = (frequency_hz + 50000U) / 100000U;
        (void)snprintf(text,
                       text_size,
                       show_unit ? "%lu.%lu MHz" : "%lu.%luM",
                       (unsigned long)(tenths_mhz / 10U),
                       (unsigned long)(tenths_mhz % 10U));
    }
    else if(frequency_hz >= 1000U) {
        uint32_t tenths_khz = (frequency_hz + 50U) / 100U;
        if((tenths_khz % 10U) == 0U) {
            (void)snprintf(text,
                           text_size,
                           show_unit ? "%lu kHz" : "%luk",
                           (unsigned long)(tenths_khz / 10U));
        }
        else {
            (void)snprintf(text,
                           text_size,
                           show_unit ? "%lu.%lu kHz" : "%lu.%luk",
                           (unsigned long)(tenths_khz / 10U),
                           (unsigned long)(tenths_khz % 10U));
        }
    }
    else {
        (void)snprintf(text,
                       text_size,
                       show_unit ? "%lu Hz" : "%lu",
                       (unsigned long)frequency_hz);
    }
}

void radio_gui_build_waveform_page(radio_gui_t * gui)
{
    lv_obj_t * content = radio_gui_create_page_content(gui->waveform_tile);
    lv_obj_set_style_pad_left(content, 8, 0);
    lv_obj_set_style_pad_right(content, 8, 0);
    lv_obj_set_style_pad_top(content, 3, 0);
    lv_obj_set_style_pad_bottom(content, 3, 0);
    lv_obj_set_style_pad_row(content, 2, 0);

    create_waveform_header(content);

    lv_obj_t * time_y_labels[3];
    create_plot_card(content,
                     "Time domain",
                     "int8  |  256 points",
                     RADIO_GUI_COLOR_CYAN,
                     "+127",
                     "0",
                     "-128",
                     &gui->time_chart,
                     &gui->time_series,
                     gui->time_x_labels,
                     TIME_X_TICK_COUNT,
                     time_y_labels);
    lv_chart_set_axis_range(gui->time_chart, LV_CHART_AXIS_PRIMARY_Y, -128, 127);
    lv_chart_set_all_values(gui->time_chart, gui->time_series, 0);

    create_plot_card(content,
                     "Spectrum",
                     "dB  |  256 points",
                     RADIO_GUI_COLOR_AMBER,
                     "0",
                     "-60",
                     "-120",
                     &gui->spectrum_chart,
                     &gui->spectrum_series,
                     gui->spectrum_x_labels,
                     SPECTRUM_X_TICK_COUNT,
                     gui->spectrum_y_labels);
    lv_chart_set_axis_range(gui->spectrum_chart, LV_CHART_AXIS_PRIMARY_Y, -120, 0);
    lv_chart_set_all_values(gui->spectrum_chart, gui->spectrum_series, -120);
}

void radio_gui_set_time_domain_data(radio_gui_t * gui,
                                    const int8_t samples[RADIO_GUI_WAVEFORM_POINT_COUNT],
                                    uint32_t sample_rate_hz)
{
    if(gui == NULL || gui->time_chart == NULL || gui->time_series == NULL || samples == NULL) return;

    int32_t chart_values[RADIO_GUI_WAVEFORM_POINT_COUNT];
    for(uint32_t i = 0; i < RADIO_GUI_WAVEFORM_POINT_COUNT; ++i) {
        chart_values[i] = samples[i] << 1;// scale for LVGL chart
    }
    lv_chart_set_series_values(gui->time_chart,
                               gui->time_series,
                               chart_values,
                               RADIO_GUI_WAVEFORM_POINT_COUNT);

    char middle_text[20];
    char end_text[20];
    format_time_tick(middle_text, sizeof(middle_text), RADIO_GUI_WAVEFORM_POINT_COUNT / 2U, sample_rate_hz);
    format_time_tick(end_text, sizeof(end_text), RADIO_GUI_WAVEFORM_POINT_COUNT - 1U, sample_rate_hz);
    lv_label_set_text(gui->time_x_labels[0], "0");
    lv_label_set_text(gui->time_x_labels[1], middle_text);
    lv_label_set_text(gui->time_x_labels[2], end_text);
    lv_chart_refresh(gui->time_chart);
}

void radio_gui_set_spectrum_data(radio_gui_t * gui,
                                 const int8_t log_magnitude_db[RADIO_GUI_WAVEFORM_POINT_COUNT],
                                 uint32_t max_frequency_hz,
                                 int8_t min_db,
                                 int8_t max_db)
{
    if(gui == NULL || gui->spectrum_chart == NULL || gui->spectrum_series == NULL ||
       log_magnitude_db == NULL) {
        return;
    }
    if(min_db >= max_db) {
        min_db = -120;
        max_db = 0;
    }

    int32_t chart_values[RADIO_GUI_WAVEFORM_POINT_COUNT];
    for(uint32_t i = 0; i < RADIO_GUI_WAVEFORM_POINT_COUNT; ++i) {
        chart_values[i] = log_magnitude_db[i];
    }
    lv_chart_set_axis_range(gui->spectrum_chart, LV_CHART_AXIS_PRIMARY_Y, min_db, max_db);
    lv_chart_set_series_values(gui->spectrum_chart,
                               gui->spectrum_series,
                               chart_values,
                               RADIO_GUI_WAVEFORM_POINT_COUNT);

    int32_t middle_db = (int32_t)min_db + ((int32_t)max_db - (int32_t)min_db) / 2;
    char db_text[3][12];
    (void)snprintf(db_text[0], sizeof(db_text[0]), "%ddB", (int)max_db);
    (void)snprintf(db_text[1], sizeof(db_text[1]), "%d", (int)middle_db);
    (void)snprintf(db_text[2], sizeof(db_text[2]), "%d", (int)min_db);
    for(uint32_t i = 0; i < 3U; ++i) {
        lv_label_set_text(gui->spectrum_y_labels[i], db_text[i]);
    }

    char tick_text[SPECTRUM_X_TICK_COUNT][20];
    for(uint32_t i = 0; i < SPECTRUM_X_TICK_COUNT; ++i) {
        if(max_frequency_hz == 0U) {
            uint32_t bin = (i * (RADIO_GUI_WAVEFORM_POINT_COUNT - 1U) +
                            (SPECTRUM_X_TICK_COUNT - 1U) / 2U) /
                           (SPECTRUM_X_TICK_COUNT - 1U);
            (void)snprintf(tick_text[i],
                           sizeof(tick_text[i]),
                           i + 1U == SPECTRUM_X_TICK_COUNT ? "%lu bin" : "%lu",
                           (unsigned long)bin);
        }
        else {
            uint32_t frequency_hz = (uint32_t)(((uint64_t)max_frequency_hz * i) /
                                               (SPECTRUM_X_TICK_COUNT - 1U));
            format_frequency_tick(tick_text[i],
                                  sizeof(tick_text[i]),
                                  frequency_hz,
                                  i + 1U == SPECTRUM_X_TICK_COUNT);
        }
        lv_label_set_text(gui->spectrum_x_labels[i], tick_text[i]);
    }
    lv_chart_refresh(gui->spectrum_chart);
}
