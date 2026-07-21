#ifndef _RP2040_ADC_H_
#define _RP2040_ADC_H_

#include "pico/stdlib.h"
#include "stdbool.h"

#define AUDIO_JACK_INSERTED_THRESHOLD 1024

void RP2040_adc_init(void);
float RP2040_adc_read_battery_voltage();
bool RP2040_adc_is_audio_jack_inserted();

#endif // _RP2040_ADC_H_