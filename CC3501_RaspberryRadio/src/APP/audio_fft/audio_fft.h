#ifndef __AUDIO_FFT_H__
#define __AUDIO_FFT_H__

#include "arm_math.h"
#include "adc_dma_capture.h"

#define AUDIO_FFT_LENGTH 1024
#define AUDIO_FFT_BIN_COUNT (AUDIO_FFT_LENGTH / 2 + 1)

#define AUDIO_FFT_SAMPLE_RATE_HZ ADC_DMA_CAPTURE_SAMPLE_RATE_HZ
#define AUDIO_FFT_BIN_FREQUENCY_RESOLUTION_HZ ((float)AUDIO_FFT_SAMPLE_RATE_HZ / (float)AUDIO_FFT_LENGTH)

typedef struct {
    int16_t time_domain_buffer[AUDIO_FFT_LENGTH];
    int16_t fft_result_buffer[AUDIO_FFT_LENGTH];
    int16_t amplitude_buffer[AUDIO_FFT_BIN_COUNT];
    int16_t phase_buffer[AUDIO_FFT_BIN_COUNT];
    uint16_t mag_peak_bin_index;
    float32_t mag_peak_freq_Hz;
} audio_fft_instance_t;


int audio_fft_load_samples(const int16_t *src_data_buffer);
int audio_fft_compute(void);
audio_fft_instance_t* audio_fft_get_data_structure(void);

#endif // __AUDIO_FFT_H__
