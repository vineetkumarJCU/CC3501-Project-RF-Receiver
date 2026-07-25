#include "radio_gui_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void keypad_number_event_cb(lv_event_t * event);
static void keypad_unit_event_cb(lv_event_t * event);
static void result_ok_event_cb(lv_event_t * event);
static void close_overlay_event_cb(lv_event_t * event);
static void compact_table_draw_event_cb(lv_event_t * event);
static void band_confirm_yes_event_cb(lv_event_t * event);
static void band_confirm_no_event_cb(lv_event_t * event);

static lv_obj_t * create_overlay(lv_obj_t * parent)
{
    lv_obj_t * overlay = lv_obj_create(parent);
    lv_obj_remove_style_all(overlay);
    lv_obj_set_size(overlay, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(overlay, lv_color_hex(0x05080C), 0);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_90, 0);
    lv_obj_set_style_pad_all(overlay, 10, 0);
    lv_obj_add_flag(overlay, LV_OBJ_FLAG_HIDDEN);
    return overlay;
}

static lv_obj_t * create_modal_card(lv_obj_t * parent, int32_t height)
{
    lv_obj_t * card = lv_obj_create(parent);
    lv_obj_set_size(card, LV_PCT(100), height);
    lv_obj_center(card);
    lv_obj_set_style_bg_color(card, RADIO_GUI_COLOR_CARD, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(card, RADIO_GUI_COLOR_BORDER, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_radius(card, 14, 0);
    lv_obj_set_style_pad_all(card, 12, 0);
    lv_obj_set_style_pad_row(card, 8, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    return card;
}

static lv_obj_t * create_modal_title(lv_obj_t * parent, const char * text, lv_color_t color)
{
    lv_obj_t * label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, color, 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_letter_space(label, 1, 0);
    return label;
}

radio_gui_t * radio_gui_create(lv_obj_t * parent, const radio_gui_config_t * config)
{
    if(parent == NULL) return NULL;

    radio_gui_t * gui = lv_malloc_zeroed(sizeof(*gui));
    if(gui == NULL) return NULL;

    if(config != NULL) gui->config = *config;
    gui->band = RADIO_GUI_BAND_FM;
    gui->frequency_unit = RADIO_GUI_FREQ_MHZ;
    gui->battery_voltage = 3.97F;
    gui->sd_status = RADIO_GUI_SD_NO_CARD;
    gui->gps_status = RADIO_GUI_GPS_UNLOCKED;

    gui->root = lv_obj_create(parent);
    lv_obj_remove_style_all(gui->root);
    lv_obj_set_size(gui->root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(gui->root, RADIO_GUI_COLOR_BG, 0);
    lv_obj_set_style_bg_opa(gui->root, LV_OPA_COVER, 0);
    lv_obj_remove_flag(gui->root, LV_OBJ_FLAG_SCROLLABLE);

    gui->tileview = lv_tileview_create(gui->root);
    lv_obj_set_size(gui->tileview, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(gui->tileview, RADIO_GUI_COLOR_BG, 0);
    lv_obj_set_style_bg_opa(gui->tileview, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(gui->tileview, 0, 0);
    lv_obj_set_style_pad_all(gui->tileview, 0, 0);
    lv_obj_set_scrollbar_mode(gui->tileview, LV_SCROLLBAR_MODE_OFF);

    gui->hardware_tile = lv_tileview_add_tile(gui->tileview, 0, 0, LV_DIR_RIGHT);
    gui->radio_tile = lv_tileview_add_tile(gui->tileview, 1, 0, (lv_dir_t)(LV_DIR_LEFT | LV_DIR_RIGHT));
    gui->waveform_tile = lv_tileview_add_tile(gui->tileview, 2, 0, (lv_dir_t)(LV_DIR_LEFT | LV_DIR_RIGHT));
    gui->gps_tile = lv_tileview_add_tile(gui->tileview, 3, 0, LV_DIR_LEFT);

    radio_gui_build_hardware_page(gui);
    radio_gui_build_radio_page(gui);
    radio_gui_build_waveform_page(gui);
    radio_gui_build_gps_page(gui);
    radio_gui_build_overlays(gui);
    radio_gui_show_radio_page(gui, LV_ANIM_OFF);

    return gui;
}

void radio_gui_destroy(radio_gui_t * gui)
{
    if(gui == NULL) return;
    if(gui->keypad_overlay != NULL) lv_obj_delete(gui->keypad_overlay);
    if(gui->result_overlay != NULL) lv_obj_delete(gui->result_overlay);
    if(gui->band_confirm_overlay != NULL) lv_obj_delete(gui->band_confirm_overlay);
    if(gui->root != NULL) lv_obj_delete(gui->root);
    lv_free(gui);
}

void radio_gui_show_radio_page(radio_gui_t * gui, lv_anim_enable_t animated)
{
    radio_gui_show_page(gui, RADIO_GUI_PAGE_RADIO, animated);
}

void radio_gui_show_page(radio_gui_t * gui, radio_gui_page_t page, lv_anim_enable_t animated)
{
    if(gui == NULL) return;
    lv_obj_t * tile = gui->radio_tile;
    if(page == RADIO_GUI_PAGE_HARDWARE) tile = gui->hardware_tile;
    else if(page == RADIO_GUI_PAGE_WAVEFORM) tile = gui->waveform_tile;
    else if(page == RADIO_GUI_PAGE_GPS) tile = gui->gps_tile;
    lv_tileview_set_tile(gui->tileview, tile, animated);
}

radio_gui_page_t radio_gui_get_current_page(const radio_gui_t * gui)
{
    if(gui == NULL || gui->tileview == NULL) return RADIO_GUI_PAGE_RADIO;
    lv_obj_t * active_tile = lv_tileview_get_tile_active(gui->tileview);
    if(active_tile == gui->hardware_tile) return RADIO_GUI_PAGE_HARDWARE;
    if(active_tile == gui->waveform_tile) return RADIO_GUI_PAGE_WAVEFORM;
    if(active_tile == gui->gps_tile) return RADIO_GUI_PAGE_GPS;
    return RADIO_GUI_PAGE_RADIO;
}

radio_gui_frequency_unit_t radio_gui_get_frequency_unit(const radio_gui_t * gui)
{
    return gui != NULL ? gui->frequency_unit : RADIO_GUI_FREQ_KHZ;
}

lv_obj_t * radio_gui_create_page_content(lv_obj_t * tile)
{
    lv_obj_set_style_bg_color(tile, RADIO_GUI_COLOR_BG, 0);
    lv_obj_set_style_bg_opa(tile, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(tile, 0, 0);
    lv_obj_set_scrollbar_mode(tile, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t * content = lv_obj_create(tile);
    lv_obj_remove_style_all(content);
    lv_obj_set_width(content, LV_PCT(100));
    lv_obj_set_height(content, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(content, RADIO_GUI_COLOR_BG, 0);
    lv_obj_set_style_bg_opa(content, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_left(content, 12, 0);
    lv_obj_set_style_pad_right(content, 12, 0);
    lv_obj_set_style_pad_top(content, 9, 0);
    lv_obj_set_style_pad_bottom(content, 14, 0);
    lv_obj_set_style_pad_row(content, 9, 0);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_scroll_dir(content, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(content, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_style_bg_color(content, RADIO_GUI_COLOR_BORDER, LV_PART_SCROLLBAR);
    lv_obj_set_style_bg_opa(content, LV_OPA_50, LV_PART_SCROLLBAR);
    lv_obj_set_style_width(content, 3, LV_PART_SCROLLBAR);
    return content;
}

lv_obj_t * radio_gui_create_header(lv_obj_t * parent,
                                   const char * eyebrow,
                                   const char * title,
                                   const char * page_number,
                                   lv_color_t accent)
{
    lv_obj_t * header = lv_obj_create(parent);
    lv_obj_remove_style_all(header);
    lv_obj_set_size(header, LV_PCT(100), 39);
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t * title_group = lv_obj_create(header);
    lv_obj_remove_style_all(title_group);
    lv_obj_set_size(title_group, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(title_group, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(title_group, 1, 0);

    lv_obj_t * eyebrow_label = lv_label_create(title_group);
    lv_label_set_text(eyebrow_label, eyebrow);
    lv_obj_set_style_text_color(eyebrow_label, accent, 0);
    lv_obj_set_style_text_font(eyebrow_label, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_letter_space(eyebrow_label, 2, 0);

    lv_obj_t * title_label = lv_label_create(title_group);
    lv_label_set_text(title_label, title);
    lv_obj_set_style_text_color(title_label, RADIO_GUI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_18, 0);

    lv_obj_t * page_pill = radio_gui_create_value_pill(header, page_number, accent);
    lv_obj_set_style_text_font(page_pill, &lv_font_montserrat_10, 0);
    return header;
}

lv_obj_t * radio_gui_create_card(lv_obj_t * parent)
{
    lv_obj_t * card = lv_obj_create(parent);
    lv_obj_set_width(card, LV_PCT(100));
    lv_obj_set_height(card, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(card, RADIO_GUI_COLOR_CARD, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(card, RADIO_GUI_COLOR_BORDER, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_pad_all(card, 11, 0);
    lv_obj_set_style_pad_row(card, 8, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    return card;
}

lv_obj_t * radio_gui_create_section_label(lv_obj_t * parent, const char * text)
{
    lv_obj_t * label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, RADIO_GUI_COLOR_MUTED, 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_letter_space(label, 1, 0);
    return label;
}

lv_obj_t * radio_gui_create_value_pill(lv_obj_t * parent, const char * text, lv_color_t color)
{
    lv_obj_t * label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, color, 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_12, 0);
    lv_obj_set_style_bg_color(label, color, 0);
    lv_obj_set_style_bg_opa(label, LV_OPA_20, 0);
    lv_obj_set_style_border_color(label, color, 0);
    lv_obj_set_style_border_opa(label, LV_OPA_40, 0);
    lv_obj_set_style_border_width(label, 1, 0);
    lv_obj_set_style_radius(label, 20, 0);
    lv_obj_set_style_pad_hor(label, 8, 0);
    lv_obj_set_style_pad_ver(label, 4, 0);
    return label;
}

lv_obj_t * radio_gui_create_compact_table(lv_obj_t * parent,
                                          const char * const * names,
                                          uint32_t row_count)
{
    lv_obj_t * table = lv_table_create(parent);
    lv_obj_set_width(table, LV_PCT(100));
    lv_obj_set_height(table, LV_SIZE_CONTENT);
    lv_table_set_column_count(table, 2);
    lv_table_set_row_count(table, row_count);
    lv_table_set_column_width(table, 0, 154);
    lv_table_set_column_width(table, 1, 118);
    lv_obj_set_style_bg_opa(table, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(table, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(table, RADIO_GUI_COLOR_CARD_ALT, LV_PART_ITEMS);
    lv_obj_set_style_bg_opa(table, LV_OPA_COVER, LV_PART_ITEMS);
    lv_obj_set_style_border_color(table, RADIO_GUI_COLOR_BORDER, LV_PART_ITEMS);
    lv_obj_set_style_border_width(table, 1, LV_PART_ITEMS);
    lv_obj_set_style_text_color(table, RADIO_GUI_COLOR_TEXT, LV_PART_ITEMS);
    lv_obj_set_style_text_font(table, &lv_font_montserrat_14, LV_PART_ITEMS);
    lv_obj_set_style_pad_left(table, 6, LV_PART_ITEMS);
    lv_obj_set_style_pad_right(table, 6, LV_PART_ITEMS);
    lv_obj_set_style_pad_top(table, 8, LV_PART_ITEMS);
    lv_obj_set_style_pad_bottom(table, 8, LV_PART_ITEMS);

    lv_obj_add_event_cb(table, compact_table_draw_event_cb, LV_EVENT_DRAW_TASK_ADDED, NULL);
    lv_obj_add_flag(table, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);

    for(uint32_t row = 0; row < row_count; ++row) {
        lv_table_set_cell_value(table, row, 0, names[row]);
        lv_table_set_cell_value(table, row, 1, "--");
    }
    return table;
}

static void compact_table_draw_event_cb(lv_event_t * event)
{
    lv_draw_task_t * draw_task = lv_event_get_draw_task(event);
    lv_draw_dsc_base_t * base_dsc = (lv_draw_dsc_base_t *)lv_draw_task_get_draw_dsc(draw_task);
    if(base_dsc == NULL || base_dsc->part != LV_PART_ITEMS) return;

    lv_color_t text_color = base_dsc->id2 == 1U ? RADIO_GUI_COLOR_CYAN : RADIO_GUI_COLOR_TEXT;
    lv_draw_label_dsc_t * label_dsc = lv_draw_task_get_label_dsc(draw_task);
    if(label_dsc != NULL) {
        label_dsc->color = text_color;
        label_dsc->outline_stroke_color = text_color;
        label_dsc->outline_stroke_width = 1;
        label_dsc->outline_stroke_opa = LV_OPA_60;
        if(base_dsc->id2 == 1U) label_dsc->align = LV_TEXT_ALIGN_RIGHT;
    }

    if((base_dsc->id1 & 1U) != 0U) {
        lv_draw_fill_dsc_t * fill_dsc = lv_draw_task_get_fill_dsc(draw_task);
        if(fill_dsc != NULL) {
            fill_dsc->color = lv_color_hex(0x1D2A36);
            fill_dsc->opa = LV_OPA_COVER;
        }
    }
}

void radio_gui_style_slider(lv_obj_t * slider, lv_color_t accent)
{
    lv_obj_set_height(slider, 12);
    lv_obj_set_style_bg_color(slider, RADIO_GUI_COLOR_CARD_ALT, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(slider, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(slider, 8, LV_PART_MAIN);
    lv_obj_set_style_bg_color(slider, accent, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(slider, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_radius(slider, 8, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider, RADIO_GUI_COLOR_TEXT, LV_PART_KNOB);
    lv_obj_set_style_border_color(slider, accent, LV_PART_KNOB);
    lv_obj_set_style_border_width(slider, 3, LV_PART_KNOB);
    lv_obj_set_style_pad_all(slider, 2, LV_PART_KNOB);
}

void radio_gui_style_switch(lv_obj_t * sw, lv_color_t accent)
{
    lv_obj_set_size(sw, 42, 22);
    lv_obj_set_style_bg_color(sw, RADIO_GUI_COLOR_CARD_ALT, LV_PART_MAIN);
    lv_obj_set_style_bg_color(sw, accent, LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(sw, RADIO_GUI_COLOR_TEXT, LV_PART_KNOB);
    lv_obj_set_style_pad_all(sw, 2, LV_PART_KNOB);
}

void radio_gui_set_status_pill(lv_obj_t * label, const char * text, lv_color_t color)
{
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, color, 0);
    lv_obj_set_style_bg_color(label, color, 0);
    lv_obj_set_style_border_color(label, color, 0);
}

void radio_gui_build_overlays(radio_gui_t * gui)
{
    static const char * const keypad_map[] = {
        "1", "2", "3", "\n",
        "4", "5", "6", "\n",
        "7", "8", "9", "\n",
        ".", "0", LV_SYMBOL_BACKSPACE, ""
    };
    static const char * const unit_map[] = {"kHz", "MHz", "OK", ""};

    gui->keypad_overlay = create_overlay(lv_layer_top());
    lv_obj_set_style_pad_all(gui->keypad_overlay, 4, 0);
    lv_obj_add_event_cb(gui->keypad_overlay, close_overlay_event_cb, LV_EVENT_CLICKED, gui->keypad_overlay);

    lv_obj_t * card = create_modal_card(gui->keypad_overlay, 232);
    lv_obj_set_style_pad_all(card, 5, 0);
    lv_obj_set_style_pad_row(card, 3, 0);
    lv_obj_add_flag(card, LV_OBJ_FLAG_EVENT_BUBBLE);

    lv_obj_t * heading = lv_obj_create(card);
    lv_obj_remove_style_all(heading);
    lv_obj_set_size(heading, LV_PCT(100), 12);
    lv_obj_set_flex_flow(heading, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(heading, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    create_modal_title(heading, "SET RX FREQUENCY", RADIO_GUI_COLOR_CYAN);

    gui->keypad_textarea = lv_textarea_create(card);
    lv_obj_set_size(gui->keypad_textarea, LV_PCT(100), 27);
    lv_textarea_set_one_line(gui->keypad_textarea, true);
    lv_textarea_set_accepted_chars(gui->keypad_textarea, "0123456789.");
    lv_textarea_set_max_length(gui->keypad_textarea, 12);
    lv_textarea_set_placeholder_text(gui->keypad_textarea, "Enter frequency");
    lv_obj_set_style_bg_color(gui->keypad_textarea, RADIO_GUI_COLOR_CARD_ALT, 0);
    lv_obj_set_style_border_color(gui->keypad_textarea, RADIO_GUI_COLOR_CYAN, LV_STATE_FOCUSED);
    lv_obj_set_style_border_width(gui->keypad_textarea, 1, 0);
    lv_obj_set_style_radius(gui->keypad_textarea, 6, 0);
    lv_obj_set_style_text_color(gui->keypad_textarea, RADIO_GUI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(gui->keypad_textarea, &lv_font_montserrat_16, 0);
    lv_obj_set_style_pad_all(gui->keypad_textarea, 4, 0);

    lv_obj_t * keypad = lv_buttonmatrix_create(card);
    lv_buttonmatrix_set_map(keypad, keypad_map);
    lv_obj_set_size(keypad, LV_PCT(100), 145);
    lv_obj_set_style_bg_opa(keypad, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(keypad, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(keypad, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_row(keypad, 3, LV_PART_MAIN);
    lv_obj_set_style_pad_column(keypad, 3, LV_PART_MAIN);
    lv_obj_set_style_bg_color(keypad, RADIO_GUI_COLOR_CARD_ALT, LV_PART_ITEMS);
    lv_obj_set_style_bg_color(keypad, RADIO_GUI_COLOR_CYAN, LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_set_style_text_color(keypad, RADIO_GUI_COLOR_TEXT, LV_PART_ITEMS);
    lv_obj_set_style_text_font(keypad, &lv_font_montserrat_18, LV_PART_ITEMS);
    lv_obj_set_style_border_width(keypad, 0, LV_PART_ITEMS);
    lv_obj_set_style_radius(keypad, 7, LV_PART_ITEMS);
    lv_obj_add_event_cb(keypad, keypad_number_event_cb, LV_EVENT_VALUE_CHANGED, gui);

    gui->keypad_units = lv_buttonmatrix_create(card);
    lv_buttonmatrix_set_map(gui->keypad_units, unit_map);
    lv_obj_set_size(gui->keypad_units, LV_PCT(100), 29);
    lv_obj_set_style_bg_opa(gui->keypad_units, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(gui->keypad_units, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(gui->keypad_units, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(gui->keypad_units, 5, LV_PART_MAIN);
    lv_obj_set_style_bg_color(gui->keypad_units, RADIO_GUI_COLOR_CARD_ALT, LV_PART_ITEMS);
    lv_obj_set_style_bg_color(gui->keypad_units, RADIO_GUI_COLOR_BLUE, LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(gui->keypad_units, RADIO_GUI_COLOR_CYAN, LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_set_style_text_color(gui->keypad_units, RADIO_GUI_COLOR_TEXT, LV_PART_ITEMS);
    lv_obj_set_style_text_font(gui->keypad_units, &lv_font_montserrat_12, LV_PART_ITEMS);
    lv_obj_set_style_border_width(gui->keypad_units, 0, LV_PART_ITEMS);
    lv_obj_set_style_radius(gui->keypad_units, 12, LV_PART_ITEMS);
    lv_buttonmatrix_set_one_checked(gui->keypad_units, true);
    lv_buttonmatrix_set_button_ctrl(gui->keypad_units, 0, LV_BUTTONMATRIX_CTRL_CHECKABLE);
    lv_buttonmatrix_set_button_ctrl(gui->keypad_units, 1, LV_BUTTONMATRIX_CTRL_CHECKABLE);
    lv_buttonmatrix_set_button_ctrl(gui->keypad_units, 1, LV_BUTTONMATRIX_CTRL_CHECKED);
    lv_buttonmatrix_set_button_width(gui->keypad_units, 2, 2);
    lv_obj_add_event_cb(gui->keypad_units, keypad_unit_event_cb, LV_EVENT_VALUE_CHANGED, gui);

    gui->result_overlay = create_overlay(lv_layer_top());
    lv_obj_set_style_pad_all(gui->result_overlay, 4, 0);
    lv_obj_t * result_card = create_modal_card(gui->result_overlay, 232);
    lv_obj_set_style_pad_all(result_card, 10, 0);
    lv_obj_set_style_pad_row(result_card, 5, 0);
    gui->result_title = create_modal_title(result_card, "TUNE RESULT", RADIO_GUI_COLOR_GREEN);
    gui->result_text = lv_label_create(result_card);
    lv_label_set_text(gui->result_text, "");
    lv_label_set_long_mode(gui->result_text, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(gui->result_text, LV_PCT(100));
    lv_obj_set_flex_grow(gui->result_text, 1);
    lv_obj_set_style_text_color(gui->result_text, RADIO_GUI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(gui->result_text, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_line_space(gui->result_text, 1, 0);

    lv_obj_t * ok = lv_button_create(result_card);
    lv_obj_set_size(ok, LV_PCT(100), 32);
    lv_obj_set_style_bg_color(ok, RADIO_GUI_COLOR_CYAN, 0);
    lv_obj_set_style_radius(ok, 8, 0);
    lv_obj_add_event_cb(ok, result_ok_event_cb, LV_EVENT_CLICKED, gui);
    lv_obj_t * ok_label = lv_label_create(ok);
    lv_label_set_text(ok_label, "OK");
    lv_obj_set_style_text_color(ok_label, RADIO_GUI_COLOR_BG, 0);
    lv_obj_set_style_text_font(ok_label, &lv_font_montserrat_12, 0);
    lv_obj_center(ok_label);

    gui->band_confirm_overlay = create_overlay(lv_layer_top());
    lv_obj_set_style_pad_all(gui->band_confirm_overlay, 12, 0);
    lv_obj_t * confirm_card = create_modal_card(gui->band_confirm_overlay, 142);
    lv_obj_set_style_pad_all(confirm_card, 12, 0);
    lv_obj_set_style_pad_row(confirm_card, 8, 0);
    lv_obj_set_style_border_color(confirm_card, RADIO_GUI_COLOR_AMBER, 0);
    lv_obj_set_style_border_opa(confirm_card, LV_OPA_60, 0);

    create_modal_title(confirm_card, "CONFIRM BAND CHANGE", RADIO_GUI_COLOR_AMBER);

    gui->band_confirm_target = lv_label_create(confirm_card);
    lv_label_set_text(gui->band_confirm_target, "FM  ->  SW / AM / LW");
    lv_obj_set_width(gui->band_confirm_target, LV_PCT(100));
    lv_obj_set_style_text_align(gui->band_confirm_target, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(gui->band_confirm_target, RADIO_GUI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(gui->band_confirm_target, &lv_font_montserrat_16, 0);

    lv_obj_t * warning = lv_label_create(confirm_card);
    lv_label_set_text(warning, "That will restart SI4732");
    lv_obj_set_width(warning, LV_PCT(100));
    lv_obj_set_style_text_align(warning, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(warning, RADIO_GUI_COLOR_AMBER, 0);
    lv_obj_set_style_text_font(warning, &lv_font_montserrat_12, 0);

    lv_obj_t * actions = lv_obj_create(confirm_card);
    lv_obj_remove_style_all(actions);
    lv_obj_set_size(actions, LV_PCT(100), 34);
    lv_obj_set_flex_flow(actions, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(actions,
                          LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(actions, 8, 0);

    lv_obj_t * no_button = lv_button_create(actions);
    lv_obj_set_size(no_button, 125, 34);
    lv_obj_set_style_bg_color(no_button, RADIO_GUI_COLOR_CARD_ALT, 0);
    lv_obj_set_style_border_color(no_button, RADIO_GUI_COLOR_BORDER, 0);
    lv_obj_set_style_border_width(no_button, 1, 0);
    lv_obj_set_style_radius(no_button, 8, 0);
    lv_obj_add_event_cb(no_button, band_confirm_no_event_cb, LV_EVENT_CLICKED, gui);
    lv_obj_t * no_label = lv_label_create(no_button);
    lv_label_set_text(no_label, "NO");
    lv_obj_set_style_text_color(no_label, RADIO_GUI_COLOR_TEXT, 0);
    lv_obj_set_style_text_font(no_label, &lv_font_montserrat_12, 0);
    lv_obj_center(no_label);

    lv_obj_t * yes_button = lv_button_create(actions);
    lv_obj_set_size(yes_button, 125, 34);
    lv_obj_set_style_bg_color(yes_button, RADIO_GUI_COLOR_CYAN, 0);
    lv_obj_set_style_radius(yes_button, 8, 0);
    lv_obj_add_event_cb(yes_button, band_confirm_yes_event_cb, LV_EVENT_CLICKED, gui);
    lv_obj_t * yes_label = lv_label_create(yes_button);
    lv_label_set_text(yes_label, "YES");
    lv_obj_set_style_text_color(yes_label, RADIO_GUI_COLOR_BG, 0);
    lv_obj_set_style_text_font(yes_label, &lv_font_montserrat_12, 0);
    lv_obj_center(yes_label);
}

void radio_gui_open_keypad(radio_gui_t * gui)
{
    if(gui == NULL) return;
    lv_textarea_set_text(gui->keypad_textarea, "");
    lv_obj_remove_flag(gui->keypad_overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(gui->keypad_overlay);
}

void radio_gui_show_frequency_keypad(radio_gui_t * gui)
{
    radio_gui_open_keypad(gui);
}

void radio_gui_show_tune_result(radio_gui_t * gui, bool accepted, const char * result_text)
{
    if(gui == NULL) return;
    lv_label_set_text(gui->result_title, accepted ? "TUNE ACCEPTED" : "CHECK INPUT");
    lv_obj_set_style_text_color(gui->result_title,
                                accepted ? RADIO_GUI_COLOR_GREEN : RADIO_GUI_COLOR_RED,
                                0);
    lv_label_set_text(gui->result_text, result_text != NULL ? result_text : "");
    lv_obj_add_flag(gui->keypad_overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(gui->result_overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(gui->result_overlay);
}

void radio_gui_open_band_confirmation(radio_gui_t * gui, radio_gui_band_t band)
{
    if(gui == NULL || gui->band_confirm_overlay == NULL || band == gui->band) return;

    const char * current_name = gui->band == RADIO_GUI_BAND_FM ? "FM" : "SW / AM / LW";
    const char * target_name = band == RADIO_GUI_BAND_FM ? "FM" : "SW / AM / LW";
    gui->pending_band = band;
    lv_label_set_text_fmt(gui->band_confirm_target, "%s  ->  %s", current_name, target_name);
    lv_obj_remove_flag(gui->band_confirm_overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(gui->band_confirm_overlay);
}

static void keypad_number_event_cb(lv_event_t * event)
{
    radio_gui_t * gui = lv_event_get_user_data(event);
    lv_obj_t * button_matrix = lv_event_get_target_obj(event);
    uint32_t button = lv_buttonmatrix_get_selected_button(button_matrix);
    const char * text = lv_buttonmatrix_get_button_text(button_matrix, button);
    if(text == NULL) return;

    if(strcmp(text, LV_SYMBOL_BACKSPACE) == 0) {
        lv_textarea_delete_char(gui->keypad_textarea);
    }
    else {
        const char * current = lv_textarea_get_text(gui->keypad_textarea);
        if(strcmp(text, ".") != 0 || strchr(current, '.') == NULL) {
            lv_textarea_add_text(gui->keypad_textarea, text);
        }
    }
}

static void keypad_unit_event_cb(lv_event_t * event)
{
    radio_gui_t * gui = lv_event_get_user_data(event);
    lv_obj_t * button_matrix = lv_event_get_target_obj(event);
    uint32_t button = lv_buttonmatrix_get_selected_button(button_matrix);
    const char * text = lv_buttonmatrix_get_button_text(button_matrix, button);
    if(text == NULL) return;

    if(strcmp(text, "kHz") == 0) {
        gui->frequency_unit = RADIO_GUI_FREQ_KHZ;
        return;
    }
    if(strcmp(text, "MHz") == 0) {
        gui->frequency_unit = RADIO_GUI_FREQ_MHZ;
        return;
    }
    if(strcmp(text, "OK") != 0) return;

    const char * input = lv_textarea_get_text(gui->keypad_textarea);
    char * end = NULL;
    double value = strtod(input, &end);
    bool valid = input[0] != '\0' && end != input && *end == '\0' && value > 0.0;
    char result[160] = "";

    radio_gui_frequency_request_t request = {
        .value = value,
        .unit = gui->frequency_unit,
        .band = gui->band
    };

    if(valid && gui->config.callbacks.frequency_submitted != NULL) {
        valid = gui->config.callbacks.frequency_submitted(gui->config.callback_context,
                                                           &request,
                                                           result,
                                                           sizeof(result));
    }
    else if(!valid) {
        (void)snprintf(result, sizeof(result), "Enter a positive numeric frequency.");
    }

    if(valid) {
        char display[48];
        (void)snprintf(display,
                       sizeof(display),
                       "%s %s",
                       input,
                       gui->frequency_unit == RADIO_GUI_FREQ_MHZ ? "MHz" : "kHz");
        radio_gui_set_frequency_text(gui, display);
    }
    radio_gui_show_tune_result(gui, valid, result);
}

static void result_ok_event_cb(lv_event_t * event)
{
    radio_gui_t * gui = lv_event_get_user_data(event);
    lv_label_set_text(gui->result_text, "");
    lv_obj_add_flag(gui->result_overlay, LV_OBJ_FLAG_HIDDEN);
}

static void band_confirm_yes_event_cb(lv_event_t * event)
{
    radio_gui_t * gui = lv_event_get_user_data(event);
    lv_obj_add_flag(gui->band_confirm_overlay, LV_OBJ_FLAG_HIDDEN);
    if(gui->pending_band == gui->band) return;

    radio_gui_set_band(gui, gui->pending_band);
    if(gui->config.callbacks.band_changed != NULL) {
        gui->config.callbacks.band_changed(gui->config.callback_context, gui->band);
    }
}

static void band_confirm_no_event_cb(lv_event_t * event)
{
    radio_gui_t * gui = lv_event_get_user_data(event);
    gui->pending_band = gui->band;
    radio_gui_sync_band_button_state(gui, gui->band);
    lv_obj_add_flag(gui->band_confirm_overlay, LV_OBJ_FLAG_HIDDEN);
}

static void close_overlay_event_cb(lv_event_t * event)
{
    lv_obj_t * overlay = lv_event_get_user_data(event);
    if(lv_event_get_target_obj(event) == overlay) {
        lv_obj_add_flag(overlay, LV_OBJ_FLAG_HIDDEN);
    }
}
