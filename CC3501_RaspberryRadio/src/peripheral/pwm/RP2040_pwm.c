#include "RP2040_pwm.h"
#include <stdint.h>

#include "board_pin_def.h"

#include "hardware/pwm.h"
#include "hardware/gpio.h"


void RP2040_screen_pwm_init()
{
  gpio_set_function(SCREEN_PWM0A_BLK_PIN, GPIO_FUNC_PWM);
  gpio_set_drive_strength(SCREEN_PWM0A_BLK_PIN, GPIO_DRIVE_STRENGTH_2MA); // Set drive strength
  gpio_set_slew_rate(SCREEN_PWM0A_BLK_PIN, GPIO_SLEW_RATE_SLOW);          // Set slew rate

  uint slice_num = pwm_gpio_to_slice_num(SCREEN_PWM0A_BLK_PIN);

  pwm_config cfg = pwm_get_default_config();
  pwm_config_set_clkdiv(&cfg, SCREEN_BACKLIGHT_PWM_CLK_DIVIDER); // Set divider to reduce counter //fractional divider, format is fix8.4
  pwm_init(slice_num, &cfg, false);                              // Use the configuration and don't start yet

  pwm_set_wrap(slice_num, SCREEN_BACKLIGHT_PWM_WARP);                                                      // Set PWM frequency
  pwm_set_chan_level(slice_num, pwm_gpio_to_channel(SCREEN_PWM0A_BLK_PIN), SCREEN_BACKLIGHT_PWM_WARP / 2); // Set duty cycle to 50%
  pwm_set_enabled(slice_num, true);                                                                        // Enable the PWM slice
}

void RP2040_screen_pwm_set_level(uint8_t level)
{
  uint slice_num = pwm_gpio_to_slice_num(SCREEN_PWM0A_BLK_PIN);
  pwm_set_chan_level(slice_num, pwm_gpio_to_channel(SCREEN_PWM0A_BLK_PIN), (uint16_t)level); // Set duty cycle based on level (0~SCREEN_BACKLIGHT_PWM_WARP-1);
}