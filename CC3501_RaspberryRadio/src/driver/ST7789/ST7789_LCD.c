#include "ST7789_LCD.h"


static void ST7789_TX_DATA8(uint8_t dat)
{
	ST7789_Write_DC(1);//write data
	// HAL_SPI_Transmit(&ST7789_SPI_HEADER,&dat,1,1000);
  spi_write_blocking(ST7789_SPI_HEADER,&dat,1);
}
static void ST7789_TX_DATA16(uint16_t dat)
{
	uint8_t DATA[2]={dat>>8,dat&0xff};
	ST7789_Write_DC(1);//write data
	// HAL_SPI_Transmit(&ST7789_SPI_HEADER,DATA,2,1000);
  spi_write_blocking(ST7789_SPI_HEADER,&DATA,2);

}
static void ST7789_TX_CMD(uint8_t dat)
{
	ST7789_Write_DC(0);//write cmd
	// HAL_SPI_Transmit(&ST7789_SPI_HEADER,&dat,1,1000);
  spi_write_blocking(ST7789_SPI_HEADER,&dat,1);
}

static void ST7789_VRAM_Access(uint16_t x0,uint16_t y0,uint16_t x1,uint16_t y1)
{
	#if(SCREEN_DIRECTION==0)
	{
		ST7789_TX_CMD(0x2a);
		ST7789_TX_DATA16(x0);
		ST7789_TX_DATA16(x1);
		ST7789_TX_CMD(0x2b);
		ST7789_TX_DATA16(y0);
		ST7789_TX_DATA16(y1);
		ST7789_TX_CMD(0x2c);
	}
	#elif(SCREEN_DIRECTION==1)
	{
		ST7789_TX_CMD(0x2a);
		ST7789_TX_DATA16(x0);
		ST7789_TX_DATA16(x1);
		ST7789_TX_CMD(0x2b);
		ST7789_TX_DATA16(y0);
		ST7789_TX_DATA16(y1);
		ST7789_TX_CMD(0x2c);
	}
	#elif(SCREEN_DIRECTION==2)
	{
		ST7789_TX_CMD(0x2a);
		ST7789_TX_DATA16(x0);
		ST7789_TX_DATA16(x1);
		ST7789_TX_CMD(0x2b);
		ST7789_TX_DATA16(y0);
		ST7789_TX_DATA16(y1);
		ST7789_TX_CMD(0x2c);
	}
	#elif(SCREEN_DIRECTION==3)
	{
		ST7789_TX_CMD(0x2a);
		// ST7789_TX_DATA16(x0+80);
		// ST7789_TX_DATA16(x1+80);
		ST7789_TX_DATA16(x0);
		ST7789_TX_DATA16(x1);
		ST7789_TX_CMD(0x2b);
		ST7789_TX_DATA16(y0);
		ST7789_TX_DATA16(y1);
		ST7789_TX_CMD(0x2c);
	}
	#endif
}

void ST7789_FillDot(uint16_t x,uint16_t y,uint16_t pPixel)
{
  #if(DRAW_OUT_OF_RANGE_BEHAVIOR==1)
	if(x>=ST7789_VIRTUAL_SIZE_X)return;//x=ST7789_VIRTUAL_SIZE_X-1;
	if(y>=ST7789_VIRTUAL_SIZE_Y)return;//y=ST7789_VIRTUAL_SIZE_Y-1;
  #else
	if(x>=ST7789_VIRTUAL_SIZE_X)x=ST7789_VIRTUAL_SIZE_X-1;
	if(y>=ST7789_VIRTUAL_SIZE_Y)y=ST7789_VIRTUAL_SIZE_Y-1;
  #endif

  ST7789_VRAM_Access(x,y,x,y);
  ST7789_TX_DATA16(pPixel);

}

void ST7789_FlushArea(uint16_t x0,uint16_t y0,uint16_t x1,uint16_t y1,uint16_t *pPixel)
{
    uint32_t byte_remain;
    uint32_t SetSize=((x1-x0+1)*(y1-y0+1))<<1;

    uint8_t *pData = (uint8_t*)pPixel;

    byte_remain=SetSize;

    ST7789_VRAM_Access(x0,y0,x1,y1);
    ST7789_Write_DC(1);

    while(byte_remain)
    {
        uint16_t chunk = (byte_remain > 65535) ? 65535 : byte_remain;

        // HAL_SPI_Transmit(&ST7789_SPI_HEADER, pData, chunk, 1000);
        spi_write_blocking(ST7789_SPI_HEADER,pData,chunk);

        pData += chunk;
        byte_remain -= chunk;
    }
}


void ST7789_Init()
{
	//ST7789_Write_CS(0);
	//ST7789_Write_RES(0);
	sleep_ms(1);
	//ST7789_Write_RES(1);
	
	ST7789_TX_CMD(0x36);
	if(SCREEN_DIRECTION==0)
		ST7789_TX_DATA8(0x00);
	else if(SCREEN_DIRECTION==1)
		ST7789_TX_DATA8(0xc0);
	else if(SCREEN_DIRECTION==2)
		ST7789_TX_DATA8(0x70);
	else if(SCREEN_DIRECTION==3)
		ST7789_TX_DATA8(0xa0);
	
	ST7789_TX_CMD(0x3A);
	ST7789_TX_DATA8(0x05);

	ST7789_TX_CMD(0xB2);
	ST7789_TX_DATA8(0x0C);
	ST7789_TX_DATA8(0x0C);
	ST7789_TX_DATA8(0x00);
	ST7789_TX_DATA8(0x33);
	ST7789_TX_DATA8(0x33); 

	ST7789_TX_CMD(0xB7); 
	ST7789_TX_DATA8(0x35);  

	ST7789_TX_CMD(0xBB);
	ST7789_TX_DATA8(0x19);

	ST7789_TX_CMD(0xC0);
	ST7789_TX_DATA8(0x2C);

	ST7789_TX_CMD(0xC2);
	ST7789_TX_DATA8(0x01);

	ST7789_TX_CMD(0xC3);
	ST7789_TX_DATA8(0x12);   

	ST7789_TX_CMD(0xC4);
	ST7789_TX_DATA8(0x20);  

	ST7789_TX_CMD(0xC6); 
	ST7789_TX_DATA8(0x0F);    

	ST7789_TX_CMD(0xD0); 
	ST7789_TX_DATA8(0xA4);
	ST7789_TX_DATA8(0xA1);

	ST7789_TX_CMD(0xE0);
	ST7789_TX_DATA8(0xD0);
	ST7789_TX_DATA8(0x04);
	ST7789_TX_DATA8(0x0D);
	ST7789_TX_DATA8(0x11);
	ST7789_TX_DATA8(0x13);
	ST7789_TX_DATA8(0x2B);
	ST7789_TX_DATA8(0x3F);
	ST7789_TX_DATA8(0x54);
	ST7789_TX_DATA8(0x4C);
	ST7789_TX_DATA8(0x18);
	ST7789_TX_DATA8(0x0D);
	ST7789_TX_DATA8(0x0B);
	ST7789_TX_DATA8(0x1F);
	ST7789_TX_DATA8(0x23);

	ST7789_TX_CMD(0xE1);
	ST7789_TX_DATA8(0xD0);
	ST7789_TX_DATA8(0x04);
	ST7789_TX_DATA8(0x0C);
	ST7789_TX_DATA8(0x11);
	ST7789_TX_DATA8(0x13);
	ST7789_TX_DATA8(0x2C);
	ST7789_TX_DATA8(0x3F);
	ST7789_TX_DATA8(0x44);
	ST7789_TX_DATA8(0x51);
	ST7789_TX_DATA8(0x2F);
	ST7789_TX_DATA8(0x1F);
	ST7789_TX_DATA8(0x1F);
	ST7789_TX_DATA8(0x20);
	ST7789_TX_DATA8(0x23);

	ST7789_TX_CMD(0x21); 

	ST7789_TX_CMD(0x11); 
	//Delay (120); 

	ST7789_TX_CMD(0x29); 

	// ST7789_Write_BLK(1);
}

