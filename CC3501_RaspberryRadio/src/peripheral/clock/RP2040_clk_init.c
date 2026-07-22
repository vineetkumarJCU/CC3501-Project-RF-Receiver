#include "RP2040_clk_init.h"
#include "hardware/clocks.h"
#include "hardware/vreg.h"
#include "board_pin_def.h"
#include "stdio.h"
#include "pico/stdlib.h"

static void rp2040_sys_clock_init()
{
  vreg_set_voltage(VREG_VOLTAGE_1_30);              // Required minimum VREG for the 192 MHz system clock
  sleep_ms(10);
  set_sys_clock_khz(RP2040_SYS_CLK_HZ / KHZ, true); // Set system clock to 192 MHz
  clock_configure(clk_peri,
                  0,                                         // No aux mux
                  CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLK_SYS, // System clock on AUX mux
                  RP2040_SYS_CLK_HZ,                         // Input frequency is sys clock
                  RP2040_SYS_CLK_HZ);                        // Output is the same
}

void rp2040_clocks_test()
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

static void si4732_rclk_from_rp2040_init()
{
  float div = (float)RP2040_SYS_CLK_HZ / (32768.0f - SI4732_RCLK_FREQ_32K_OFFSET);

  clock_gpio_init_int_frac16(
      SI4732_RCLK_PIN,
      CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_CLK_SYS,
      (uint32_t)div,
      (uint16_t)(65536u * (div - (uint16_t)(div))));
  // 192MHz / 5859.375 = 32768Hz
  gpio_set_drive_strength(SI4732_RCLK_PIN, GPIO_DRIVE_STRENGTH_2MA); // Set drive strength
  gpio_set_slew_rate(SI4732_RCLK_PIN, GPIO_SLEW_RATE_SLOW); // Set slew rate
}

void rp2040_clocks_init()
{
  rp2040_sys_clock_init();
  si4732_rclk_from_rp2040_init();
}
