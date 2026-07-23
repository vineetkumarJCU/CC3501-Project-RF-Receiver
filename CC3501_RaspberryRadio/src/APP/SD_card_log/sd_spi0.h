#ifndef SD_SPI0_H
#define SD_SPI0_H

#include <stdbool.h>
#include <stdint.h>

#define SD_SPI0_SECTOR_SIZE 512u

#define SD_SPI0_INIT_BAUDRATE_HZ 400000u
#ifdef SD_SPI0_HOST_TEST
#define SD_SPI0_TRANSFER_BAUDRATE_HZ 16000000u
#else
#define SD_SPI0_TRANSFER_BAUDRATE_HZ SD_CARD_SPI_BAUD_RATE
#endif
#define SD_SPI0_R1_IDLE_STATE 0x01u
#define SD_SPI0_R1_ILLEGAL_COMMAND 0x04u
#define SD_SPI0_TOKEN_START_BLOCK 0xfeu
#define SD_SPI0_TOKEN_WRITE_BLOCK 0xfeu
#define SD_SPI0_TOKEN_DATA_ACCEPTED 0x05u
#define SD_SPI0_CMD_TIMEOUT_US 100000u
#define SD_SPI0_DATA_TIMEOUT_US 250000u
#define SD_SPI0_WRITE_TIMEOUT_US 500000u
#define SD_SPI0_INIT_TIMEOUT_US 1000000u

typedef enum {
    SD_SPI0_OK = 0,
    SD_SPI0_ERR_NO_CARD,
    SD_SPI0_ERR_TIMEOUT,
    SD_SPI0_ERR_RESPONSE,
    SD_SPI0_ERR_UNSUPPORTED
} sd_spi0_result_t;

enum {
    SD_SPI0_IOCTL_CTRL_SYNC = 0,
    SD_SPI0_IOCTL_GET_SECTOR_COUNT = 1,
    SD_SPI0_IOCTL_GET_SECTOR_SIZE = 2,
    SD_SPI0_IOCTL_GET_BLOCK_SIZE = 3,
    SD_SPI0_IOCTL_MMC_GET_TYPE = 10,
    SD_SPI0_IOCTL_MMC_GET_CSD = 11,
    SD_SPI0_IOCTL_MMC_GET_CID = 12,
    SD_SPI0_IOCTL_MMC_GET_OCR = 13
};

sd_spi0_result_t sd_spi0_init(void);
void sd_spi0_deinit(void);
bool sd_spi0_is_initialized(void);
bool sd_spi0_card_is_inserted(void);
sd_spi0_result_t sd_spi0_read_blocks(uint8_t *buffer, uint32_t lba, uint32_t count);
sd_spi0_result_t sd_spi0_write_blocks(const uint8_t *buffer, uint32_t lba, uint32_t count);
sd_spi0_result_t sd_spi0_ioctl(uint8_t command, void *buffer);
const char *sd_spi0_result_string(sd_spi0_result_t result);

#endif
