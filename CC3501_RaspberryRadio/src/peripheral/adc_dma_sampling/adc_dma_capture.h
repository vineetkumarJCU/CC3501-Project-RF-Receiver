#ifndef ADC_DMA_CAPTURE_H
#define ADC_DMA_CAPTURE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "bsp_board.h"

#define ADC_DMA_CAPTURE_SAMPLE_COUNT 1024U
#define ADC_DMA_CAPTURE_SAMPLE_RATE_HZ 44100U
#define ADC_DMA_CAPTURE_DEFAULT_ADC_INPUT MICROPHONE_ADC_INPUT_PIN
#define ADC_DMA_CAPTURE_DEFAULT_GPIO MICROPHONE_GPIO_PIN

#ifndef ADC_DMA_CAPTURE_RESOLUTION_BITS
#define ADC_DMA_CAPTURE_RESOLUTION_BITS 8u
#endif

#if ADC_DMA_CAPTURE_RESOLUTION_BITS == 8u
typedef uint8_t adc_dma_capture_sample_t;
#define ADC_DMA_CAPTURE_SAMPLE_MIDPOINT 128
#define ADC_DMA_CAPTURE_Q15_SHIFT 8
#elif ADC_DMA_CAPTURE_RESOLUTION_BITS == 12u
typedef uint16_t adc_dma_capture_sample_t;
#define ADC_DMA_CAPTURE_SAMPLE_MIDPOINT 2048
#define ADC_DMA_CAPTURE_Q15_SHIFT 4
#else
#error "ADC_DMA_CAPTURE_RESOLUTION_BITS must be 8 or 12"
#endif

#if ADC_DMA_CAPTURE_RESOLUTION_BITS == 8u
#define ADC_DMA_CAPTURE_DMA_TRANSFER_SIZE DMA_SIZE_8
#define ADC_DMA_CAPTURE_FIFO_BYTE_SHIFT true
#elif ADC_DMA_CAPTURE_RESOLUTION_BITS == 12u
#define ADC_DMA_CAPTURE_DMA_TRANSFER_SIZE DMA_SIZE_16
#define ADC_DMA_CAPTURE_FIFO_BYTE_SHIFT false
#endif

typedef void (*adc_dma_capture_done_callback_t)(void);

bool adc_dma_capture_start_with_callback(adc_dma_capture_done_callback_t callback);
bool adc_dma_capture_is_busy(void);
bool adc_dma_capture_is_done(void);
void adc_dma_capture_stop(void);
adc_dma_capture_sample_t *adc_dma_capture_get_samples(void);

#endif
