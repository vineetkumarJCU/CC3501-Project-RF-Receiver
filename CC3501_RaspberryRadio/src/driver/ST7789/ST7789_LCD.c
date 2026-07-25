#include "ST7789_LCD.h"

#include <stdio.h>

#include "board_init.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"

static int st7789_dma_channel = -1;
static volatile bool st7789_dma_busy = false;
static ST7789_FlushCompleteCallback st7789_flush_complete_callback = NULL;
static void *st7789_flush_callback_context = NULL;

static void ST7789_DMA_IRQHandler(void)
{
  if (st7789_dma_channel < 0) {
    return;
  }

  uint32_t interrupt_mask = 1u << (uint32_t)st7789_dma_channel;
  if ((dma_hw->ints1 & interrupt_mask) == 0u) {
    return;
  }

  dma_hw->ints1 = interrupt_mask;

  /* A DMA completion means that the last byte reached the SPI FIFO. Wait for
   * the FIFO and shifter to finish before a later command changes the DC pin. */
  while (spi_is_busy(ST7789_SPI_HEADER)) {
    tight_loop_contents();
  }

  st7789_dma_busy = false;

  ST7789_FlushCompleteCallback complete_callback = st7789_flush_complete_callback;
  void *callback_context = st7789_flush_callback_context;
  st7789_flush_complete_callback = NULL;
  st7789_flush_callback_context = NULL;

  if (complete_callback != NULL) {
    complete_callback(callback_context);
  }
}

static void ST7789_TX_DATA8(uint8_t dat)
{
	ST7789_Write_DC(1);//write data
	// HAL_SPI_Transmit(&ST7789_SPI_HEADER,&dat,1,1000);
  spi_write_blocking(ST7789_SPI_HEADER, &dat, 1);
}
static void ST7789_TX_DATA16(uint16_t dat)
{
	uint8_t DATA[2]={dat>>8,dat&0xff};
	ST7789_Write_DC(1);//write data
	// HAL_SPI_Transmit(&ST7789_SPI_HEADER,DATA,2,1000);
  spi_write_blocking(ST7789_SPI_HEADER,DATA,2);

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

// void ST7789_FlushArea(uint16_t x0,uint16_t y0,uint16_t x1,uint16_t y1,uint16_t *pPixel)
// {
//     uint32_t byte_remain;
//     uint32_t SetSize=((x1-x0+1)*(y1-y0+1))<<1;

//     uint8_t *pData = (uint8_t*)pPixel;

//     byte_remain=SetSize;

//     ST7789_VRAM_Access(x0,y0,x1,y1);
//     ST7789_Write_DC(1);

//     while(byte_remain)
//     {
//         uint16_t chunk = (byte_remain > 65535) ? 65535 : byte_remain;

//         // HAL_SPI_Transmit(&ST7789_SPI_HEADER, pData, chunk, 1000);
//         spi_write_blocking(ST7789_SPI_HEADER,pData,chunk);

//         pData += chunk;
//         byte_remain -= chunk;
//     }
// }

void ST7789_FlushArea(uint16_t x0, uint16_t y0,
                      uint16_t x1, uint16_t y1,
                      uint16_t *pPixel)
{
  size_t byte_count = (size_t)(x1 - x0 + 1U) *
                      (size_t)(y1 - y0 + 1U) * sizeof(*pPixel);

  ST7789_VRAM_Access(x0, y0, x1, y1);
  ST7789_Write_DC(1);

  spi_write_blocking(ST7789_SPI_HEADER,
                     (const uint8_t *)pPixel,
                     byte_count);
}

bool ST7789_DMA_Init(void)
{
  if (st7789_dma_channel >= 0) {
    return true;
  }

  int dma_channel = dma_claim_unused_channel(false);
  if (dma_channel < 0) {
    printf("[ST7789] ERROR: No DMA channel is available for SPI1 TX.\n");
    return false;
  }

  st7789_dma_channel = dma_channel;
  dma_hw->ints1 = 1u << (uint32_t)st7789_dma_channel;
  dma_channel_set_irq1_enabled((uint)st7789_dma_channel, true);
  irq_set_exclusive_handler(DMA_IRQ_1, ST7789_DMA_IRQHandler);
  irq_set_enabled(DMA_IRQ_1, true);

  printf("[ST7789] SPI1 TX DMA initialized on channel %d.\n",
         st7789_dma_channel);
  return true;
}

bool ST7789_FlushAreaDMA(uint16_t x0,
                         uint16_t y0,
                         uint16_t x1,
                         uint16_t y1,
                         const uint16_t *pPixel,
                         ST7789_FlushCompleteCallback complete_callback,
                         void *callback_context)
{
  if (st7789_dma_channel < 0) {
    printf("[ST7789] ERROR: DMA flush requested before DMA initialization.\n");
    return false;
  }

  if (st7789_dma_busy) {
    printf("[ST7789] ERROR: DMA flush requested while another flush is active.\n");
    return false;
  }

  if ((pPixel == NULL) || (x1 < x0) || (y1 < y0)) {
    printf("[ST7789] ERROR: Invalid DMA flush area or pixel buffer.\n");
    return false;
  }

  size_t byte_count = (size_t)(x1 - x0 + 1U) *
                      (size_t)(y1 - y0 + 1U) * sizeof(*pPixel);

  ST7789_VRAM_Access(x0, y0, x1, y1);
  ST7789_Write_DC(1);

  dma_channel_config config =
      dma_channel_get_default_config((uint)st7789_dma_channel);
  channel_config_set_transfer_data_size(&config, DMA_SIZE_8);
  channel_config_set_read_increment(&config, true);
  channel_config_set_write_increment(&config, false);
  channel_config_set_dreq(&config, spi_get_dreq(ST7789_SPI_HEADER, true));

  st7789_flush_complete_callback = complete_callback;
  st7789_flush_callback_context = callback_context;
  st7789_dma_busy = true;

  dma_channel_configure((uint)st7789_dma_channel,
                        &config,
                        &spi_get_hw(ST7789_SPI_HEADER)->dr,
                        (const uint8_t *)pPixel,
                        byte_count,
                        true);
  return true;
}

bool ST7789_IsFlushBusy(void)
{
  return st7789_dma_busy;
}

void ST7789_Init()
{
	ST7789_Write_RES(0);
	sleep_ms(20);
	ST7789_Write_RES(1);
  sleep_ms(120);

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
  sleep_ms(120);

  ST7789_TX_CMD(0x29);
  sleep_ms(20);
}
