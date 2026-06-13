#include "audio_fft.h"
#include "fft_window.h"

#include <stddef.h>

#define AUDIO_FFT_HANN_WINDOW_Q_FORMAT 11

static audio_fft_instance_t audio_fft_s;

static int16_t saturate_q15(int32_t value)
{
    if (value > INT16_MAX) {
        return INT16_MAX;
    }
    if (value < INT16_MIN) {
        return INT16_MIN;
    }
    return (int16_t)value;
}

static int16_t abs_q15(int16_t value)
{
    if (value == INT16_MIN) {
        return INT16_MAX;
    }
    return value < 0 ? (int16_t)-value : value;
}

int audio_fft_load_samples(const int16_t *src_data_buffer)
{
    if (src_data_buffer == NULL) {
        return -1;
    }

    for (size_t i = 0; i < AUDIO_FFT_LENGTH; i++) {
        audio_fft_s.time_domain_buffer[i] = src_data_buffer[i];
    }

    return 0;
}

static int audio_fft_window_samples(void)
{
    for (size_t i = 0; i < AUDIO_FFT_LENGTH; i++) {
        int32_t windowed_sample = (int32_t)audio_fft_s.time_domain_buffer[i] *
                                  (int32_t)hann_win_l1024_sfix12[i];
        audio_fft_s.time_domain_buffer[i] =
            saturate_q15(windowed_sample >> AUDIO_FFT_HANN_WINDOW_Q_FORMAT);
    }

    return 0;
}

static void audio_fft_compute_phase(void)
{
    audio_fft_s.phase_buffer[0] = 0;
    audio_fft_s.phase_buffer[AUDIO_FFT_LENGTH / 2] = 0;

    for (size_t bin = 1; bin < AUDIO_FFT_LENGTH / 2; bin++) {
        q15_t phase = 0;
        q15_t real = audio_fft_s.fft_result_buffer[2u * bin];
        q15_t imag = audio_fft_s.fft_result_buffer[2u * bin + 1u];

        if (arm_atan2_q15(imag, real, &phase) != ARM_MATH_SUCCESS) {
            phase = 0;
        }

        audio_fft_s.phase_buffer[bin] = phase;
    }
}

static void audio_fft_compute_magnitude(void)
{
    audio_fft_s.amplitude_buffer[0] = abs_q15(audio_fft_s.fft_result_buffer[0]);
    audio_fft_s.amplitude_buffer[AUDIO_FFT_LENGTH / 2] = abs_q15(audio_fft_s.fft_result_buffer[1]);

    arm_cmplx_mag_q15(
        &audio_fft_s.fft_result_buffer[2],
        &audio_fft_s.amplitude_buffer[1],
        AUDIO_FFT_LENGTH / 2 - 1);
}

static void find_mag_peak(void)
{
    uint16_t peak_index = 0;
    int16_t peak_value = 0;
    for (size_t i = 0; i < AUDIO_FFT_BIN_COUNT; i++) {
        if (audio_fft_s.amplitude_buffer[i] > peak_value) {
            peak_value = audio_fft_s.amplitude_buffer[i];
            peak_index = (uint16_t)i;
        }
    }
    audio_fft_s.mag_peak_bin_index = peak_index;
    audio_fft_s.mag_peak_freq_Hz = (float32_t)peak_index * AUDIO_FFT_BIN_FREQUENCY_RESOLUTION_HZ;
}

int audio_fft_compute(void)
{
    arm_rfft_instance_q15 rfft_instance;
    if (arm_rfft_init_1024_q15(&rfft_instance, 0, 1) != ARM_MATH_SUCCESS) {
        return -1;
    }

    audio_fft_window_samples();

    arm_rfft_q15(&rfft_instance, audio_fft_s.time_domain_buffer, audio_fft_s.fft_result_buffer);

    audio_fft_compute_magnitude();
    audio_fft_compute_phase();
    find_mag_peak();
    return 0;
}

audio_fft_instance_t* audio_fft_get_data_structure(void)
{
    return &audio_fft_s;
}
