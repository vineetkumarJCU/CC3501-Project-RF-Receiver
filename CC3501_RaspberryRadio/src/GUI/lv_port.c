#include "lv_port.h"

#include <stdint.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "CST816D.h"
#include "ST7789_LCD.h"
#include "lvgl.h"

#define LV_PORT_HORIZONTAL_RESOLUTION ST7789_VIRTUAL_SIZE_X
#define LV_PORT_VERTICAL_RESOLUTION   ST7789_VIRTUAL_SIZE_Y
#define LV_PORT_BUFFER_ROWS ST7789_VIRTUAL_SIZE_Y/2

static uint16_t display_buffer_1[LV_PORT_HORIZONTAL_RESOLUTION * LV_PORT_BUFFER_ROWS]
    __attribute__((aligned(LV_DRAW_BUF_ALIGN)));
// static uint16_t display_buffer_2[LV_PORT_HORIZONTAL_RESOLUTION * LV_PORT_BUFFER_ROWS]
//     __attribute__((aligned(LV_DRAW_BUF_ALIGN)));

static SemaphoreHandle_t display_flush_semaphore = NULL;
static bool display_dma_enabled = false;

static void lv_port_display_dma_complete(void *context)
{
    lv_display_t *display = (lv_display_t *)context;
    BaseType_t higher_priority_task_woken = pdFALSE;

    lv_display_flush_ready(display);
    if (display_flush_semaphore != NULL) {
        xSemaphoreGiveFromISR(display_flush_semaphore,
                              &higher_priority_task_woken);
    }
    portYIELD_FROM_ISR(higher_priority_task_woken);
}

static void lv_port_display_flush(lv_display_t *display,
                                  const lv_area_t *area,
                                  uint8_t *pixel_map)
{
    if (!display_dma_enabled) {
        ST7789_FlushArea((uint16_t)area->x1,
                         (uint16_t)area->y1,
                         (uint16_t)area->x2,
                         (uint16_t)area->y2,
                         (uint16_t *)pixel_map);
        lv_display_flush_ready(display);
        return;
    }

    /* Remove a completion token left by a transfer that finished before LVGL
     * needed to wait for its single draw buffer. */
    (void)xSemaphoreTake(display_flush_semaphore, 0);

    if (!ST7789_FlushAreaDMA((uint16_t)area->x1,
                             (uint16_t)area->y1,
                             (uint16_t)area->x2,
                             (uint16_t)area->y2,
                             (const uint16_t *)pixel_map,
                             lv_port_display_dma_complete,
                             display)) {
        printf("[GUI] ERROR: Failed to start ST7789 DMA flush.\n");
        lv_display_flush_ready(display);
    }
}

static void lv_port_display_flush_wait(lv_display_t *display)
{
    (void)display;

    while (ST7789_IsFlushBusy()) {
        (void)xSemaphoreTake(display_flush_semaphore, portMAX_DELAY);
    }
}

void lv_port_display_init(void)
{
    ST7789_Init();

    display_flush_semaphore = xSemaphoreCreateBinary();
    display_dma_enabled = (display_flush_semaphore != NULL) &&
                          ST7789_DMA_Init();
    if (!display_dma_enabled) {
        printf("[GUI] ERROR: ST7789 DMA is unavailable; using blocking SPI flush.\n");
    }

    lv_display_t *display = lv_display_create(LV_PORT_HORIZONTAL_RESOLUTION,
                                              LV_PORT_VERTICAL_RESOLUTION);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565_SWAPPED);
    lv_display_set_flush_cb(display, lv_port_display_flush);
    if (display_dma_enabled) {
        lv_display_set_flush_wait_cb(display, lv_port_display_flush_wait);
    }
    lv_display_set_buffers(display,
                           display_buffer_1,
                           NULL,
                           sizeof(display_buffer_1),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);
}

static void lv_port_touch_read(lv_indev_t *input, lv_indev_data_t *data)
{
    static lv_point_t last_point = {0, 0};
    (void)input;

    uint8_t finger_count = CST816_Get_FingerNum();
    if ((finger_count > 0U) && (finger_count != 0xFFU)) {
        uint16_t raw_x;
        uint16_t raw_y;
        CST816_Calculate_XY_Axis();
        CST816_Get_XY_Axis(&raw_x, &raw_y);

        int32_t mapped_x = (int32_t)raw_y;
        int32_t mapped_y = (int32_t)raw_x;
        if (mapped_x < 0) mapped_x = 0;
        if (mapped_x >= LV_PORT_HORIZONTAL_RESOLUTION) mapped_x = LV_PORT_HORIZONTAL_RESOLUTION - 1;
        if (mapped_y < 0) mapped_y = 0;
        if (mapped_y >= LV_PORT_VERTICAL_RESOLUTION) mapped_y = LV_PORT_VERTICAL_RESOLUTION - 1;

        last_point.x = mapped_x;
        last_point.y = mapped_y;
        data->state = LV_INDEV_STATE_PRESSED;
    }
    else {
        data->state = LV_INDEV_STATE_RELEASED;
    }

    data->point = last_point;
}

void lv_port_indev_init(void)
{
    CST816_Reset();
    CST816_Init();
    lv_indev_t *input = lv_indev_create();
    lv_indev_set_type(input, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(input, lv_port_touch_read);
}

static uint32_t lv_port_tick_get_ms(void)
{
    return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
}

void lv_port_tick_init(void)
{
    lv_tick_set_cb(lv_port_tick_get_ms);
}

void lv_port_init(void)
{
    lv_port_tick_init();
    lv_port_display_init();
    lv_port_indev_init();
}
