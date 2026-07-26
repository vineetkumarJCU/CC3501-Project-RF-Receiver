#ifndef __AUDIO_FFT_H__
#define __AUDIO_FFT_H__

#include <stddef.h>
#include <stdint.h>

#include "adc_dma_capture.h"
#include "arm_math.h"

#define AUDIO_FFT_LENGTH 256
#define AUDIO_FFT_BIN_COUNT (AUDIO_FFT_LENGTH / 2 + 1)
#define AUDIO_FFT_Q15_OUTPUT_LENGTH (2U * AUDIO_FFT_LENGTH)

#define AUDIO_FFT_CALCULATE_PHASE 0
#define AUDIO_FFT_ENABLE_FLOATING_MAX_LIN2DB 0

#if AUDIO_FFT_ENABLE_FLOATING_MAX_LIN2DB == 0
#define AUDIO_FFT_FIXED_MAX_AMPL (2048 - 256)
#endif

#define AUDIO_FFT_DATA_FORMAT_INT8 1
#define AUDIO_FFT_DATA_FORMAT_Q15 2
#define AUDIO_FFT_DATA_FORMAT_FLOAT32 3

/* Select the FFT storage and calculation format here or from the build system. */
#ifndef AUDIO_FFT_DATA_FORMAT
#define AUDIO_FFT_DATA_FORMAT AUDIO_FFT_DATA_FORMAT_INT8
#endif

#if AUDIO_FFT_DATA_FORMAT == AUDIO_FFT_DATA_FORMAT_INT8
typedef int8_t audio_fft_data_t;
#elif AUDIO_FFT_DATA_FORMAT == AUDIO_FFT_DATA_FORMAT_Q15
typedef int16_t audio_fft_data_t;
#elif AUDIO_FFT_DATA_FORMAT == AUDIO_FFT_DATA_FORMAT_FLOAT32
typedef float32_t audio_fft_data_t;
#else
#error "AUDIO_FFT_DATA_FORMAT must be INT8, Q15, or FLOAT32"
#endif

#define AUDIO_FFT_SAMPLE_RATE_HZ ADC_DMA_CAPTURE_SAMPLE_RATE_HZ
#define AUDIO_FFT_BIN_FREQUENCY_RESOLUTION_HZ ((float)AUDIO_FFT_SAMPLE_RATE_HZ / (float)AUDIO_FFT_LENGTH)

typedef struct
{
  audio_fft_data_t time_domain_buffer[AUDIO_FFT_LENGTH];
  audio_fft_data_t fft_result_buffer[AUDIO_FFT_Q15_OUTPUT_LENGTH];
  audio_fft_data_t amplitude_buffer[AUDIO_FFT_BIN_COUNT];
#if AUDIO_FFT_CALCULATE_PHASE != 0
  audio_fft_data_t phase_buffer[AUDIO_FFT_BIN_COUNT];
#endif
  uint16_t mag_peak_bin_index;
  float32_t mag_peak_freq_Hz;
} audio_fft_instance_t;

/* Convert unsigned ADC DMA samples into the selected signed/float FFT format. */
int audio_fft_load_samples(const adc_dma_capture_sample_t *src_data_buffer);
int audio_fft_load_time_domain(const audio_fft_data_t *src_data_buffer);
int audio_fft_compute(void);
int audio_fft_get_log_magnitude_db(int8_t *dst_buffer,
                                   size_t dst_length,
                                   int8_t floor_db);
audio_fft_instance_t *audio_fft_get_data_structure(void);

#endif // __AUDIO_FFT_H__
