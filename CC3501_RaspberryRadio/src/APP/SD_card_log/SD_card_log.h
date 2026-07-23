#ifndef SD_CARD_LOG_H
#define SD_CARD_LOG_H

#include <stdbool.h>
#include <stdint.h>

#include "GPS_module.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SD_CARD_LOG_DRIVE "0:"
#define SD_CARD_LOG_DIRECTORY "0:/RaspberryRadio_log"
#define SD_CARD_LOG_FILE_PREFIX "GPSandRadio_log_"
#define SD_CARD_LOG_INTERVAL_US 5000000ULL
#define SD_CARD_LOG_TEXT_BUFFER_SIZE 1024U
#define SD_CARD_LOG_PATH_SIZE 96U
#define SD_CARD_LOG_DEFAULT_YEAR 1980U
#define SD_CARD_LOG_UTC_OFFSET_HOURS 10U

typedef enum
{
  SD_CARD_LOG_RADIO_FM = 0,
  SD_CARD_LOG_RADIO_SW_AM_LW
} sd_card_log_radio_band_t;

/** Compact SI4732 snapshot supplied by the GUI/hardware boundary. */
typedef struct
{
  bool valid;
  sd_card_log_radio_band_t band;
  bool afcrl;
  bool station_valid;
  bool pilot;
  uint8_t stereo_blend;
  int16_t rssi_dbm;
  uint8_t snr_db;
  uint8_t multipath;
  int8_t frequency_offset;
} sd_card_log_rsq_info_t;

typedef enum
{
  SD_CARD_LOG_STATE_IDLE = 0,
  SD_CARD_LOG_STATE_LOGGING,
  SD_CARD_LOG_STATE_NO_CARD,
  SD_CARD_LOG_STATE_ERROR
} sd_card_log_state_t;

typedef enum
{
  SD_CARD_LOG_OK = 0,
  SD_CARD_LOG_ERR_NO_CARD,
  SD_CARD_LOG_ERR_MOUNT,
  SD_CARD_LOG_ERR_DIRECTORY,
  SD_CARD_LOG_ERR_FILE,
  SD_CARD_LOG_ERR_WRITE,
  SD_CARD_LOG_ERR_UNMOUNT
} sd_card_log_result_t;

/** Mount the card, create the log directory, and open a new numbered file. */
sd_card_log_result_t sd_card_log_start(void);

/** Return true when the active file is ready for its next five-second record. */
bool sd_card_log_record_due(void);

/** Append one complete GPS/SI4732 record and synchronize it to the card. */
sd_card_log_result_t sd_card_log_append(const gps_info_t *gps_info,
                                        const sd_card_log_rsq_info_t *rsq_info);

/** Close the active file and unmount the card. Safe to call while idle. */
sd_card_log_result_t sd_card_log_stop(void);

sd_card_log_state_t sd_card_log_get_state(void);
bool sd_card_log_is_active(void);
bool sd_card_log_card_is_inserted(void);
const char *sd_card_log_state_string(sd_card_log_state_t state);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* SD_CARD_LOG_H */
