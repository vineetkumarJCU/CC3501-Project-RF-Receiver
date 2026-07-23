#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "ff.h"
#include "diskio.h"
#include "sd_spi0.h"

static bool sd_spi0_disk_initialized;

static DRESULT sd_spi0_disk_result(sd_spi0_result_t result) {
    if (result == SD_SPI0_OK) {
        return RES_OK;
    }
    if (result == SD_SPI0_ERR_NO_CARD) {
        return RES_NOTRDY;
    }
    return RES_ERROR;
}

DSTATUS disk_initialize(BYTE pdrv) {
    if (pdrv != 0u) {
        return STA_NOINIT;
    }

    sd_spi0_result_t result = sd_spi0_init();
    sd_spi0_disk_initialized = result == SD_SPI0_OK;
    if (!sd_spi0_disk_initialized) {
        printf("[SD] FatFs disk initialization failed: %s\n",
               sd_spi0_result_string(result));
    }
    return sd_spi0_disk_initialized ? 0u : STA_NOINIT;
}

DSTATUS disk_status(BYTE pdrv) {
    if (pdrv != 0u) {
        return STA_NOINIT;
    }
    if (!sd_spi0_card_is_inserted()) {
        sd_spi0_disk_initialized = false;
        sd_spi0_deinit();
        return STA_NOINIT | STA_NODISK;
    }
    if (!sd_spi0_disk_initialized || !sd_spi0_is_initialized()) {
        return STA_NOINIT;
    }
    return 0u;
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count) {
    if (pdrv != 0u) {
        return RES_PARERR;
    }
    if (!sd_spi0_disk_initialized) {
        return RES_NOTRDY;
    }
    if (buff == NULL || count == 0u) {
        return RES_PARERR;
    }
    sd_spi0_result_t result = sd_spi0_read_blocks(buff, (uint32_t)sector, (uint32_t)count);
    if (result != SD_SPI0_OK) {
        printf("[SD] Block read failed at LBA %lu (%u block(s)): %s\n",
               (unsigned long)sector,
               (unsigned)count,
               sd_spi0_result_string(result));
    }
    return sd_spi0_disk_result(result);
}

DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count) {
    if (pdrv != 0u) {
        return RES_PARERR;
    }
    if (!sd_spi0_disk_initialized) {
        return RES_NOTRDY;
    }
    if (buff == NULL || count == 0u) {
        return RES_PARERR;
    }
    sd_spi0_result_t result = sd_spi0_write_blocks(buff, (uint32_t)sector, (uint32_t)count);
    if (result != SD_SPI0_OK) {
        printf("[SD] Block write failed at LBA %lu (%u block(s)): %s\n",
               (unsigned long)sector,
               (unsigned)count,
               sd_spi0_result_string(result));
    }
    return sd_spi0_disk_result(result);
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff) {
    if (pdrv != 0u) {
        return RES_PARERR;
    }
    if (!sd_spi0_disk_initialized) {
        return RES_NOTRDY;
    }

    switch (cmd) {
    case CTRL_SYNC:
        return sd_spi0_disk_result(sd_spi0_ioctl(SD_SPI0_IOCTL_CTRL_SYNC, NULL));
    case GET_SECTOR_COUNT:
        if (buff == NULL) {
            return RES_PARERR;
        }
        return sd_spi0_disk_result(sd_spi0_ioctl(SD_SPI0_IOCTL_GET_SECTOR_COUNT, buff));
    case GET_SECTOR_SIZE:
        if (buff == NULL) {
            return RES_PARERR;
        }
        *(WORD *)buff = SD_SPI0_SECTOR_SIZE;
        return RES_OK;
    case GET_BLOCK_SIZE:
        if (buff == NULL) {
            return RES_PARERR;
        }
        *(DWORD *)buff = 1u;
        return RES_OK;
    default:
        return RES_PARERR;
    }
}
