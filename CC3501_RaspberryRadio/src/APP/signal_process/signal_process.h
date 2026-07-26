#ifndef SIGNAL_PROCESS_H
#define SIGNAL_PROCESS_H

#include <stdint.h>

#include "adc_dma_capture.h"

#define SIGNAL_PROCESS_SAMPLE_COUNT ADC_DMA_CAPTURE_SAMPLE_COUNT

typedef struct
{
    int8_t time_domain[SIGNAL_PROCESS_SAMPLE_COUNT];
    int8_t log_magnitude_db[SIGNAL_PROCESS_SAMPLE_COUNT];
} signal_process_result_t;

/**
 * Convert one ADC frame to signed, DC-free samples and a logarithmic spectrum.
 * The spectrum is linearly spaced from 0 Hz to half the ADC sample rate.
 */
int signal_process_run(const adc_dma_capture_sample_t *adc_samples,
                       int8_t spectrum_floor_db,
                       signal_process_result_t *result);

#endif /* SIGNAL_PROCESS_H */
