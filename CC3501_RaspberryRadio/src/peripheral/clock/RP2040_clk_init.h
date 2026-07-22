#ifndef _RP2040_CLK_INIT_H_
#define _RP2040_CLK_INIT_H_

#include <stdint.h>

#define RP2040_SYS_CLK_HZ 240 * MHZ
#define SI4732_RCLK_FREQ_32K_OFFSET 1.46f // Frequency offset (Hz) between ideal 32768Hz output and actual output from RP2040 clock generator. This is a measured value.
void rp2040_clocks_init(void);
void rp2040_clocks_test();


#endif // _RP2040_CLK_INIT_H_
