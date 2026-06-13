#ifndef _ST7789_LCD_H_
#define _ST7789_LCD_H_

/* Includes from PICO SDK */
#include "board_pin_def.h"
#include "pico/stdlib.h"
#include "pico/divider.h"
#include "hardware/spi.h"
#include "hardware/i2c.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "hardware/interp.h"
#include "hardware/timer.h"
#include "hardware/watchdog.h"
#include "hardware/clocks.h"
#include "hardware/pll.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "hardware/vreg.h"
#include "hardware/pwm.h"

/* Includes from C standard library */
#include "math.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "stdarg.h"

/*Includes from user*/
#include "main.h"
#include "board_init.h"

/*Includes from ARM CMSIS*/
#include "arm_math.h"



/* LCD commands pin definitions */
#define ST7789_Write_RES(n)	 		(gpio_put(ST7789_RES_PIN,n))
#define ST7789_Write_DC(n)		 	(gpio_put(ST7789_DC_PIN,n))
//#define ST7789_Write_CS(n)		 	(HAL_GPIO_WritePin(SCREEN_CS_GPIO_Port,SCREEN_CS_Pin,n))
// #define ST7789_Write_BLK(n)		 	(HAL_GPIO_WritePin(SCREEN_BLK_GPIO_Port,SCREEN_BLK_Pin,n))


#define SCREEN_PHYSICAL_SIZE_X                 240//actual screen pixel width
#define SCREEN_PHYSICAL_SIZE_Y                 320//actual screen pixel height
#define SCREEN_DIRECTION             2 //0 and 1:is vertical		2,3 is horizontal

#if((SCREEN_DIRECTION==0)||(SCREEN_DIRECTION==1))//vertical screen
	#define ST7789_VIRTUAL_SIZE_X SCREEN_PHYSICAL_SIZE_X
	#define ST7789_VIRTUAL_SIZE_Y SCREEN_PHYSICAL_SIZE_Y
#elif((SCREEN_DIRECTION==2)||(SCREEN_DIRECTION==3))//horizontal screen
	#define ST7789_VIRTUAL_SIZE_X SCREEN_PHYSICAL_SIZE_Y
	#define ST7789_VIRTUAL_SIZE_Y SCREEN_PHYSICAL_SIZE_X
#endif

#define DRAW_OUT_OF_RANGE_BEHAVIOR   1 //0:block the dot on the edge of screen		1:abort the drawing process


extern void ST7789_Init();//Initialize the LCD
extern void ST7789_FillDot(uint16_t x,uint16_t y,uint16_t pPixel);//Fill a dot with a color
extern void ST7789_FlushArea(uint16_t x0,uint16_t y0,uint16_t x1,uint16_t y1,uint16_t *pPixel);


//画笔颜色
#define RGB565_WHITE						0xFFFF // 0xFFFF -> 0xFFFF
#define RGB565_BLACK						0x0000 // 0x0000 -> 0x0000
#define RGB565_BLUE							0x001F // 0x001F -> 0x1F00  
#define RGB565_BRED             0xF81F // 0xF81F -> 0x1FF8
#define RGB565_GRED							0xFFE0 // 0xFFE0 -> 0xE0FF
#define RGB565_GBLUE						0x07FF // 0x07FF -> 0xFF07
#define RGB565_RED							0xF800 // 0xF800 -> 0x00F8
#define RGB565_PINK							0xF81F // 0xF81F -> 0x1FF8
#define RGB565_GREEN						0x07E0 // 0x07E0 -> 0xE007
#define RGB565_CYAN1						0x7FFF // 0x7FFF -> 0xFF7F
#define RGB565_CYAN2						0x07FF // 0x07FF -> 0xFF07
#define RGB565_YELLOW						0xFFE0 // 0xFFE0 -> 0xE0FF
#define RGB565_BROWN						0xBC40 // 0xBC40 -> 0x40BC //棕色
#define RGB565_BRRED						0xFC07 // 0xFC07 -> 0x07FC //棕红色
#define RGB565_GRAY							0x8430 // 0x8430 -> 0x3084 //灰色
#define RGB565_PURPLE						0xF81F // 0xF81F -> 0x1FF8 //紫色
#define RGB565_ORANGE						0xFC01 // 0xFC01 -> 0x01FC //橙色
#define RGB565_GREENYELLOW			0xAFE5 // 0xAFE5 -> 0xE5AF //绿黄色
//GUI颜色
#define RGB565_DARKBLUE					0x01CF // 0x01CF -> 0xCF01 //深蓝色
#define RGB565_LIGHTBLUE				0x7D7C // 0x7D7C -> 0x7C7D //浅蓝色  
#define RGB565_GRAYBLUE   			0x5458 // 0x5458 -> 0x5854 //灰蓝色
//以上三色为PANEL的颜色 
#define RGB565_LIGHTGREEN			 	0x841F // 0x841F -> 0x1F84 //浅绿色
#define RGB565_LGRAY 			   	  0xC618 // 0xC618 -> 0x18C6 //浅灰色(PANNEL),窗体背景色
#define RGB565_LGRAYBLUE        0xA651 // 0xA651 -> 0x51A6 //浅灰蓝色(中间层颜色)
#define RGB565_LBBLUE           0x2B12 // 0x2B12 -> 0x122B //浅棕蓝色(选择条目的反色)

#endif