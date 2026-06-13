#include "board_init.h"




static void rp2040_clock_init()
{
  vreg_set_voltage(VREG_VOLTAGE_1_30);// Set VREG to 1.3V
  set_sys_clock_khz(256*KHZ, true);// Set system clock to 256MHz
  clock_configure(clk_peri,
                  0, // No aux mux
                  CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLK_SYS, // System clock on AUX mux
                  256 * MHZ, // Input frequency is sys clock
                  256 * MHZ); // Output is the same
}

static void rp2040_clocks_test()
{
  uint32_t f_pll_sys = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_PLL_SYS_CLKSRC_PRIMARY);
  uint32_t f_pll_usb = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_PLL_USB_CLKSRC_PRIMARY);
  uint32_t f_rosc = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_ROSC_CLKSRC);
  uint32_t f_clk_sys = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS);
  uint32_t f_clk_peri = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_PERI);
  uint32_t f_clk_usb = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_USB);
  uint32_t f_clk_adc = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_ADC);
  uint32_t f_clk_rtc = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_RTC);

  printf("pll_sys  = %dkHz\n", f_pll_sys);
  printf("pll_usb  = %dkHz\n", f_pll_usb);
  printf("rosc     = %dkHz\n", f_rosc);
  printf("clk_sys  = %dkHz\n", f_clk_sys);
  printf("clk_peri = %dkHz\n", f_clk_peri);
  printf("clk_usb  = %dkHz\n", f_clk_usb);
  printf("clk_adc  = %dkHz\n", f_clk_adc);
  printf("clk_rtc  = %dkHz\n", f_clk_rtc);
  printf("\n");

  // Can't measure clk_ref / xosc as it is the reference for the measurement
}

static void ST7789_spi_init()
{
  gpio_set_function(ST7789_SPI1_SCLK_PIN, GPIO_FUNC_SPI);
  gpio_set_function(ST7789_SPI1_MOSI_PIN, GPIO_FUNC_SPI);
  gpio_set_function(ST7789_SPI1_CS_PIN, GPIO_FUNC_SPI);
  gpio_set_drive_strength(ST7789_SPI1_SCLK_PIN, GPIO_DRIVE_STRENGTH_12MA); // Set drive strength
  gpio_set_drive_strength(ST7789_SPI1_MOSI_PIN, GPIO_DRIVE_STRENGTH_12MA); // Set drive strength
  gpio_set_drive_strength(ST7789_SPI1_CS_PIN, GPIO_DRIVE_STRENGTH_12MA); // Set drive strength
  gpio_set_slew_rate(ST7789_SPI1_SCLK_PIN, GPIO_SLEW_RATE_FAST); // Set slew rate
  gpio_set_slew_rate(ST7789_SPI1_MOSI_PIN, GPIO_SLEW_RATE_FAST); // Set slew rate
  gpio_set_slew_rate(ST7789_SPI1_CS_PIN, GPIO_SLEW_RATE_FAST); // Set slew rate
  spi_set_format(ST7789_SPI_HEADER,8,SPI_CPOL_0,SPI_CPHA_0,SPI_MSB_FIRST);
  spi_init(ST7789_SPI_HEADER, ST7789_SPI_BAUD_RATE); // Initialize SPI1
}

static void ST7789_control_gpio_init()
{
  gpio_set_dir(ST7789_RES_PIN, GPIO_OUT);
  gpio_set_dir(ST7789_DC_PIN, GPIO_OUT);
  gpio_set_drive_strength(ST7789_RES_PIN, GPIO_DRIVE_STRENGTH_4MA); // Set drive strength
  gpio_set_drive_strength(ST7789_DC_PIN, GPIO_DRIVE_STRENGTH_12MA); // Set drive strength
  gpio_set_slew_rate(ST7789_RES_PIN, GPIO_SLEW_RATE_SLOW); // Set slew rate
  gpio_set_slew_rate(ST7789_DC_PIN, GPIO_SLEW_RATE_FAST); // Set slew rate
  gpio_init(ST7789_RES_PIN);
  gpio_init(ST7789_DC_PIN);
}

static void screen_backlight_PWM_init()
{
  gpio_set_function(SCREEN_PWM0A_BLK_PIN, GPIO_FUNC_PWM);
  gpio_set_drive_strength(SCREEN_PWM0A_BLK_PIN, GPIO_DRIVE_STRENGTH_4MA); // Set drive strength
  gpio_set_slew_rate(SCREEN_PWM0A_BLK_PIN, GPIO_SLEW_RATE_SLOW); // Set slew rate
  
  uint slice_num = pwm_gpio_to_slice_num(SCREEN_PWM0A_BLK_PIN);

  pwm_config cfg = pwm_get_default_config();
  pwm_config_set_clkdiv(&cfg, SCREEN_BACKLIGHT_PWM_CLK_DIVIDER); // Set divider to reduce counter //fractional divider, format is fix8.4
  pwm_init(slice_num, &cfg, false); // Use the configuration and don't start yet

  pwm_set_wrap(slice_num, SCREEN_BACKLIGHT_PWM_WARP); // Set PWM frequency
  pwm_set_chan_level(slice_num, PWM_CHAN_A, 50); // Set duty cycle to 50%
  pwm_set_enabled(slice_num, true); // Enable the PWM slice
}

static void screen_wakeup_gpio_init()
{
  gpio_set_dir(NS4160_CTRL_PIN, GPIO_IN);
  // gpio_set_drive_strength(NS4160_CTRL_PIN, GPIO_DRIVE_STRENGTH_4MA); // Set drive strength
  // gpio_set_slew_rate(NS4160_CTRL_PIN, GPIO_SLEW_RATE_SLOW); // Set slew rate
  gpio_init(NS4160_CTRL_PIN);
}

static void gps_uart_init()
{
  gpio_set_function(GPS_UART1_TX_PIN, GPIO_FUNC_UART);
  gpio_set_function(GPS_UART1_RX_PIN, GPIO_FUNC_UART);
  gpio_set_drive_strength(GPS_UART1_TX_PIN, GPIO_DRIVE_STRENGTH_4MA); // Set drive strength
  gpio_set_drive_strength(GPS_UART1_RX_PIN, GPIO_DRIVE_STRENGTH_4MA); // Set drive strength
  gpio_set_slew_rate(GPS_UART1_TX_PIN, GPIO_SLEW_RATE_SLOW); // Set slew rate
  gpio_set_slew_rate(GPS_UART1_RX_PIN, GPIO_SLEW_RATE_SLOW); // Set slew rate

  uart_set_format(uart1, 8, 1, UART_PARITY_NONE);
  uart_init(uart1, 9600); // Initialize UART1 with baud rate 9600
}

static void gps_powerup_gpio_init()
{
  gpio_set_dir(GPS_POWER_UP_PIN, GPIO_OUT);
  gpio_set_drive_strength(GPS_POWER_UP_PIN, GPIO_DRIVE_STRENGTH_2MA); // Set drive strength
  gpio_set_slew_rate(GPS_POWER_UP_PIN, GPIO_SLEW_RATE_SLOW); // Set slew rate
  gpio_init(GPS_POWER_UP_PIN);
}

static void gps_pps_gpio_init()
{
  gpio_set_dir(GPS_PPS_PIN, GPIO_IN);
  // gpio_set_drive_strength(GPS_PPS_PIN, GPIO_DRIVE_STRENGTH_2MA); // Set drive strength
  // gpio_set_slew_rate(GPS_PPS_PIN, GPIO_SLEW_RATE_SLOW); // Set slew rate
  gpio_init(GPS_PPS_PIN);
}

static void user_uart_init()
{
  gpio_set_function(USER_UART_TX_PIN, GPIO_FUNC_UART);
  gpio_set_function(USER_UART_RX_PIN, GPIO_FUNC_UART);
  gpio_set_drive_strength(USER_UART_TX_PIN, GPIO_DRIVE_STRENGTH_8MA); // Set drive strength
  gpio_set_drive_strength(USER_UART_RX_PIN, GPIO_DRIVE_STRENGTH_8MA); // Set drive strength
  gpio_set_slew_rate(USER_UART_TX_PIN, GPIO_SLEW_RATE_SLOW); // Set slew rate
  gpio_set_slew_rate(USER_UART_RX_PIN, GPIO_SLEW_RATE_SLOW); // Set slew rate

  uart_set_format(uart0, 8, 2, UART_PARITY_EVEN);
  uart_init(uart0, USER_UART_BAUD_RATE); // Initialize UART0 with baud rate 2000000
}

static void si4732_rst_gpio_init()
{
  gpio_set_dir(SI4732_RST_PIN, GPIO_OUT);
  gpio_set_drive_strength(SI4732_RST_PIN, GPIO_DRIVE_STRENGTH_4MA); // Set drive strength
  gpio_set_slew_rate(SI4732_RST_PIN, GPIO_SLEW_RATE_SLOW); // Set slew rate
  gpio_init(SI4732_RST_PIN);
}

static void si4732_i2c_init()
{
  gpio_set_function(SI4732_IIC1_SDA_PIN, GPIO_FUNC_I2C);
  gpio_set_function(SI4732_IIC1_SCL_PIN, GPIO_FUNC_I2C);
  gpio_set_drive_strength(SI4732_IIC1_SDA_PIN, GPIO_DRIVE_STRENGTH_4MA);
  gpio_set_drive_strength(SI4732_IIC1_SCL_PIN, GPIO_DRIVE_STRENGTH_4MA);
  gpio_set_slew_rate(SI4732_IIC1_SDA_PIN, GPIO_SLEW_RATE_SLOW); // Set slew rate
  gpio_set_slew_rate(SI4732_IIC1_SCL_PIN, GPIO_SLEW_RATE_SLOW); // Set slew rate
  
  i2c_init(SI4732_IIC_HANDLE, SI4732_IIC_BAUD_RATE); // Initialize I2C1
}

static void cst816d_i2c_init()
{
  gpio_set_function(CST816D_IIC0_SDA_PIN, GPIO_FUNC_I2C);
  gpio_set_function(CST816D_IIC0_SCL_PIN, GPIO_FUNC_I2C);
  gpio_set_drive_strength(CST816D_IIC0_SDA_PIN, GPIO_DRIVE_STRENGTH_4MA);
  gpio_set_drive_strength(CST816D_IIC0_SCL_PIN, GPIO_DRIVE_STRENGTH_4MA);
  gpio_set_slew_rate(CST816D_IIC0_SDA_PIN, GPIO_SLEW_RATE_SLOW); // Set slew rate
  gpio_set_slew_rate(CST816D_IIC0_SCL_PIN, GPIO_SLEW_RATE_SLOW); // Set slew rate

  i2c_init(i2c0, CST816D_IIC_BAUD_RATE); // Initialize I2C0
}

static void cst816d_gpio_init()
{
  gpio_set_dir(CST816D_INT_PIN, GPIO_IN);
  gpio_set_dir(CST816D_RST_PIN, GPIO_OUT);
  // gpio_set_drive_strength(CST816D_INT_PIN, GPIO_DRIVE_STRENGTH_4MA); // Set drive strength
  gpio_set_drive_strength(CST816D_RST_PIN, GPIO_DRIVE_STRENGTH_4MA); // Set drive strength
  // gpio_set_slew_rate(CST816D_INT_PIN, GPIO_SLEW_RATE_SLOW); // Set slew rate
  gpio_set_slew_rate(CST816D_RST_PIN, GPIO_SLEW_RATE_FAST); // Set slew rate
  gpio_init(CST816D_INT_PIN);
  gpio_init(CST816D_RST_PIN);
}

static void NS4160_ctrl_gpio_init()
{
  gpio_set_dir(NS4160_CTRL_PIN, GPIO_OUT);
  gpio_set_drive_strength(NS4160_CTRL_PIN, GPIO_DRIVE_STRENGTH_4MA); // Set drive strength
  gpio_set_slew_rate(NS4160_CTRL_PIN, GPIO_SLEW_RATE_SLOW); // Set slew rate
  gpio_init(NS4160_CTRL_PIN);
}

static void sd_card_spi_init()
{
  gpio_set_function(SD_CARD_SPI0_SCLK_PIN, GPIO_FUNC_SPI);
  gpio_set_function(SD_CARD_SPI0_MOSI_PIN, GPIO_FUNC_SPI);
  gpio_set_function(SD_CARD_SPI0_MISO_PIN,GPIO_FUNC_SPI);
  gpio_set_function(SD_CARD_SPI0_CS_PIN,GPIO_FUNC_SPI);
  gpio_set_drive_strength(SD_CARD_SPI0_SCLK_PIN, GPIO_DRIVE_STRENGTH_12MA); // Set drive strength
  gpio_set_drive_strength(SD_CARD_SPI0_MOSI_PIN, GPIO_DRIVE_STRENGTH_12MA); // Set drive strength
  gpio_set_drive_strength(SD_CARD_SPI0_MISO_PIN, GPIO_DRIVE_STRENGTH_12MA); // Set drive strength
  gpio_set_drive_strength(SD_CARD_SPI0_CS_PIN, GPIO_DRIVE_STRENGTH_12MA); // Set drive strength
  gpio_set_slew_rate(SD_CARD_SPI0_SCLK_PIN, GPIO_SLEW_RATE_FAST); // Set slew rate
  gpio_set_slew_rate(SD_CARD_SPI0_MOSI_PIN, GPIO_SLEW_RATE_FAST); // Set slew rate
  gpio_set_slew_rate(SD_CARD_SPI0_MISO_PIN, GPIO_SLEW_RATE_FAST); // Set slew rate
  gpio_set_slew_rate(SD_CARD_SPI0_CS_PIN, GPIO_SLEW_RATE_FAST); // Set slew rate
  spi_set_format(spi0,8,SPI_CPOL_0,SPI_CPHA_0,SPI_MSB_FIRST);
  spi_init(spi0, SD_CARD_SPI_BAUD_RATE); // Initialize SPI0
}

static void sd_card_detect_gpio_init()
{
  gpio_set_function(SD_CARD_DETECT_PIN, GPIO_FUNC_SPI);
  gpio_set_dir(SD_CARD_DETECT_PIN, GPIO_IN);
  // gpio_set_drive_strength(SD_CARD_DETECT_PIN, GPIO_DRIVE_STRENGTH_4MA); // Set drive strength
  // gpio_set_slew_rate(SD_CARD_DETECT_PIN, GPIO_SLEW_RATE_SLOW); // Set slew rate
  gpio_init(SD_CARD_DETECT_PIN);
}

static void battery_monitor_adc_gpio_init()
{
  adc_gpio_init(BATTERY_MONITOR_PIN);
  adc_select_input(BATTERY_MONITOR_ADC_CH);
  adc_init();
}

static void audio_jack_plugin_detect_adc_gpio_init()
{
  adc_gpio_init(AUDIO_JACK_DETECT_PIN);
  adc_select_input(AUDIO_JACK_DETECT_PIN);
  adc_init();
}

void board_init()
{
  rp2040_clock_init();
  rp2040_clocks_test();
  ST7789_spi_init();
  ST7789_control_gpio_init();
  screen_backlight_PWM_init();
  screen_wakeup_gpio_init();
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
  battery_monitor_adc_gpio_init();
  audio_jack_plugin_detect_adc_gpio_init();
}