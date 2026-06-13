#include "adc_dma_capture.h"

#include "hardware/adc.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/irq.h"

static int adc_dma_channel = -1;
static volatile bool adc_dma_busy;
static volatile bool adc_dma_done;
static adc_dma_capture_done_callback_t adc_dma_done_callback;
static adc_dma_capture_sample_t adc_samples_buffer[ADC_DMA_CAPTURE_SAMPLE_COUNT];

static void adc_dma_capture_irq_handler(void)
{
    if (adc_dma_channel < 0) {
        return;
    }

    const uint32_t channel_mask = 1u << adc_dma_channel;
    if ((dma_hw->ints0 & channel_mask) == 0) {
        return;
    }

    dma_hw->ints0 = channel_mask;
    adc_run(false);
    adc_fifo_drain();

    adc_dma_busy = false;
    adc_dma_done = true;

    if (adc_dma_done_callback != NULL) {
        adc_dma_done_callback();
    }
}

static void adc_dma_capture_init_once(void)
{
    static bool initialized;

    if (initialized) {
        return;
    }

    adc_init();
    adc_gpio_init(ADC_DMA_CAPTURE_DEFAULT_GPIO);
    adc_select_input(ADC_DMA_CAPTURE_DEFAULT_ADC_INPUT);

    adc_dma_channel = dma_claim_unused_channel(true);
    irq_set_exclusive_handler(DMA_IRQ_0, adc_dma_capture_irq_handler);
    irq_set_enabled(DMA_IRQ_0, true);

    initialized = true;
}

bool adc_dma_capture_start_with_callback(adc_dma_capture_done_callback_t callback)
{
    if (adc_dma_busy) {
        return false;
    }

    adc_dma_capture_init_once();

    adc_dma_done_callback = callback;
    adc_dma_done = false;
    adc_dma_busy = true;

    adc_run(false);
    adc_fifo_drain();

    const uint32_t adc_clock_hz = clock_get_hz(clk_adc);
    const float adc_clkdiv = ((float)adc_clock_hz / (float)ADC_DMA_CAPTURE_SAMPLE_RATE_HZ) - 1.0f;
    adc_set_clkdiv(adc_clkdiv);

    adc_fifo_setup(
        true,
        true,
        1,
        false,
        ADC_DMA_CAPTURE_FIFO_BYTE_SHIFT);

    dma_channel_config config = dma_channel_get_default_config(adc_dma_channel);
    channel_config_set_transfer_data_size(&config, ADC_DMA_CAPTURE_DMA_TRANSFER_SIZE);
    channel_config_set_read_increment(&config, false);
    channel_config_set_write_increment(&config, true);
    channel_config_set_dreq(&config, DREQ_ADC);

    dma_channel_configure(
        adc_dma_channel,
        &config,
        adc_samples_buffer,
        &adc_hw->fifo,
        ADC_DMA_CAPTURE_SAMPLE_COUNT,
        false);

    dma_hw->ints0 = 1u << adc_dma_channel;
    dma_channel_set_irq0_enabled(adc_dma_channel, true);
    dma_channel_start(adc_dma_channel);
    adc_run(true);

    return true;
}

bool adc_dma_capture_is_busy(void)
{
    return adc_dma_busy;
}

bool adc_dma_capture_is_done(void)
{
    return adc_dma_done;
}

void adc_dma_capture_stop(void)
{
    if (adc_dma_channel >= 0) {
        dma_channel_abort(adc_dma_channel);
        dma_hw->ints0 = 1u << adc_dma_channel;
    }

    adc_run(false);
    adc_fifo_drain();
    adc_dma_busy = false;
}

adc_dma_capture_sample_t *adc_dma_capture_get_samples(void)
{
    return adc_samples_buffer;
}