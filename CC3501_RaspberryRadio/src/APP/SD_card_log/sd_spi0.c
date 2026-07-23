#include "sd_spi0.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#ifdef SD_SPI0_HOST_TEST
extern void sd_spi0_host_setup(void);
extern void sd_spi0_host_set_baudrate(uint32_t baudrate_hz);
extern void sd_spi0_host_set_cs(bool asserted);
extern bool sd_spi0_host_is_busy(void);
extern uint64_t sd_spi0_host_time_us(void);
extern uint8_t sd_spi0_host_transfer(uint8_t tx);
#else
#include "board_init.h"
#include "board_pin_def.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "pico/time.h"
#endif



typedef struct {
    bool initialized;
    bool high_capacity;
    uint8_t card_type;
    uint32_t sector_count;
    uint8_t ocr[4];
} sd_spi0_state_t;

static sd_spi0_state_t sd_state;

static void sd_spi0_platform_setup(void) {
#ifdef SD_SPI0_HOST_TEST
    sd_spi0_host_setup();
#else
    /* board_init() owns the SPI/pin setup. Keep the software CS inactive and
     * temporarily lower only the SPI baud rate during the SD idle sequence. */
    gpio_put(SD_CARD_SPI0_CS_PIN, 1);
    gpio_set_dir(SD_CARD_SPI0_CS_PIN, GPIO_OUT);
    spi_set_format(SD_CARD_SPI_HEADER, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
#endif
}

static void sd_spi0_set_baudrate(uint32_t baudrate_hz) {
#ifdef SD_SPI0_HOST_TEST
    sd_spi0_host_set_baudrate(baudrate_hz);
#else
    spi_set_baudrate(SD_CARD_SPI_HEADER, baudrate_hz);
#endif
}

static uint64_t sd_spi0_time_us(void) {
#ifdef SD_SPI0_HOST_TEST
    return sd_spi0_host_time_us();
#else
    return time_us_64();
#endif
}

static bool sd_spi0_is_busy(void) {
#ifdef SD_SPI0_HOST_TEST
    return sd_spi0_host_is_busy();
#else
    return spi_is_busy(SD_CARD_SPI_HEADER);
#endif
}

static uint8_t sd_spi0_transfer(uint8_t tx) {
#ifdef SD_SPI0_HOST_TEST
    return sd_spi0_host_transfer(tx);
#else
    uint8_t rx;
    (void)spi_write_read_blocking(SD_CARD_SPI_HEADER, &tx, &rx, 1);
    return rx;
#endif
}

static void sd_spi0_transaction_begin(void) {
#ifdef SD_SPI0_HOST_TEST
    sd_spi0_host_set_cs(true);
#else
    gpio_put(SD_CARD_SPI0_CS_PIN, 0);
#endif
}

static void sd_spi0_transaction_end(void) {
    while (sd_spi0_is_busy()) {
    }
#ifdef SD_SPI0_HOST_TEST
    sd_spi0_host_set_cs(false);
#else
    gpio_put(SD_CARD_SPI0_CS_PIN, 1);
#endif
    (void)sd_spi0_transfer(0xff);
}

static bool sd_spi0_timed_out(uint64_t start, uint32_t timeout_us) {
    return (sd_spi0_time_us() - start) >= timeout_us;
}

static uint8_t sd_spi0_wait_r1(void) {
    uint64_t start = sd_spi0_time_us();
    uint8_t response;
    do {
        response = sd_spi0_transfer(0xff);
        if ((response & 0x80u) == 0u) {
            return response;
        }
    } while (!sd_spi0_timed_out(start, SD_SPI0_CMD_TIMEOUT_US));
    return 0xff;
}

static uint8_t sd_spi0_command_open(uint8_t command, uint32_t argument) {
    sd_spi0_transaction_begin();
    (void)sd_spi0_transfer((uint8_t)(0x40u | command));
    (void)sd_spi0_transfer((uint8_t)(argument >> 24));
    (void)sd_spi0_transfer((uint8_t)(argument >> 16));
    (void)sd_spi0_transfer((uint8_t)(argument >> 8));
    (void)sd_spi0_transfer((uint8_t)argument);
    (void)sd_spi0_transfer(command == 0u ? 0x95u : (command == 8u ? 0x87u : 0x01u));
    return sd_spi0_wait_r1();
}

static sd_spi0_result_t sd_spi0_command(uint8_t command, uint32_t argument, uint8_t expected_r1) {
    uint8_t response = sd_spi0_command_open(command, argument);
    sd_spi0_transaction_end();
    if (response == 0xffu) {
        return SD_SPI0_ERR_TIMEOUT;
    }
    return response == expected_r1 ? SD_SPI0_OK : SD_SPI0_ERR_RESPONSE;
}

static sd_spi0_result_t sd_spi0_wait_data_token(uint8_t expected) {
    uint64_t start = sd_spi0_time_us();
    uint8_t token;
    do {
        token = sd_spi0_transfer(0xff);
        if (token == expected) {
            return SD_SPI0_OK;
        }
        if ((token & 0xf0u) == 0u || (token >= 0xf1u && token <= 0xfeu)) {
            return SD_SPI0_ERR_RESPONSE;
        }
    } while (!sd_spi0_timed_out(start, SD_SPI0_DATA_TIMEOUT_US));
    return SD_SPI0_ERR_TIMEOUT;
}

static sd_spi0_result_t sd_spi0_read_data(uint8_t *buffer, uint32_t length) {
    sd_spi0_result_t result = sd_spi0_wait_data_token(SD_SPI0_TOKEN_START_BLOCK);
    if (result != SD_SPI0_OK) {
        return result;
    }
    for (uint32_t i = 0; i < length; ++i) {
        buffer[i] = sd_spi0_transfer(0xff);
    }
    (void)sd_spi0_transfer(0xff);
    (void)sd_spi0_transfer(0xff);
    return SD_SPI0_OK;
}

static sd_spi0_result_t sd_spi0_read_register(uint8_t command, uint8_t output[16]) {
    uint8_t response = sd_spi0_command_open(command, 0);
    sd_spi0_result_t result = response == 0u ? sd_spi0_read_data(output, 16) : SD_SPI0_ERR_RESPONSE;
    sd_spi0_transaction_end();
    return result;
}

static sd_spi0_result_t sd_spi0_parse_capacity(const uint8_t csd[16]) {
    uint64_t sectors;
    if ((csd[0] >> 6) == 1u) {
        uint32_t c_size = ((uint32_t)(csd[7] & 0x3fu) << 16)
                        | ((uint32_t)csd[8] << 8) | csd[9];
        sectors = ((uint64_t)c_size + 1u) * 1024u;
    } else if ((csd[0] >> 6) == 0u) {
        uint32_t c_size = ((uint32_t)(csd[6] & 0x03u) << 10)
                        | ((uint32_t)csd[7] << 2) | (csd[8] >> 6);
        uint32_t c_size_mult = ((uint32_t)(csd[9] & 0x03u) << 1) | (csd[10] >> 7);
        uint32_t read_bl_len = csd[5] & 0x0fu;
        if (read_bl_len < 9u) {
            return SD_SPI0_ERR_UNSUPPORTED;
        }
        sectors = ((uint64_t)c_size + 1u) << (c_size_mult + 2u + read_bl_len - 9u);
    } else {
        return SD_SPI0_ERR_UNSUPPORTED;
    }
    if (sectors == 0u || sectors > UINT32_MAX) {
        return SD_SPI0_ERR_UNSUPPORTED;
    }
    sd_state.sector_count = (uint32_t)sectors;
    return SD_SPI0_OK;
}

static sd_spi0_result_t sd_spi0_address(uint32_t lba, uint32_t *argument) {
    if (sd_state.high_capacity) {
        *argument = lba;
        return SD_SPI0_OK;
    }
    if (lba > UINT32_MAX / SD_SPI0_SECTOR_SIZE) {
        return SD_SPI0_ERR_UNSUPPORTED;
    }
    *argument = lba * SD_SPI0_SECTOR_SIZE;
    return SD_SPI0_OK;
}

sd_spi0_result_t sd_spi0_init(void) {
    uint8_t response;
    uint8_t csd[16];
    uint64_t start;

    memset(&sd_state, 0, sizeof(sd_state));
    if (!sd_spi0_card_is_inserted()) {
        printf("[SD] Initialization aborted: no card detected\n");
        return SD_SPI0_ERR_NO_CARD;
    }

    printf("[SD] Initializing card in SPI mode\n");
    sd_spi0_platform_setup();
    sd_spi0_set_baudrate(SD_SPI0_INIT_BAUDRATE_HZ);
    for (unsigned int i = 0; i < 10u; ++i) {
        (void)sd_spi0_transfer(0xff);
    }

    sd_spi0_result_t init_result = sd_spi0_command(0, 0, SD_SPI0_R1_IDLE_STATE);
    if (init_result != SD_SPI0_OK) {
        printf("[SD] CMD0 failed: %s\n", sd_spi0_result_string(init_result));
        return init_result == SD_SPI0_ERR_TIMEOUT ? SD_SPI0_ERR_NO_CARD : init_result;
    }

    response = sd_spi0_command_open(8, 0x1aau);
    if (response == SD_SPI0_R1_IDLE_STATE) {
        uint8_t r7[4];
        for (unsigned int i = 0; i < sizeof(r7); ++i) {
            r7[i] = sd_spi0_transfer(0xff);
        }
        sd_spi0_transaction_end();
        if (r7[2] != 0x01u || r7[3] != 0xaau) {
            printf("[SD] CMD8 returned an unsupported voltage/check pattern\n");
            return SD_SPI0_ERR_UNSUPPORTED;
        }
        sd_state.card_type = 2u;
    } else if ((response & SD_SPI0_R1_ILLEGAL_COMMAND) != 0u) {
        sd_spi0_transaction_end();
        sd_state.card_type = 1u;
    } else {
        sd_spi0_transaction_end();
        printf("[SD] CMD8 failed with R1=0x%02x\n", response);
        return response == 0xffu ? SD_SPI0_ERR_TIMEOUT : SD_SPI0_ERR_RESPONSE;
    }

    start = sd_spi0_time_us();
    do {
        sd_spi0_result_t result = sd_spi0_command(55, 0, SD_SPI0_R1_IDLE_STATE);
        if (result != SD_SPI0_OK) {
            printf("[SD] CMD55 failed: %s\n", sd_spi0_result_string(result));
            return result;
        }
        result = sd_spi0_command(41, sd_state.card_type == 2u ? 0x40000000u : 0u, 0u);
        if (result == SD_SPI0_OK) {
            break;
        }
        if (result != SD_SPI0_ERR_RESPONSE || sd_spi0_timed_out(start, SD_SPI0_INIT_TIMEOUT_US)) {
            printf("[SD] ACMD41 failed: %s\n", sd_spi0_result_string(result));
            return result == SD_SPI0_ERR_RESPONSE ? SD_SPI0_ERR_TIMEOUT : result;
        }
    } while (true);

    response = sd_spi0_command_open(58, 0);
    if (response == 0u) {
        for (unsigned int i = 0; i < sizeof(sd_state.ocr); ++i) {
            sd_state.ocr[i] = sd_spi0_transfer(0xff);
        }
    }
    sd_spi0_transaction_end();
    if (response != 0u) {
        printf("[SD] CMD58 failed with R1=0x%02x\n", response);
        return response == 0xffu ? SD_SPI0_ERR_TIMEOUT : SD_SPI0_ERR_RESPONSE;
    }
    sd_state.high_capacity = (sd_state.ocr[0] & 0x40u) != 0u;

    if (!sd_state.high_capacity && sd_spi0_command(16, SD_SPI0_SECTOR_SIZE, 0u) != SD_SPI0_OK) {
        printf("[SD] CMD16 failed while selecting 512-byte blocks\n");
        return SD_SPI0_ERR_RESPONSE;
    }
    if (sd_spi0_read_register(9, csd) != SD_SPI0_OK) {
        printf("[SD] CMD9 failed while reading CSD\n");
        return SD_SPI0_ERR_RESPONSE;
    }
    if (sd_spi0_parse_capacity(csd) != SD_SPI0_OK) {
        printf("[SD] Unsupported or invalid capacity in CSD\n");
        return SD_SPI0_ERR_UNSUPPORTED;
    }
    sd_state.initialized = true;
    sd_spi0_set_baudrate(SD_SPI0_TRANSFER_BAUDRATE_HZ);
    printf("[SD] Card initialized: type=%s, sectors=%lu, SPI=%lu Hz\n",
           sd_state.high_capacity ? "SDHC/SDXC" : "SDSC",
           (unsigned long)sd_state.sector_count,
           (unsigned long)SD_SPI0_TRANSFER_BAUDRATE_HZ);
    return SD_SPI0_OK;
}

void sd_spi0_deinit(void) {
    sd_state.initialized = false;
#ifndef SD_SPI0_HOST_TEST
    gpio_put(SD_CARD_SPI0_CS_PIN, 1);
#endif
}

bool sd_spi0_is_initialized(void) {
    return sd_state.initialized;
}

bool sd_spi0_card_is_inserted(void) {
#ifdef SD_SPI0_HOST_TEST
    return true;
#else
    return gpio_get(SD_CARD_DETECT_PIN) == 0u;
#endif
}

sd_spi0_result_t sd_spi0_read_blocks(uint8_t *buffer, uint32_t lba, uint32_t count) {
    if (!sd_state.initialized || !sd_spi0_card_is_inserted() || buffer == NULL || count == 0u) {
        return SD_SPI0_ERR_NO_CARD;
    }
    while (count-- != 0u) {
        uint32_t argument;
        sd_spi0_result_t result = sd_spi0_address(lba++, &argument);
        if (result != SD_SPI0_OK) {
            return result;
        }
        uint8_t response = sd_spi0_command_open(17, argument);
        result = response == 0u ? sd_spi0_read_data(buffer, SD_SPI0_SECTOR_SIZE) : SD_SPI0_ERR_RESPONSE;
        sd_spi0_transaction_end();
        if (result != SD_SPI0_OK) {
            return result;
        }
        buffer += SD_SPI0_SECTOR_SIZE;
    }
    return SD_SPI0_OK;
}

sd_spi0_result_t sd_spi0_write_blocks(const uint8_t *buffer, uint32_t lba, uint32_t count) {
    if (!sd_state.initialized || !sd_spi0_card_is_inserted() || buffer == NULL || count == 0u) {
        return SD_SPI0_ERR_NO_CARD;
    }
    while (count-- != 0u) {
        uint32_t argument;
        sd_spi0_result_t result = sd_spi0_address(lba++, &argument);
        if (result != SD_SPI0_OK) {
            return result;
        }
        uint8_t response = sd_spi0_command_open(24, argument);
        if (response != 0u) {
            sd_spi0_transaction_end();
            return response == 0xffu ? SD_SPI0_ERR_TIMEOUT : SD_SPI0_ERR_RESPONSE;
        }
        (void)sd_spi0_transfer(0xff);
        (void)sd_spi0_transfer(SD_SPI0_TOKEN_WRITE_BLOCK);
        for (unsigned int i = 0; i < SD_SPI0_SECTOR_SIZE; ++i) {
            (void)sd_spi0_transfer(buffer[i]);
        }
        (void)sd_spi0_transfer(0xff);
        (void)sd_spi0_transfer(0xff);
        response = sd_spi0_transfer(0xff);
        if ((response & 0x1fu) != SD_SPI0_TOKEN_DATA_ACCEPTED) {
            sd_spi0_transaction_end();
            return SD_SPI0_ERR_RESPONSE;
        }
        uint64_t start = sd_spi0_time_us();
        while (sd_spi0_transfer(0xff) == 0u) {
            if (sd_spi0_timed_out(start, SD_SPI0_WRITE_TIMEOUT_US)) {
                sd_spi0_transaction_end();
                return SD_SPI0_ERR_TIMEOUT;
            }
        }
        sd_spi0_transaction_end();
        buffer += SD_SPI0_SECTOR_SIZE;
    }
    return SD_SPI0_OK;
}

sd_spi0_result_t sd_spi0_ioctl(uint8_t command, void *buffer) {
    uint8_t register_data[16];
    if (!sd_state.initialized) {
        return SD_SPI0_ERR_NO_CARD;
    }
    switch (command) {
    case SD_SPI0_IOCTL_CTRL_SYNC: {
        uint8_t response = sd_spi0_command_open(13, 0);
        uint8_t card_status = 0xffu;
        if (response == 0u) {
            card_status = sd_spi0_transfer(0xff);
        }
        sd_spi0_transaction_end();
        return response == 0u && card_status == 0u ?
               SD_SPI0_OK : SD_SPI0_ERR_RESPONSE;
    }
    case SD_SPI0_IOCTL_GET_SECTOR_COUNT:
        if (buffer == NULL) return SD_SPI0_ERR_RESPONSE;
        *(uint32_t *)buffer = sd_state.sector_count;
        return SD_SPI0_OK;
    case SD_SPI0_IOCTL_GET_SECTOR_SIZE:
        if (buffer == NULL) return SD_SPI0_ERR_RESPONSE;
        *(uint16_t *)buffer = SD_SPI0_SECTOR_SIZE;
        return SD_SPI0_OK;
    case SD_SPI0_IOCTL_GET_BLOCK_SIZE:
        if (buffer == NULL) return SD_SPI0_ERR_RESPONSE;
        *(uint32_t *)buffer = 1u;
        return SD_SPI0_OK;
    case SD_SPI0_IOCTL_MMC_GET_TYPE:
        if (buffer == NULL) return SD_SPI0_ERR_RESPONSE;
        *(uint8_t *)buffer = sd_state.card_type | (sd_state.high_capacity ? 4u : 0u);
        return SD_SPI0_OK;
    case SD_SPI0_IOCTL_MMC_GET_CSD:
        if (buffer == NULL) return SD_SPI0_ERR_RESPONSE;
        if (sd_spi0_read_register(9, register_data) != SD_SPI0_OK) return SD_SPI0_ERR_RESPONSE;
        memcpy(buffer, register_data, sizeof(register_data));
        return SD_SPI0_OK;
    case SD_SPI0_IOCTL_MMC_GET_CID:
        if (buffer == NULL) return SD_SPI0_ERR_RESPONSE;
        if (sd_spi0_read_register(10, register_data) != SD_SPI0_OK) return SD_SPI0_ERR_RESPONSE;
        memcpy(buffer, register_data, sizeof(register_data));
        return SD_SPI0_OK;
    case SD_SPI0_IOCTL_MMC_GET_OCR:
        if (buffer == NULL) return SD_SPI0_ERR_RESPONSE;
        memcpy(buffer, sd_state.ocr, sizeof(sd_state.ocr));
        return SD_SPI0_OK;
    default:
        return SD_SPI0_ERR_UNSUPPORTED;
    }
}

const char *sd_spi0_result_string(sd_spi0_result_t result) {
    switch (result) {
    case SD_SPI0_OK: return "ok";
    case SD_SPI0_ERR_NO_CARD: return "no card/not initialized";
    case SD_SPI0_ERR_TIMEOUT: return "timeout";
    case SD_SPI0_ERR_RESPONSE: return "card response error";
    case SD_SPI0_ERR_UNSUPPORTED: return "unsupported card/operation";
    default: return "unknown error";
    }
}
