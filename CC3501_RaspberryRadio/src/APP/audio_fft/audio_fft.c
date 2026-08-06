#include "audio_fft.h"

#include <stddef.h>
#include <stdint.h>
#if AUDIO_FFT_DATA_FORMAT == AUDIO_FFT_DATA_FORMAT_FLOAT32
#include <string.h>
#endif

#include "fft_window.h"
#include "fixed_log10_table.h"

#define AUDIO_FFT_HANN_WINDOW_Q_FORMAT 8U
#define AUDIO_FFT_HANN_HALF_LENGTH (AUDIO_FFT_LENGTH / 2U)
#if AUDIO_FFT_DATA_FORMAT == AUDIO_FFT_DATA_FORMAT_FLOAT32
#define AUDIO_FFT_FIXED_MAGNITUDE_FRACTION_BITS 16U
#else
#define AUDIO_FFT_FIXED_MAGNITUDE_FRACTION_BITS 8U
#endif

// Fixed-point logarithm base 10 for 20*log10(x) in Q12 format

_Static_assert(AUDIO_FFT_LENGTH == 256U,
               "hann_win_l256_sfix8_half requires a 256-point FFT");
_Static_assert((sizeof(hann_win_l256_sfix8_half) /
                sizeof(hann_win_l256_sfix8_half[0])) == AUDIO_FFT_HANN_HALF_LENGTH,
               "Hann half-window length does not match AUDIO_FFT_LENGTH");

static audio_fft_instance_t audio_fft_s;

// Compute the FFT of the loaded time-domain samples and store the results in the instance structure

#if AUDIO_FFT_DATA_FORMAT == AUDIO_FFT_DATA_FORMAT_INT8
/* CMSIS-DSP has no Q7 real FFT, so INT8 storage is promoted to Q15 for FFT. */
static q15_t int8_rfft_input[AUDIO_FFT_LENGTH];
static q15_t int8_rfft_output[AUDIO_FFT_Q15_OUTPUT_LENGTH];
static q15_t int8_magnitude[AUDIO_FFT_BIN_COUNT];
#endif

// Apply the Hann window to the time-domain samples and prepare them for FFT processing
#if AUDIO_FFT_DATA_FORMAT == AUDIO_FFT_DATA_FORMAT_Q15
static q15_t saturate_q15(int32_t value)
{
  if (value > INT16_MAX)
    return INT16_MAX;
  if (value < INT16_MIN)
    return INT16_MIN;
  return (q15_t)value;
}

#endif

// Load the ADC DMA captured samples into the FFT instance structure, centering them around zero and converting to the appropriate format
#if AUDIO_FFT_DATA_FORMAT == AUDIO_FFT_DATA_FORMAT_INT8
static int8_t saturate_int8(int32_t value)
{
  if (value > INT8_MAX)
    return INT8_MAX;
  if (value < INT8_MIN)
    return INT8_MIN;
  return (int8_t)value;
}
#endif

// Load the time-domain samples into the FFT instance structure without any conversion or centering

int audio_fft_load_samples(const adc_dma_capture_sample_t *src_data_buffer)
{
  if (src_data_buffer == NULL)
    return -1;

  for (size_t i = 0; i < AUDIO_FFT_LENGTH; i++)
  {
    int32_t centered = (int32_t)src_data_buffer[i] -
                       (int32_t)ADC_DMA_CAPTURE_SAMPLE_MIDPOINT;

#if AUDIO_FFT_DATA_FORMAT == AUDIO_FFT_DATA_FORMAT_INT8
    const uint32_t shift = ADC_DMA_CAPTURE_RESOLUTION_BITS - 8U;
    audio_fft_s.time_domain_buffer[i] = saturate_int8(centered >> shift);
#elif AUDIO_FFT_DATA_FORMAT == AUDIO_FFT_DATA_FORMAT_Q15
    audio_fft_s.time_domain_buffer[i] =
        saturate_q15(centered * (1L << ADC_DMA_CAPTURE_Q15_SHIFT));
#elif AUDIO_FFT_DATA_FORMAT == AUDIO_FFT_DATA_FORMAT_FLOAT32
    audio_fft_s.time_domain_buffer[i] =
        (float32_t)centered / (float32_t)ADC_DMA_CAPTURE_SAMPLE_MIDPOINT;
#endif
  }

  return 0;
}

// Load the time-domain samples into the FFT instance structure without any conversion or centering

int audio_fft_load_time_domain(const audio_fft_data_t *src_data_buffer)
{
  if (src_data_buffer == NULL)
    return -1;

  for (size_t i = 0; i < AUDIO_FFT_LENGTH; i++)
  {
    audio_fft_s.time_domain_buffer[i] = src_data_buffer[i];
  }

  return 0;
}

// Get the Hann window value for a given sample index in Q8 format

static uint8_t audio_fft_get_hann_q8(size_t sample_index)
{
  const size_t window_index = sample_index < AUDIO_FFT_HANN_HALF_LENGTH
                                  ? sample_index
                                  : (AUDIO_FFT_LENGTH - 1U) - sample_index;
  return hann_win_l256_sfix8_half[window_index];
}

static void audio_fft_prepare_fft_input(void)
{
  for (size_t i = 0; i < AUDIO_FFT_LENGTH; i++)
  {
    const uint8_t window_q8 = audio_fft_get_hann_q8(i);
#if AUDIO_FFT_DATA_FORMAT == AUDIO_FFT_DATA_FORMAT_INT8
    /* Q7 sample multiplied by the unsigned Q0.8 window is already Q15. */
    int8_rfft_input[i] =
        (q15_t)((int16_t)audio_fft_s.time_domain_buffer[i] *
                (int16_t)window_q8);
#elif AUDIO_FFT_DATA_FORMAT == AUDIO_FFT_DATA_FORMAT_Q15
    int32_t windowed =
        (int32_t)audio_fft_s.time_domain_buffer[i] * (int32_t)window_q8;
    audio_fft_s.time_domain_buffer[i] =
        saturate_q15(windowed >> AUDIO_FFT_HANN_WINDOW_Q_FORMAT);
#elif AUDIO_FFT_DATA_FORMAT == AUDIO_FFT_DATA_FORMAT_FLOAT32
    audio_fft_s.time_domain_buffer[i] *=
        (float32_t)window_q8 /
        (float32_t)(1U << AUDIO_FFT_HANN_WINDOW_Q_FORMAT);
#endif
  }
}

// Compute the FFT of the loaded time-domain samples and store the results in the instance structure
#if AUDIO_FFT_DATA_FORMAT == AUDIO_FFT_DATA_FORMAT_Q15
static void audio_fft_compute_q15_results(void)
{
  arm_cmplx_mag_q15(audio_fft_s.fft_result_buffer,
                    audio_fft_s.amplitude_buffer,
                    AUDIO_FFT_BIN_COUNT);

#if AUDIO_FFT_CALCULATE_PHASE != 0
  audio_fft_s.phase_buffer[0] = 0;
  audio_fft_s.phase_buffer[AUDIO_FFT_LENGTH / 2] = 0;
  for (size_t bin = 1; bin < AUDIO_FFT_LENGTH / 2; bin++)
  {
    q15_t phase = 0;
    if (arm_atan2_q15(audio_fft_s.fft_result_buffer[2U * bin + 1U],
                      audio_fft_s.fft_result_buffer[2U * bin],
                      &phase) != ARM_MATH_SUCCESS)
    {
      phase = 0;
    }
    audio_fft_s.phase_buffer[bin] = phase;
  }
#endif
}
#endif

// Compute the FFT of the loaded time-domain samples and store the results in the instance structure

#if AUDIO_FFT_DATA_FORMAT == AUDIO_FFT_DATA_FORMAT_INT8
static void audio_fft_compute_int8_results(void)
{
  arm_cmplx_mag_q15(int8_rfft_output,
                    int8_magnitude,
                    AUDIO_FFT_BIN_COUNT);

  for (size_t i = 0; i < AUDIO_FFT_Q15_OUTPUT_LENGTH; i++)
  {
    audio_fft_s.fft_result_buffer[i] = saturate_int8(int8_rfft_output[i] >> 8);
  }
  for (size_t bin = 0; bin < AUDIO_FFT_BIN_COUNT; bin++)
  {
    audio_fft_s.amplitude_buffer[bin] = saturate_int8(int8_magnitude[bin] >> 8);
  }

  // Compute the phase for each frequency bin if enabled
#if AUDIO_FFT_CALCULATE_PHASE != 0
  audio_fft_s.phase_buffer[0] = 0;
  audio_fft_s.phase_buffer[AUDIO_FFT_LENGTH / 2] = 0;
  for (size_t bin = 1; bin < AUDIO_FFT_LENGTH / 2; bin++)
  {
    q15_t phase = 0;
    if (arm_atan2_q15(int8_rfft_output[2U * bin + 1U],
                      int8_rfft_output[2U * bin],
                      &phase) != ARM_MATH_SUCCESS)
    {
      phase = 0;
    }
    audio_fft_s.phase_buffer[bin] = saturate_int8(phase >> 8);
  }
#endif
}
#endif

// Compute the FFT of the loaded time-domain samples and store the results in the instance structure
#if AUDIO_FFT_DATA_FORMAT == AUDIO_FFT_DATA_FORMAT_FLOAT32
static void audio_fft_compute_float32_results(void)
{
  float32_t dc = audio_fft_s.fft_result_buffer[0];
  float32_t nyquist = audio_fft_s.fft_result_buffer[1];
  audio_fft_s.amplitude_buffer[0] = dc < 0.0f ? -dc : dc;
  audio_fft_s.amplitude_buffer[AUDIO_FFT_LENGTH / 2] =
      nyquist < 0.0f ? -nyquist : nyquist;

  arm_cmplx_mag_f32(&audio_fft_s.fft_result_buffer[2],
                    &audio_fft_s.amplitude_buffer[1],
                    AUDIO_FFT_LENGTH / 2 - 1);

#if AUDIO_FFT_CALCULATE_PHASE != 0
  audio_fft_s.phase_buffer[0] = 0.0f;
  audio_fft_s.phase_buffer[AUDIO_FFT_LENGTH / 2] = 0.0f;
  for (size_t bin = 1; bin < AUDIO_FFT_LENGTH / 2; bin++)
  {
    float32_t phase = 0.0f;
    if (arm_atan2_f32(audio_fft_s.fft_result_buffer[2U * bin + 1U],
                      audio_fft_s.fft_result_buffer[2U * bin],
                      &phase) != ARM_MATH_SUCCESS)
    {
      phase = 0.0f;
    }
    audio_fft_s.phase_buffer[bin] = phase;
  }
#endif
}
#endif

// Find the frequency bin with the maximum magnitude and store its index and corresponding frequency in the instance structure

static void find_mag_peak(void)
{
  uint16_t peak_index = 0;
  audio_fft_data_t peak_value = 0;
  for (size_t i = 0; i < AUDIO_FFT_BIN_COUNT; i++)
  {
    if (audio_fft_s.amplitude_buffer[i] > peak_value)
    {
      peak_value = audio_fft_s.amplitude_buffer[i];
      peak_index = (uint16_t)i;
    }
  }
  audio_fft_s.mag_peak_bin_index = peak_index;
  audio_fft_s.mag_peak_freq_Hz =
      (float32_t)peak_index * AUDIO_FFT_BIN_FREQUENCY_RESOLUTION_HZ;
}

// Get the fixed-point magnitude of a given frequency bin, scaled to a Q format suitable for logarithmic conversion

static uint32_t audio_fft_get_fixed_magnitude(size_t bin)
{
#if AUDIO_FFT_DATA_FORMAT == AUDIO_FFT_DATA_FORMAT_INT8
  const q15_t magnitude = int8_magnitude[bin];
  return magnitude > 0
             ? (uint32_t)(uint16_t)magnitude
                   << AUDIO_FFT_FIXED_MAGNITUDE_FRACTION_BITS
             : 0U;
#elif AUDIO_FFT_DATA_FORMAT == AUDIO_FFT_DATA_FORMAT_Q15
  const q15_t magnitude = audio_fft_s.amplitude_buffer[bin];
  return magnitude > 0
             ? (uint32_t)(uint16_t)magnitude
                   << AUDIO_FFT_FIXED_MAGNITUDE_FRACTION_BITS
             : 0U;
#elif AUDIO_FFT_DATA_FORMAT == AUDIO_FFT_DATA_FORMAT_FLOAT32
  uint32_t bits = 0U;
  memcpy(&bits, &audio_fft_s.amplitude_buffer[bin], sizeof(bits));

  const uint32_t exponent = (bits >> 23U) & 0xffU;
  const uint32_t fraction = bits & 0x7fffffU;
  if ((bits & 0x80000000U) != 0U || exponent == 0U)
    return 0U;
  if (exponent == 0xffU)
    return fraction == 0U ? UINT32_MAX : 0U;

  const uint32_t significand = fraction | 0x800000U;
  const int32_t shift = (int32_t)exponent - 134;
  if (shift >= 0)
  {
    if (shift > 8 || significand > (UINT32_MAX >> (uint32_t)shift))
    {
      return UINT32_MAX;
    }
    return significand << (uint32_t)shift;
  }

  const uint32_t right_shift = (uint32_t)(-shift);
  if (right_shift >= 32U)
    return 0U;
  return (significand + (1U << (right_shift - 1U))) >> right_shift;
#endif
}

// Compute the logarithmic magnitude in decibels for each frequency bin and store it in the provided destination buffer, applying a floor value to avoid negative infinity
static uint32_t audio_fft_interpolate_magnitude(uint32_t lower,
                                                uint32_t upper,
                                                uint32_t numerator,
                                                uint32_t denominator)
{
  if (numerator == 0U || lower == upper)
    return lower;

  const uint32_t delta = upper > lower ? upper - lower : lower - upper;
  uint32_t offset;
  if (delta <= (UINT32_MAX - (denominator / 2U)) / numerator)
  {
    offset = (delta * numerator + denominator / 2U) / denominator;
  }
  else
  {
    offset = (uint32_t)(((uint64_t)delta * numerator + denominator / 2U) /
                        denominator);
  }

  return upper > lower ? lower + offset : lower - offset;
}

// Compute the logarithmic magnitude in decibels for each frequency bin and store it in the provided destination buffer, applying a floor value to avoid negative infinity

int audio_fft_get_log_magnitude_db(int8_t *dst_buffer,
                                   size_t dst_length,
                                   int8_t floor_db)
{
  if (dst_buffer == NULL || dst_length == 0U || floor_db >= 0)
    return -1;

  uint32_t peak_magnitude = 0U;
#if AUDIO_FFT_ENABLE_FLOATING_MAX_LIN2DB != 0
  for (size_t bin = 1; bin < AUDIO_FFT_BIN_COUNT; bin++)
  {
    const uint32_t magnitude = audio_fft_get_fixed_magnitude(bin);
    if (magnitude > peak_magnitude)
      peak_magnitude = magnitude;
  }
#else
  _Static_assert(AUDIO_FFT_FIXED_MAX_AMPL <=
                     (UINT32_MAX >> AUDIO_FFT_FIXED_MAGNITUDE_FRACTION_BITS),
                 "Fixed reference does not fit the magnitude Q format");
  peak_magnitude =
      (uint32_t)AUDIO_FFT_FIXED_MAX_AMPL
      << AUDIO_FFT_FIXED_MAGNITUDE_FRACTION_BITS;
#endif

  if (peak_magnitude == 0U)
  {
    for (size_t i = 0; i < dst_length; i++)
      dst_buffer[i] = floor_db;
    return 0;
  }

  const size_t bin_intervals = AUDIO_FFT_BIN_COUNT - 1U;
  if ((dst_length - 1U) > UINT32_MAX / bin_intervals)
    return -1;

  const uint32_t position_denominator = dst_length > 1U
                                            ? (uint32_t)(dst_length - 1U)
                                            : 1U;
  const int32_t peak_db_q12 = fixed_log10_20log_u32_q12(peak_magnitude);
  const int32_t floor_db_q12 =
      (int32_t)floor_db * (int32_t)(1U << FIXED_LOG10_DB_FRACTION_BITS);

  for (size_t i = 0; i < dst_length; i++)
  {
    const uint32_t position_numerator =
        (uint32_t)i * (uint32_t)bin_intervals;
    const size_t lower_bin = position_numerator / position_denominator;
    size_t upper_bin = lower_bin + 1U;
    if (upper_bin >= AUDIO_FFT_BIN_COUNT)
      upper_bin = lower_bin;

    const uint32_t fraction_numerator =
        position_numerator % position_denominator;
    const uint32_t lower_magnitude =
        audio_fft_get_fixed_magnitude(lower_bin);
    const uint32_t upper_magnitude =
        audio_fft_get_fixed_magnitude(upper_bin);
    const uint32_t magnitude =
        audio_fft_interpolate_magnitude(lower_magnitude,
                                        upper_magnitude,
                                        fraction_numerator,
                                        position_denominator);

    int32_t db_q12 = magnitude > 0U
                         ? fixed_log10_20log_u32_q12(magnitude) - peak_db_q12
                         : floor_db_q12;
    if (db_q12 < floor_db_q12)
      db_q12 = floor_db_q12;
    if (db_q12 > 0)
      db_q12 = 0;
    dst_buffer[i] =
        (int8_t)((db_q12 - (1L << (FIXED_LOG10_DB_FRACTION_BITS - 1U))) /
                 (1L << FIXED_LOG10_DB_FRACTION_BITS));
  }

  return 0;
}

// Compute the FFT of the loaded time-domain samples and store the results in the instance structure

int audio_fft_compute(void)
{
  audio_fft_prepare_fft_input();

#if AUDIO_FFT_DATA_FORMAT == AUDIO_FFT_DATA_FORMAT_INT8
  arm_rfft_instance_q15 rfft_instance;
  if (arm_rfft_init_256_q15(&rfft_instance, 0, 1) != ARM_MATH_SUCCESS)
    return -1;
  arm_rfft_q15(&rfft_instance, int8_rfft_input, int8_rfft_output);
  audio_fft_compute_int8_results();
#elif AUDIO_FFT_DATA_FORMAT == AUDIO_FFT_DATA_FORMAT_Q15
  arm_rfft_instance_q15 rfft_instance;
  if (arm_rfft_init_256_q15(&rfft_instance, 0, 1) != ARM_MATH_SUCCESS)
    return -1;
  arm_rfft_q15(&rfft_instance,
               audio_fft_s.time_domain_buffer,
               audio_fft_s.fft_result_buffer);
  audio_fft_compute_q15_results();
#elif AUDIO_FFT_DATA_FORMAT == AUDIO_FFT_DATA_FORMAT_FLOAT32
  arm_rfft_fast_instance_f32 rfft_instance;
  if (arm_rfft_fast_init_256_f32(&rfft_instance) != ARM_MATH_SUCCESS)
    return -1;
  arm_rfft_fast_f32(&rfft_instance,
                    audio_fft_s.time_domain_buffer,
                    audio_fft_s.fft_result_buffer,
                    0);
  audio_fft_compute_float32_results();
#endif

  find_mag_peak();
  return 0;
}

audio_fft_instance_t *audio_fft_get_data_structure(void)
{
  return &audio_fft_s;
}
