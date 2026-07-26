#include "signal_process.h"

#include <stddef.h>
#include <stdint.h>

#include "audio_fft.h"

#if AUDIO_FFT_DATA_FORMAT != AUDIO_FFT_DATA_FORMAT_INT8
#error "signal_process requires AUDIO_FFT_DATA_FORMAT_INT8"
#endif

_Static_assert(SIGNAL_PROCESS_SAMPLE_COUNT == AUDIO_FFT_LENGTH,
               "ADC capture and FFT lengths must match");

static int8_t saturate_int8(int32_t value)
{
    if (value > INT8_MAX) return INT8_MAX;
    if (value < INT8_MIN) return INT8_MIN;
    return (int8_t)value;
} 

static void fill_spectrum(int8_t *spectrum, int8_t value)
{
    for (size_t i = 0; i < SIGNAL_PROCESS_SAMPLE_COUNT; i++) {
        spectrum[i] = value;
    }
}

int signal_process_run(const adc_dma_capture_sample_t *adc_samples,
                       int8_t spectrum_floor_db,
                       signal_process_result_t *result)
{
    if (adc_samples == NULL || result == NULL || spectrum_floor_db >= 0) return -1;

    int32_t sample_sum = 0;
    for (size_t i = 0; i < SIGNAL_PROCESS_SAMPLE_COUNT; i++) {
        int32_t centered = (int32_t)adc_samples[i] -
                           (int32_t)ADC_DMA_CAPTURE_SAMPLE_MIDPOINT;
#if ADC_DMA_CAPTURE_RESOLUTION_BITS > 8U
        centered >>= ADC_DMA_CAPTURE_RESOLUTION_BITS - 8U;
#endif
        result->time_domain[i] = saturate_int8(centered);
        sample_sum += result->time_domain[i];
    }

    int32_t dc_offset = sample_sum / (int32_t)SIGNAL_PROCESS_SAMPLE_COUNT;
    for (size_t i = 0; i < SIGNAL_PROCESS_SAMPLE_COUNT; i++) {
        result->time_domain[i] =
            saturate_int8((int32_t)result->time_domain[i] - dc_offset);
    }

    if (audio_fft_load_time_domain(result->time_domain) != 0 ||
        audio_fft_compute() != 0 ||
        audio_fft_get_log_magnitude_db(result->log_magnitude_db,
                                       SIGNAL_PROCESS_SAMPLE_COUNT,
                                       spectrum_floor_db) != 0) {
        fill_spectrum(result->log_magnitude_db, spectrum_floor_db);
        return -1;
    }

    return 0;
}
