#ifndef __BOARD_PIN_DEF_H__
#define __BOARD_PIN_DEF_H__

/* NS4160 AUDIO amplifier control pin */
#define NS4160_CTRL_PIN               0 // GPIO0  //Fn: GPIO  //Dir: Output

/* SI4732 RST pin */
#define SI4732_RST_PIN                1 // GPIO1  //Fn: GPIO  //Dir: Output

/* Secondary clock source pin for SI4732 RCLK */
#define SI4732_RCLK_PIN               21 // GPIO21  //Fn: GPIO  //Dir: Output

/* SI4732 communication pins */
#define SI4732_IIC1_SDA_PIN           2 // GPIO2  //Fn: I2C  //Dir: Bidirectional
#define SI4732_IIC1_SCL_PIN           3 // GPIO3  //Fn: I2C  //Dir: Bidirectional

/* CST816D touch controller pins */
#define CST816D_INT_PIN               6 // GPIO6  //Fn: GPIO  //Dir: Input
#define CST816D_RST_PIN               7 // GPIO7  //Fn: GPIO  //Dir: Output

/* CST816D touch communication pins */
#define CST816D_IIC0_SDA_PIN          8 // GPIO8  //Fn: I2C  //Dir: Bidirectional
#define CST816D_IIC0_SCL_PIN          9 // GPIO9  //Fn: I2C  //Dir: Bidirectional

/* ST7789 LCD control pins */
#define ST7789_RES_PIN                10 // GPIO10  //Fn: GPIO  //Dir: Output
#define ST7789_DC_PIN                 12 // GPIO12  //Fn: GPIO  //Dir: Output

/* ST7789 LCD communication pins */
#define ST7789_SPI1_SCLK_PIN          14 // GPIO14  //Fn: SPI  //Dir: Output
#define ST7789_SPI1_MOSI_PIN          11 // GPIO11  //Fn: SPI  //Dir: Output
#define ST7789_SPI1_CS_PIN            13 // GPIO13  //Fn: SPI  //Dir: Output

/* Screen backlight pin */
#define SCREEN_PWM0A_BLK_PIN          16 // GPIO16  //Fn: PWM  //Dir: Output

/* Screen wake-up pin */
#define SCREEN_WAKE_PIN               15 // GPIO15  //Fn: GPIO  //Dir: Input

/* SD card communication pins */
#define SD_CARD_SPI0_SCLK_PIN         18 // GPIO18  //Fn: SPI  //Dir: Output
#define SD_CARD_SPI0_MOSI_PIN         19 // GPIO19  //Fn: SPI  //Dir: Output
#define SD_CARD_SPI0_MISO_PIN         20 // GPIO20  //Fn: SPI  //Dir: Input
#define SD_CARD_SPI0_CS_PIN           17 // GPIO17  //Fn: SPI  //Dir: Output

/* SD card detection pin */
#define SD_CARD_DETECT_PIN            5 // GPIO5  //Fn: GPIO  //Dir: Input

/* GPS module (ATK-NEO-6M) power up control pin */
#define GPS_POWER_UP_PIN              22 // GPIO22  //Fn: GPIO  //Dir: Output

/* GPS module (ATK-NEO-6M) pps output pin */
#define GPS_PPS_PIN                   23 // GPIO23  //Fn: GPIO  //Dir: Input

/* GPS module (ATK-NEO-6M) serial communication pins */
#define GPS_UART1_RX_PIN              24 // GPIO24  //Fn: UART  //Dir: Output
#define GPS_UART1_TX_PIN              25 // GPIO25  //Fn: UART  //Dir: Input

/* Battery monitoring pin */
#define BATTERY_MONITOR_PIN           26 // GPIO26  //Fn: ADC  //Dir: Input
#define BATTERY_MONITOR_ADC_CH        0

/* Audio jack plug detection pin */
#define AUDIO_JACK_DETECT_PIN         27 // GPIO27  //Fn: ADC  //Dir: Input
#define AUDIO_JACK_DETECT_ADC_CH      1

/* User uart */
#define USER_UART_TX_PIN              28 // GPIO28  //Fn: UART  //Dir: Output
#define USER_UART_RX_PIN              29 // GPIO29  //Fn: UART  //Dir: Input

#endif // __BOARD_PIN_DEF_H__
