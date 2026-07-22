#ifndef BOARD_INIT_H
#define BOARD_INIT_H

#include "hardware/clocks.h"
#include "hardware/i2c.h"
#include "hardware/spi.h"
#include "hardware/uart.h"

/* SPI Handle for LCD */
#define ST7789_SPI_HEADER spi1
/* IIC Handle for SI4732 */
#define SI4732_IIC_HANDLE i2c1
/* IIC Handle for CST816D */
#define CST816D_IIC_HANDLE i2c0
/* SPI Handle for SD card */
#define SD_CARD_SPI_HEADER spi0
/* Uart Handle for user communication */
#define USER_UART_HANDLE uart0
/* Uart Handle for GPS module communication */
#define GPS_UART_HANDLE uart1

#define ST7789_SPI_BAUD_RATE 120 * MHZ  // 120MHz to avoid SI4732's 64MHz~108MHz FM band
#define USER_UART_BAUD_RATE 2000000     // 2 Mbps
#define GPS_UART_BAUD_RATE 9600         // 9600 baud
#define SI4732_IIC_BAUD_RATE 1000 * KHZ // 1000kHz
#define CST816D_IIC_BAUD_RATE 400 * KHZ // 400kHz
#define SD_CARD_SPI_BAUD_RATE 16 * MHZ  // 16MHz

void board_init(void);

#endif // BOARD_INIT_H
