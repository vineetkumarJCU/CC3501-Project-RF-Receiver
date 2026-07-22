#include "main.h"
#include "stdio.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "GPS_module.h"


void main(void)
{
  //test function
  stdio_init_all();
  GPS_module_powerup();
  GPS_module_init();
  gps_info_t s;
  GPS_module_get_latest_info(&s);
  while (1)
  {
    sleep_ms(200);
    GPS_module_process();
    GPS_module_log_all_info(&s);
  }
}
