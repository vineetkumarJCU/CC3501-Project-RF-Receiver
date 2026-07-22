#ifndef _RP2040_PWM_H_
#define _RP2040_PWM_H_

#include "pico/stdlib.h"

#define SCREEN_BACKLIGHT_PWM_CLK_DIVIDER 240 // Fractional divider, format is fix8.4
#define SCREEN_BACKLIGHT_PWM_WARP (50 - 1)   // Set PWM frequency to 20kHz with 192MHz clock and 9600 divider

void RP2040_screen_pwm_init();
void RP2040_screen_pwm_set_level(uint8_t level);


#endif // _RP2040_PWM_H_