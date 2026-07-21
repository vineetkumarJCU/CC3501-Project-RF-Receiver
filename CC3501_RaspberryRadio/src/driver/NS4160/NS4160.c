#include "NS4160.h"

#include "board_pin_def.h"

#include "pico/stdlib.h"
#include "hardware/gpio.h"

void NS4160_powerup_class_AB()
{
  gpio_put(NS4160_CTRL_PIN, 0);
  sleep_ms(1);
  gpio_put(NS4160_CTRL_PIN, 1);
}

void NS4160_powerup_class_D()
{
  gpio_put(NS4160_CTRL_PIN, 0);
  sleep_ms(1);
  gpio_put(NS4160_CTRL_PIN, 1);
  sleep_us(1);
  gpio_put(NS4160_CTRL_PIN, 0);
  sleep_us(1);
  gpio_put(NS4160_CTRL_PIN, 1);
}

void NS4160_powerdown()
{
  gpio_put(NS4160_CTRL_PIN, 0);
}