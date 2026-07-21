#include "main.h"
#include "RP2040_adc.h"
#include "NS4160.h"

void main(void)
{
  //test function
  stdio_init_all();

  RP2040_adc_init();
  while (1)
  {
    float v = RP2040_adc_read_battery_voltage();
    printf("Battery voltage: %.2f V\n", v);

    bool audio_jack_inserted = RP2040_adc_is_audio_jack_inserted();
    printf("Audio jack inserted: %s\n", audio_jack_inserted ? "Yes" : "No");
    
    sleep_ms(1000);
  }
}