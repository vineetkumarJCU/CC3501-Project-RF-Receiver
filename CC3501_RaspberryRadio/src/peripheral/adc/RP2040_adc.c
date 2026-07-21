#include "RP2040_adc.h"

#include "hardware/adc.h"

#include "board_pin_def.h"

#include "pico/stdlib.h"

#include "stdbool.h"

#include "stdint.h"


static void battery_monitor_adc_gpio_init()
{
  adc_init();
  adc_gpio_init(BATTERY_MONITOR_PIN);
  adc_select_input(BATTERY_MONITOR_ADC_CH);
}

static void audio_jack_plugin_detect_adc_gpio_init()
{
  adc_init();
  adc_gpio_init(AUDIO_JACK_DETECT_PIN);
  adc_select_input(AUDIO_JACK_DETECT_ADC_CH);
}

void RP2040_adc_init(void)
{
  battery_monitor_adc_gpio_init();
  audio_jack_plugin_detect_adc_gpio_init();
}

float RP2040_adc_read_battery_voltage()
{
  adc_select_input(BATTERY_MONITOR_ADC_CH);
  uint32_t adc_value = 0;
  for (uint8_t i = 0; i < 64; i++)//4^3=64, so it brings 3-bit more resolution to the ADC reading
  {
    adc_value += adc_read();
    sleep_ms(1); 
  }
  adc_value = adc_value >> 3; // right shift by 3 to divide by 8 to get ideal 15-bit (12+3) resolution
  // Convert ADC value to voltage (assuming 3.3V reference and 12-bit ADC)
  float voltage = (adc_value / 32768.0f) * 3.3f;
  return voltage*2.0f*1.0288f;
  //1.0288 is cal
  // a voltage divider with equal resistors, multiply by 2 to get the actual battery voltage
}

bool RP2040_adc_is_audio_jack_inserted()
{
  adc_select_input(AUDIO_JACK_DETECT_ADC_CH);
  uint16_t adc_value = adc_read();
  // Assuming that when the audio jack is inserted, the ADC value will be below a certain threshold
  return adc_value < AUDIO_JACK_INSERTED_THRESHOLD;
}
