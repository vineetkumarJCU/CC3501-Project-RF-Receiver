#ifndef BOARD_INIT_H
#define BOARD_INIT_H

#include "board_pin_def.h"

#include "pico/stdlib.h"
#include "pico/divider.h"
#include "hardware/spi.h"
#include "hardware/i2c.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "hardware/interp.h"
#include "hardware/timer.h"
#include "hardware/watchdog.h"
#include "hardware/clocks.h"
#include "hardware/pll.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "hardware/vreg.h"
#include "hardware/pwm.h"



/* SPI Handle for LCD */
#define ST7789_SPI_HEADER			spi1
/* IIC Handle for SI4732 */
#define SI4732_IIC_HANDLE     i2c1



#define ST7789_SPI_BAUD_RATE 64*MHZ // 64MHz
#define SCREEN_BACKLIGHT_PWM_CLK_DIVIDER 256.0f // Fractional divider, format is fix8.4
#define SCREEN_BACKLIGHT_PWM_WARP 200 // Set PWM frequency to 5kHz with 256MHz clock and 256 divider
#define USER_UART_BAUD_RATE 2*MHZ // 2Mbps
#define SI4732_IIC_BAUD_RATE 400*KHZ // 400kHz
#define CST816D_IIC_BAUD_RATE 400*KHZ // 400kHz
#define SD_CARD_SPI_BAUD_RATE 16*MHZ // 16MHz


void board_init(void);

#endif // BOARD_INIT_H