#include "board_init.h"

#include <stdint.h>
#include <stdio.h>

#include "board_pin_def.h"
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/vreg.h"

static void ST7789_spi_init()
{
  spi_init(ST7789_SPI_HEADER, ST7789_SPI_BAUD_RATE); // Initialize SPI1
  spi_set_format(ST7789_SPI_HEADER, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_LSB_FIRST);
  gpio_set_function(ST7789_SPI1_SCLK_PIN, GPIO_FUNC_SPI);
  gpio_set_function(ST7789_SPI1_MOSI_PIN, GPIO_FUNC_SPI);
  gpio_set_function(ST7789_SPI1_CS_PIN, GPIO_FUNC_SPI);
  gpio_set_drive_strength(ST7789_SPI1_SCLK_PIN, GPIO_DRIVE_STRENGTH_2MA); // Set drive strength
  gpio_set_drive_strength(ST7789_SPI1_MOSI_PIN, GPIO_DRIVE_STRENGTH_2MA); // Set drive strength
  gpio_set_drive_strength(ST7789_SPI1_CS_PIN, GPIO_DRIVE_STRENGTH_2MA); // Set drive strength
  gpio_set_slew_rate(ST7789_SPI1_SCLK_PIN, GPIO_SLEW_RATE_SLOW); // Set slew rate
  gpio_set_slew_rate(ST7789_SPI1_MOSI_PIN, GPIO_SLEW_RATE_SLOW); // Set slew rate
  gpio_set_slew_rate(ST7789_SPI1_CS_PIN, GPIO_SLEW_RATE_SLOW); // Set slew rate
}

static void ST7789_control_gpio_init()
{
  gpio_init(ST7789_RES_PIN);
  gpio_init(ST7789_DC_PIN);
  gpio_set_dir(ST7789_RES_PIN, GPIO_OUT);
  gpio_set_dir(ST7789_DC_PIN, GPIO_OUT);
  gpio_set_drive_strength(ST7789_RES_PIN, GPIO_DRIVE_STRENGTH_2MA); // Set drive strength
  gpio_set_drive_strength(ST7789_DC_PIN, GPIO_DRIVE_STRENGTH_2MA); // Set drive strength
  gpio_set_slew_rate(ST7789_RES_PIN, GPIO_SLEW_RATE_SLOW); // Set slew rate
  gpio_set_slew_rate(ST7789_DC_PIN, GPIO_SLEW_RATE_SLOW); // Set slew rate
}

static void screen_wakeup_gpio_init()
{
  gpio_init(SCREEN_WAKE_PIN);
  gpio_set_dir(SCREEN_WAKE_PIN, GPIO_IN);
}

static void gps_uart_init()
{
  gpio_set_function(GPS_UART1_TX_PIN, GPIO_FUNC_UART);
  gpio_set_function(GPS_UART1_RX_PIN, GPIO_FUNC_UART);
  gpio_set_drive_strength(GPS_UART1_TX_PIN, GPIO_DRIVE_STRENGTH_2MA); // Set drive strength
  gpio_set_drive_strength(GPS_UART1_RX_PIN, GPIO_DRIVE_STRENGTH_2MA); // Set drive strength
  gpio_set_slew_rate(GPS_UART1_TX_PIN, GPIO_SLEW_RATE_SLOW); // Set slew rate
  gpio_set_slew_rate(GPS_UART1_RX_PIN, GPIO_SLEW_RATE_SLOW); // Set slew rate

  uart_init(GPS_UART_HANDLE, GPS_UART_BAUD_RATE); // Initialize UART1 with baud rate 9600
  uart_set_format(GPS_UART_HANDLE, 8, 1, UART_PARITY_NONE);
}

static void gps_powerup_gpio_init()
{
  gpio_init(GPS_POWER_UP_PIN);
  gpio_put(GPS_POWER_UP_PIN,1);//power down GPS
  gpio_set_dir(GPS_POWER_UP_PIN, GPIO_OUT);
  gpio_set_drive_strength(GPS_POWER_UP_PIN, GPIO_DRIVE_STRENGTH_2MA); // Set drive strength
  gpio_set_slew_rate(GPS_POWER_UP_PIN, GPIO_SLEW_RATE_SLOW); // Set slew rate
}

static void gps_pps_gpio_init()
{
  gpio_init(GPS_PPS_PIN);
  gpio_set_dir(GPS_PPS_PIN, GPIO_IN);
}

static void user_uart_init()
{
  gpio_set_function(USER_UART_TX_PIN, GPIO_FUNC_UART);
  gpio_set_function(USER_UART_RX_PIN, GPIO_FUNC_UART);
  gpio_set_drive_strength(USER_UART_TX_PIN, GPIO_DRIVE_STRENGTH_2MA); // Set drive strength
  gpio_set_drive_strength(USER_UART_RX_PIN, GPIO_DRIVE_STRENGTH_2MA); // Set drive strength
  gpio_set_slew_rate(USER_UART_TX_PIN, GPIO_SLEW_RATE_SLOW); // Set slew rate
  gpio_set_slew_rate(USER_UART_RX_PIN, GPIO_SLEW_RATE_SLOW); // Set slew rate

  uart_init(USER_UART_HANDLE, USER_UART_BAUD_RATE); // Initialize UART0 with baud rate 115200
  uart_set_format(USER_UART_HANDLE, 8, 1, UART_PARITY_NONE);
}

static void si4732_rst_gpio_init()
{
  gpio_init(SI4732_RST_PIN);
  gpio_set_dir(SI4732_RST_PIN, GPIO_OUT);
  gpio_set_drive_strength(SI4732_RST_PIN, GPIO_DRIVE_STRENGTH_2MA); // Set drive strength
  gpio_set_slew_rate(SI4732_RST_PIN, GPIO_SLEW_RATE_SLOW); // Set slew rate
}

static void si4732_i2c_init()
{
  i2c_init(SI4732_IIC_HANDLE, SI4732_IIC_BAUD_RATE); // Initialize I2C1
  gpio_set_function(SI4732_IIC1_SDA_PIN, GPIO_FUNC_I2C);
  gpio_set_function(SI4732_IIC1_SCL_PIN, GPIO_FUNC_I2C);
  gpio_set_drive_strength(SI4732_IIC1_SDA_PIN, GPIO_DRIVE_STRENGTH_2MA);
  gpio_set_drive_strength(SI4732_IIC1_SCL_PIN, GPIO_DRIVE_STRENGTH_2MA);
  gpio_set_slew_rate(SI4732_IIC1_SDA_PIN, GPIO_SLEW_RATE_SLOW); // Set slew rate
  gpio_set_slew_rate(SI4732_IIC1_SCL_PIN, GPIO_SLEW_RATE_SLOW); // Set slew rate
}

static void cst816d_i2c_init()
{
  i2c_init(CST816D_IIC_HANDLE, CST816D_IIC_BAUD_RATE); // Initialize I2C0
  gpio_set_function(CST816D_IIC0_SDA_PIN, GPIO_FUNC_I2C);
  gpio_set_function(CST816D_IIC0_SCL_PIN, GPIO_FUNC_I2C);
  gpio_set_drive_strength(CST816D_IIC0_SDA_PIN, GPIO_DRIVE_STRENGTH_2MA);
  gpio_set_drive_strength(CST816D_IIC0_SCL_PIN, GPIO_DRIVE_STRENGTH_2MA);
  gpio_set_slew_rate(CST816D_IIC0_SDA_PIN, GPIO_SLEW_RATE_SLOW); // Set slew rate
  gpio_set_slew_rate(CST816D_IIC0_SCL_PIN, GPIO_SLEW_RATE_SLOW); // Set slew rate
}

static void cst816d_gpio_init()
{
  gpio_init(CST816D_INT_PIN);
  gpio_init(CST816D_RST_PIN);
  gpio_set_dir(CST816D_INT_PIN, GPIO_IN);
  gpio_set_dir(CST816D_RST_PIN, GPIO_OUT);
  gpio_set_drive_strength(CST816D_RST_PIN, GPIO_DRIVE_STRENGTH_2MA); // Set drive strength
  gpio_set_slew_rate(CST816D_RST_PIN, GPIO_SLEW_RATE_SLOW); // Set slew rate
}

static void NS4160_ctrl_gpio_init()
{
  gpio_init(NS4160_CTRL_PIN);
  gpio_set_dir(NS4160_CTRL_PIN, GPIO_OUT);
  gpio_set_drive_strength(NS4160_CTRL_PIN, GPIO_DRIVE_STRENGTH_2MA); // Set drive strength
  gpio_set_slew_rate(NS4160_CTRL_PIN, GPIO_SLEW_RATE_SLOW); // Set slew rate
}

static void sd_card_spi_init()
{
  gpio_set_function(SD_CARD_SPI0_SCLK_PIN, GPIO_FUNC_SPI);
  gpio_set_function(SD_CARD_SPI0_MOSI_PIN, GPIO_FUNC_SPI);
  gpio_set_function(SD_CARD_SPI0_MISO_PIN,GPIO_FUNC_SPI);
  gpio_init(SD_CARD_SPI0_CS_PIN);
  gpio_put(SD_CARD_SPI0_CS_PIN, 1); // Software CS is inactive during startup.
  gpio_set_dir(SD_CARD_SPI0_CS_PIN, GPIO_OUT); // Set CS pin as output
  //cs pin is controlled by software, so set it as GPIO
  gpio_set_drive_strength(SD_CARD_SPI0_SCLK_PIN, GPIO_DRIVE_STRENGTH_2MA); // Set drive strength
  gpio_set_drive_strength(SD_CARD_SPI0_MOSI_PIN, GPIO_DRIVE_STRENGTH_2MA); // Set drive strength
  gpio_set_drive_strength(SD_CARD_SPI0_MISO_PIN, GPIO_DRIVE_STRENGTH_2MA); // Set drive strength
  gpio_set_drive_strength(SD_CARD_SPI0_CS_PIN, GPIO_DRIVE_STRENGTH_2MA); // Set drive strength
  gpio_set_slew_rate(SD_CARD_SPI0_SCLK_PIN, GPIO_SLEW_RATE_SLOW); // Set slew rate
  gpio_set_slew_rate(SD_CARD_SPI0_MOSI_PIN, GPIO_SLEW_RATE_SLOW); // Set slew rate
  gpio_set_slew_rate(SD_CARD_SPI0_MISO_PIN, GPIO_SLEW_RATE_SLOW); // Set slew rate
  gpio_set_slew_rate(SD_CARD_SPI0_CS_PIN, GPIO_SLEW_RATE_SLOW); // Set slew rate
  spi_init(SD_CARD_SPI_HEADER, SD_CARD_SPI_BAUD_RATE); // Initialize SPI0
  spi_set_format(SD_CARD_SPI_HEADER,8,SPI_CPOL_0,SPI_CPHA_0,SPI_MSB_FIRST);
}

static void sd_card_detect_gpio_init()
{
  gpio_init(SD_CARD_DETECT_PIN);
  gpio_set_dir(SD_CARD_DETECT_PIN, GPIO_IN);
  gpio_pull_up(SD_CARD_DETECT_PIN); // Card detect is active low.
}

void board_init()
{
  screen_wakeup_gpio_init();
  ST7789_control_gpio_init();
  ST7789_spi_init();

  gps_uart_init();
  gps_powerup_gpio_init();
  gps_pps_gpio_init();

  user_uart_init();

  si4732_rst_gpio_init();
  si4732_i2c_init();

  cst816d_i2c_init();
  cst816d_gpio_init();

  NS4160_ctrl_gpio_init();

  sd_card_spi_init();
  sd_card_detect_gpio_init();
}
