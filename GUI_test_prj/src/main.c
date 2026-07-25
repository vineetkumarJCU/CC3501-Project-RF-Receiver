#include "lvgl.h"
#include "demo/radio_gui_demo_backend.h"
#include "gui/radio_gui.h"

#include "src/drivers/sdl/lv_sdl_keyboard.h"
#include "src/drivers/sdl/lv_sdl_mouse.h"
#include "src/drivers/sdl/lv_sdl_mousewheel.h"
#include "src/drivers/sdl/lv_sdl_window.h"

#define SDL_MAIN_HANDLED
#include <SDL.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool save_renderer_snapshot(lv_display_t * display, const char * path)
{
    SDL_Renderer * renderer = lv_sdl_window_get_renderer(display);
    if(renderer == NULL || path == NULL || path[0] == '\0') return false;

    int width = 0;
    int height = 0;
    if(SDL_GetRendererOutputSize(renderer, &width, &height) != 0) return false;

    SDL_Surface * surface = SDL_CreateRGBSurfaceWithFormat(0,
                                                           width,
                                                           height,
                                                           32,
                                                           SDL_PIXELFORMAT_ARGB8888);
    if(surface == NULL) return false;

    bool saved = false;
    if(SDL_RenderReadPixels(renderer,
                            NULL,
                            SDL_PIXELFORMAT_ARGB8888,
                            surface->pixels,
                            surface->pitch) == 0) {
        saved = SDL_SaveBMP(surface, path) == 0;
    }
    SDL_FreeSurface(surface);
    return saved;
}

enum {
    WINDOW_WIDTH = 320,
    WINDOW_HEIGHT = 240
};

int main(void)
{
    lv_init();

    lv_display_t * display = lv_sdl_window_create(WINDOW_WIDTH, WINDOW_HEIGHT);
    if(display == NULL) {
        return 1;
    }

    lv_sdl_window_set_title(display, "RP2040 Radio Dashboard - LVGL v9.5.0");
    lv_sdl_window_set_resizeable(display, true);

    (void)lv_sdl_mouse_create();
    (void)lv_sdl_mousewheel_create();
    (void)lv_sdl_keyboard_create();

    radio_gui_config_t gui_config = radio_gui_demo_backend_config();
    radio_gui_t * gui = radio_gui_create(lv_screen_active(), &gui_config);
    printf("[GUI] GUI created: %p\n", (void *)gui);
    if(gui == NULL) {
        return 2;
    }
    radio_gui_demo_backend_populate(gui);

    const char * start_page = getenv("RADIO_GUI_START_PAGE");
    if(start_page != NULL && strcmp(start_page, "hardware") == 0) {
        radio_gui_show_page(gui, RADIO_GUI_PAGE_HARDWARE, LV_ANIM_OFF);
    }
    else if(start_page != NULL && strcmp(start_page, "waveform") == 0) {
        radio_gui_show_page(gui, RADIO_GUI_PAGE_WAVEFORM, LV_ANIM_OFF);
    }
    else if(start_page != NULL && strcmp(start_page, "gps") == 0) {
        radio_gui_show_page(gui, RADIO_GUI_PAGE_GPS, LV_ANIM_OFF);
    }
    if(getenv("RADIO_GUI_SHOW_KEYPAD") != NULL) {
        radio_gui_show_frequency_keypad(gui);
    }
    if(getenv("RADIO_GUI_SHOW_RESULT") != NULL) {
        radio_gui_show_tune_result(gui,
                                   true,
                                   "SI4732 FM tune success for 101.5 MHz\nBLTF=1\nAFCRL=1\nVALID=1\nFreq=101500000Hz\nRSSI=-65dBm\nSNR=30dB\nMultipath=1");
    }
    if(getenv("RADIO_GUI_PRINT_STATE") != NULL) {
        char filter_name[32];
        bool have_filter = radio_gui_get_channel_filter(gui, filter_name, sizeof(filter_name));
        printf("[GUI STATE] page=%d band=%d unit=%d frequency=%s volume=%u filter=%s\n",
               (int)radio_gui_get_current_page(gui),
               (int)radio_gui_get_band(gui),
               (int)radio_gui_get_frequency_unit(gui),
               radio_gui_get_frequency_text(gui),
               (unsigned)radio_gui_get_volume(gui),
               have_filter ? filter_name : "<unavailable>");
        printf("[GUI STATE] battery=%.2f amp=%d amp_mode=%d backlight=%u\n",
               (double)radio_gui_get_battery_voltage(gui),
               radio_gui_get_audio_amp_enabled(gui),
               (int)radio_gui_get_audio_amp_mode(gui),
               (unsigned)radio_gui_get_backlight(gui));
        printf("[GUI STATE] gps=%d sd_log=%d sd_status=%d gps_status=%d rssi=%s latitude=%s\n",
               radio_gui_get_gps_enabled(gui),
               radio_gui_get_sd_logging_enabled(gui),
               (int)radio_gui_get_sd_status(gui),
               (int)radio_gui_get_gps_status(gui),
               radio_gui_get_rx_field(gui, RADIO_GUI_RX_RSSI),
               radio_gui_get_gps_field(gui, RADIO_GUI_GPS_LATITUDE));
        fflush(stdout);
    }

    const char * screenshot_path = getenv("RADIO_GUI_SCREENSHOT");
    const char * screenshot_delay_text = getenv("RADIO_GUI_SCREENSHOT_DELAY_MS");
    printf("%s\n%s\n", screenshot_path, screenshot_delay_text);
    uint32_t screenshot_delay_ms = screenshot_delay_text != NULL
                                       ? (uint32_t)strtoul(screenshot_delay_text, NULL, 10)
                                       : 750U;
    uint32_t screenshot_start = SDL_GetTicks();
    bool screenshot_complete = screenshot_path == NULL;

    for(;;) {
        uint32_t delay_ms = lv_timer_handler();
        if(delay_ms < 1U) delay_ms = 1U;
        if(delay_ms > 5U) delay_ms = 5U;
        lv_delay_ms(delay_ms);

        if(!screenshot_complete && SDL_GetTicks() - screenshot_start >= screenshot_delay_ms) {
            screenshot_complete = save_renderer_snapshot(display, screenshot_path);
        }
    }
}
