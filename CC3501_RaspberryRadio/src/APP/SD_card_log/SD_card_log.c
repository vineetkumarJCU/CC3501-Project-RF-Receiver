#include "SD_card_log.h"

#include "ff.h"
#include "sd_spi0.h"

#include "pico/time.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>


static FATFS sd_fatfs;
static FIL sd_log_file;
static bool filesystem_mounted;
static bool log_file_open;
static sd_card_log_state_t log_state = SD_CARD_LOG_STATE_IDLE;
static uint64_t next_record_time_us;
static uint32_t log_record_number;
static char log_file_path[SD_CARD_LOG_PATH_SIZE];
static char log_text_buffer[SD_CARD_LOG_TEXT_BUFFER_SIZE];

static bool sd_card_log_is_leap_year(uint16_t year)
{
  return (year % 4U) == 0U && ((year % 100U) != 0U || (year % 400U) == 0U);
}

static uint8_t sd_card_log_days_in_month(uint16_t year, uint8_t month)
{
  static const uint8_t days[] = {31U, 28U, 31U, 30U, 31U, 30U,
                                 31U, 31U, 30U, 31U, 30U, 31U};

  if (month < 1U || month > 12U)
    return 0U;
  if (month == 2U && sd_card_log_is_leap_year(year))
    return 29U;
  return days[month - 1U];
}

static bool sd_card_log_get_local_datetime(uint16_t *year,
                                           uint8_t *month,
                                           uint8_t *day,
                                           uint8_t *hour,
                                           uint8_t *minute,
                                           uint8_t *second)
{
  gps_info_t gps_info;
  uint8_t month_days;

  if (year == NULL || month == NULL || day == NULL || hour == NULL ||
      minute == NULL || second == NULL ||
      !GPS_module_get_latest_info(&gps_info) ||
      !gps_info.utc_date.valid || !gps_info.utc_time.valid)
  {
    return false;
  }

  // Convert UTC to local time by applying the offset defined in SD_CARD_LOG_UTC_OFFSET_HOURS.

  *year = gps_info.utc_date.year;
  *month = gps_info.utc_date.month;
  *day = gps_info.utc_date.day;
  *hour = (uint8_t)(gps_info.utc_time.hour + SD_CARD_LOG_UTC_OFFSET_HOURS);
  *minute = gps_info.utc_time.minute;
  *second = gps_info.utc_time.second;

  // Validate the date and time values, ensuring they are within acceptable ranges.

  month_days = sd_card_log_days_in_month(*year, *month);
  if (*year < 1980U || *year > 2107U || month_days == 0U ||
      *day < 1U || *day > month_days || gps_info.utc_time.hour > 23U ||
      *minute > 59U || *second > 59U)
  {
    return false;
  }

  if (*hour >= 24U)
  {
    *hour = (uint8_t)(*hour - 24U);
    (*day)++;
    if (*day > month_days)
    {
      *day = 1U;
      (*month)++;
      if (*month > 12U)
      {
        *month = 1U;
        (*year)++;
        if (*year > 2107U)
          return false;
      }
    }
  }
  return true;
}

// This function is called by the FatFs library to get the current time for
DWORD get_fattime(void)
{
  uint16_t year = SD_CARD_LOG_DEFAULT_YEAR;
  uint8_t month = 1U;
  uint8_t day = 1U;
  uint8_t hour = 0U;
  uint8_t minute = 0U;
  uint8_t second = 0U;

  // Get the current local date and time from the GPS module, if available.

  if (!sd_card_log_get_local_datetime(&year, &month, &day,
                                      &hour, &minute, &second))
  {
    year = SD_CARD_LOG_DEFAULT_YEAR;
    month = 1U;
    day = 1U;
    hour = 0U;
    minute = 0U;
    second = 0U;
  }
  return ((DWORD)(year - 1980U) << 25) |
         ((DWORD)month << 21) |
         ((DWORD)day << 16) |
         ((DWORD)hour << 11) |
         ((DWORD)minute << 5) |
         ((DWORD)second >> 1);
}

// This function releases the resources used by the SD card logging system,
// including closing the log file and unmounting the filesystem.

static void sd_card_log_release_storage(void)
{
  FRESULT result;

  if (log_file_open)
  {
    result = f_close(&sd_log_file);
    if (result != FR_OK)
    {
      printf("[SD LOG] File close during cleanup failed (FRESULT=%d)\n", (int)result);
    }
    log_file_open = false;
  }

// Unmount the filesystem if it was mounted, or unregister the work area if it was not.
  if (filesystem_mounted)
  {
    result = f_mount(NULL, SD_CARD_LOG_DRIVE, 0);
    if (result != FR_OK)
    {
      printf("[SD LOG] Unmount during cleanup failed (FRESULT=%d)\n", (int)result);
    }
    filesystem_mounted = false;
  }
  else
  {
    /* f_mount(..., 1) registers the work area before attempting the physical
     * mount, so unregister it even when that immediate mount failed. */
    result = f_mount(NULL, SD_CARD_LOG_DRIVE, 0);
    if (result != FR_OK)
    {
      printf("[SD LOG] FatFs unregister during cleanup failed (FRESULT=%d)\n",
             (int)result);
    }
  }
  sd_spi0_deinit();
}

// This function aborts the SD card logging operation, releasing resources and
// setting the log state and result code accordingly.
static sd_card_log_result_t sd_card_log_abort(sd_card_log_state_t state,
                                              sd_card_log_result_t result)
{
  sd_card_log_release_storage();
  log_state = state;
  return result;
}

// This function parses a log file name to extract the index number, ensuring
// that it follows the expected format of "log_<index>.txt". It returns true if
// the parsing is successful and sets the index value, or false if the format is invalid.
static bool sd_card_log_parse_file_index(const char *name, uint32_t *index)
{
  const size_t prefix_length = strlen(SD_CARD_LOG_FILE_PREFIX);
  const char *cursor;
  uint64_t value = 0U;
  bool has_digit = false;

  // Validate the input parameters and check if the file name starts with the expected prefix.

  if (name == NULL || index == NULL ||
      strncmp(name, SD_CARD_LOG_FILE_PREFIX, prefix_length) != 0)
  {
    return false;
  }

  // Parse the numeric index from the file name, ensuring it consists of digits and is followed by ".txt".

  cursor = name + prefix_length;
  while (*cursor >= '0' && *cursor <= '9')
  {
    has_digit = true;
    value = value * 10U + (uint32_t)(*cursor - '0');
    if (value >= UINT32_MAX)
      return false;
    cursor++;
  }
  if (!has_digit || strcmp(cursor, ".txt") != 0)
    return false;
  *index = (uint32_t)value;
  return true;
}

// This function opens a new log file with a unique index in the specified directory.

static FRESULT sd_card_log_open_unique_file(void)
{
  DIR directory;
  FILINFO file_info;
  FRESULT result;
  FRESULT close_result;
  uint32_t maximum_index = 0U;

  // Open the log directory to find the highest existing log file index.

  result = f_opendir(&directory, SD_CARD_LOG_DIRECTORY);
  if (result != FR_OK)
    return result;

  for (;;)
  {
    uint32_t index;
    result = f_readdir(&directory, &file_info);
    if (result != FR_OK || file_info.fname[0] == '\0')
      break;
    if (sd_card_log_parse_file_index(file_info.fname, &index) &&
        index > maximum_index)
    {
      maximum_index = index;
    }
  }
  close_result = f_closedir(&directory);
  if (result != FR_OK)
    return result;
  if (close_result != FR_OK)
    return close_result;
  if (maximum_index == UINT32_MAX - 1U)
    return FR_DENIED;

  for (uint32_t index = maximum_index + 1U; index < UINT32_MAX; index++)
  {
    int length = snprintf(log_file_path,
                          sizeof(log_file_path),
                          "%s/%s%lu.txt",
                          SD_CARD_LOG_DIRECTORY,
                          SD_CARD_LOG_FILE_PREFIX,
                          (unsigned long)index);
    if (length < 0 || (size_t)length >= sizeof(log_file_path))
      return FR_INVALID_NAME;

    result = f_open(&sd_log_file, log_file_path, FA_WRITE | FA_CREATE_NEW);
    if (result == FR_EXIST)
      continue;
    if (result == FR_OK)
      log_record_number = 0U;
    return result;
  }
  return FR_DENIED;
}


// This function appends formatted text to the log text buffer, ensuring that it does not exceed the buffer size.

static bool sd_card_log_append_format(size_t *used, const char *format, ...)
{
  va_list args;
  int written;

  if (used == NULL || format == NULL || *used >= sizeof(log_text_buffer))
    return false;
  va_start(args, format);
  written = vsnprintf(log_text_buffer + *used,
                      sizeof(log_text_buffer) - *used,
                      format,
                      args);
  va_end(args);
  if (written < 0 || (size_t)written >= sizeof(log_text_buffer) - *used)
    return false;
  *used += (size_t)written;
  return true;
}

static char sd_card_log_hemisphere(char value, char fallback)
{
  return value != '\0' ? value : fallback;
}

static bool sd_card_log_format_gps(size_t *used, const gps_info_t *gps_info)
{
  if (!sd_card_log_append_format(used, "GPS:\r\n"))
    return false;
  if (gps_info == NULL)
  {
    return sd_card_log_append_format(used,
                                     "  UTC date: --\r\n"
                                     "  UTC time: --\r\n"
                                     "  Latitude: --\r\n"
                                     "  Longitude: --\r\n"
                                     "  Speed: --\r\n"
                                     "  Direction: --\r\n");
  }

  if (gps_info->utc_date.valid)
  {
    if (!sd_card_log_append_format(used,
                                   "  UTC date: %04u-%02u-%02u\r\n",
                                   (unsigned)gps_info->utc_date.year,
                                   (unsigned)gps_info->utc_date.month,
                                   (unsigned)gps_info->utc_date.day))
      return false;
  }
  else if (!sd_card_log_append_format(used, "  UTC date: --\r\n"))
    return false;

  if (gps_info->utc_time.valid)
  {
    if (!sd_card_log_append_format(used,
                                   "  UTC time: %02u:%02u:%02u.%03u\r\n",
                                   (unsigned)gps_info->utc_time.hour,
                                   (unsigned)gps_info->utc_time.minute,
                                   (unsigned)gps_info->utc_time.second,
                                   (unsigned)gps_info->utc_time.millisecond))
      return false;
  }
  else if (!sd_card_log_append_format(used, "  UTC time: --\r\n"))
    return false;

  if (gps_info->position.valid)
  {
    double latitude = gps_info->position.latitude_deg < 0.0 ? -gps_info->position.latitude_deg : gps_info->position.latitude_deg;
    double longitude = gps_info->position.longitude_deg < 0.0 ? -gps_info->position.longitude_deg : gps_info->position.longitude_deg;
    if (!sd_card_log_append_format(used,
                                   "  Latitude: %.6f %c\r\n"
                                   "  Longitude: %.6f %c\r\n",
                                   latitude,
                                   sd_card_log_hemisphere(gps_info->position.latitude_hemisphere,
                                                          gps_info->position.latitude_deg < 0.0 ? 'S' : 'N'),
                                   longitude,
                                   sd_card_log_hemisphere(gps_info->position.longitude_hemisphere,
                                                          gps_info->position.longitude_deg < 0.0 ? 'W' : 'E')))
    {
      return false;
    }
  }
  else if (!sd_card_log_append_format(used,
                                      "  Latitude: --\r\n"
                                      "  Longitude: --\r\n"))
    return false;

  if (gps_info->has_speed_kmh)
  {
    if (!sd_card_log_append_format(used, "  Speed: %.2f km/h\r\n", gps_info->speed_kmh))
      return false;
  }
  else if (gps_info->has_speed_knots)
  {
    if (!sd_card_log_append_format(used, "  Speed: %.2f kn\r\n", gps_info->speed_knots))
      return false;
  }
  else if (!sd_card_log_append_format(used, "  Speed: --\r\n"))
    return false;

  if (gps_info->has_course_true)
  {
    return sd_card_log_append_format(used,
                                     "  Direction: %.2f deg true\r\n",
                                     gps_info->course_true_deg);
  }
  return sd_card_log_append_format(used, "  Direction: --\r\n");
}

// This function formats the received signal quality (RSQ) information for logging.
static bool sd_card_log_format_rsq(size_t *used,
                                   const sd_card_log_rsq_info_t *rsq_info)
{
  const char *band = rsq_info != NULL && rsq_info->band == SD_CARD_LOG_RADIO_SW_AM_LW ? "SW/AM/LW" : "FM";

  if (!sd_card_log_append_format(used, "SI4732 %s RSQ:\r\n", band))
    return false;
  if (rsq_info == NULL || !rsq_info->receive_frequency_valid)
  {
    if (!sd_card_log_append_format(used, "  Receive frequency: --\r\n"))
      return false;
  }
  else if (rsq_info->band == SD_CARD_LOG_RADIO_FM)
  {
    uint32_t frequency_khz = rsq_info->receive_frequency_hz / 1000U;
    if (!sd_card_log_append_format(used,
                                   "  Receive frequency: %lu.%03lu MHz\r\n",
                                   (unsigned long)(frequency_khz / 1000U),
                                   (unsigned long)(frequency_khz % 1000U)))
      return false;
  }
  else
  {
    if (!sd_card_log_append_format(used,
                                   "  Receive frequency: %lu kHz\r\n",
                                   (unsigned long)(rsq_info->receive_frequency_hz /
                                                   1000U)))
      return false;
  }

  if (rsq_info == NULL || !rsq_info->valid)
  {
    return sd_card_log_append_format(used,
                                     "  AFCRL: --\r\n"
                                     "  VALID: --\r\n"
                                     "  PILOT: --\r\n"
                                     "  STBLEND: --\r\n"
                                     "  RSSI: --\r\n"
                                     "  SNR: --\r\n"
                                     "  Multipath: --\r\n"
                                     "  Frequency offset: --\r\n");
  }

  if (!sd_card_log_append_format(used,
                                 "  AFCRL: %s\r\n"
                                 "  VALID: %s\r\n",
                                 rsq_info->afcrl ? "true" : "false",
                                 rsq_info->station_valid ? "true" : "false"))
    return false;

  if (rsq_info->band == SD_CARD_LOG_RADIO_FM)
  {
    return sd_card_log_append_format(used,
                                     "  PILOT: %s\r\n"
                                     "  STBLEND: %u\r\n"
                                     "  RSSI: %d dBm\r\n"
                                     "  SNR: %u dB\r\n"
                                     "  Multipath: %u\r\n"
                                     "  Frequency offset: %d\r\n",
                                     rsq_info->pilot ? "true" : "false",
                                     (unsigned)rsq_info->stereo_blend,
                                     (int)rsq_info->rssi_dbm,
                                     (unsigned)rsq_info->snr_db,
                                     (unsigned)rsq_info->multipath,
                                     (int)rsq_info->frequency_offset);
  }

  return sd_card_log_append_format(used,
                                   "  PILOT: --\r\n"
                                   "  STBLEND: --\r\n"
                                   "  RSSI: %d dBm\r\n"
                                   "  SNR: %u dB\r\n"
                                   "  Multipath: --\r\n"
                                   "  Frequency offset: --\r\n",
                                   (int)rsq_info->rssi_dbm,
                                   (unsigned)rsq_info->snr_db);
}

// This function starts the SD card logging process, mounting the filesystem, creating the log directory if necessary, and opening a unique log file for writing.

sd_card_log_result_t sd_card_log_start(void)
{
  FRESULT result;
  uint16_t year;
  uint8_t month;
  uint8_t day;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;

  // Release any previously allocated resources before starting a new logging session.

  if (log_file_open || filesystem_mounted)
    sd_card_log_release_storage();
  log_state = SD_CARD_LOG_STATE_IDLE;

  // Check if the SD card is inserted by reading the SD_CARD_DETECT_PIN. If the pin is high, it indicates that no card is present.
  if (!sd_spi0_card_is_inserted())
  {
    // If the SD card is not detected, print a message and set the log state to indicate that there is no card.
    printf("[SD LOG] Start failed: SD_CARD_DETECT_PIN is high (no card)\n");
    log_state = SD_CARD_LOG_STATE_NO_CARD;
    return SD_CARD_LOG_ERR_NO_CARD;
  }
// If the SD card is detected, proceed to mount the FatFs volume and create the log directory if it does not exist.
  printf("[SD LOG] Card detected; mounting FatFs volume %s\n", SD_CARD_LOG_DRIVE);
  result = f_mount(&sd_fatfs, SD_CARD_LOG_DRIVE, 1);
  if (result != FR_OK)
  {
    // If mounting the FatFs volume fails, print an error message and abort the logging process with an appropriate error code.
    printf("[SD LOG] Mount failed (FRESULT=%d)\n", (int)result);
    return sd_card_log_abort(SD_CARD_LOG_STATE_ERROR, SD_CARD_LOG_ERR_MOUNT);
  }
  // If mounting is successful, set the filesystem_mounted flag to true and print a message indicating that the volume has been mounted.
  filesystem_mounted = true;
  printf("[SD LOG] FatFs volume mounted\n");

  // Attempt to create the log directory. If it already exists, this will return FR_EXIST, which is acceptable.
  result = f_mkdir(SD_CARD_LOG_DIRECTORY);
  if (result != FR_OK && result != FR_EXIST)
  {

    // If creating the log directory fails for any reason other than it already existing, print an error message and abort the logging process with an appropriate error code.
    printf("[SD LOG] Directory creation failed for %s (FRESULT=%d)\n",
           SD_CARD_LOG_DIRECTORY, (int)result);
    return sd_card_log_abort(SD_CARD_LOG_STATE_ERROR, SD_CARD_LOG_ERR_DIRECTORY);
  }
  // Print a message indicating that the log directory is ready, along with whether it was newly created or already existed.
  printf("[SD LOG] Directory ready: %s%s\n",
         SD_CARD_LOG_DIRECTORY,
         result == FR_EXIST ? " (already exists)" : "");

  result = sd_card_log_open_unique_file();

  // If opening a unique log file fails, print an error message and abort the logging process with an appropriate error code.
  if (result != FR_OK)
  {
    printf("[SD LOG] Unable to create a unique log file (FRESULT=%d)\n", (int)result);
    return sd_card_log_abort(SD_CARD_LOG_STATE_ERROR, SD_CARD_LOG_ERR_FILE);
  }

  // If opening the log file is successful, set the log_file_open flag to true and print a message indicating that the log file has been created.
  log_file_open = true;

  result = f_sync(&sd_log_file);
  if (result != FR_OK)
  {
    // If synchronizing the log file fails, print an error message and abort the logging process with an appropriate error code.
    printf("[SD LOG] Initial file synchronization failed (FRESULT=%d)\n", (int)result);
    return sd_card_log_abort(SD_CARD_LOG_STATE_ERROR, SD_CARD_LOG_ERR_FILE);
  }

  // Print a message indicating that the log file has been created, along with the FAT timestamp obtained from the GPS module or a default timestamp if GPS data is unavailable.
  if (sd_card_log_get_local_datetime(&year, &month, &day, &hour, &minute, &second))
  {
    printf("[SD LOG] Created %s; FAT timestamp=%04u-%02u-%02u %02u:%02u:%02u (GPS UTC+10)\n",
           log_file_path,
           (unsigned)year, (unsigned)month, (unsigned)day,
           (unsigned)hour, (unsigned)minute, (unsigned)second);
  }
  else
  {
    printf("[SD LOG] Created %s; GPS date/time unavailable, FAT timestamp defaults to 1980-01-01 00:00:00\n",
           log_file_path);
  }

  next_record_time_us = time_us_64();
  log_state = SD_CARD_LOG_STATE_LOGGING;
  return SD_CARD_LOG_OK;
}

// This function checks if it is time to record a new log entry based on the current time and the next scheduled record time. It also checks if the SD card is still inserted, and if not, it stops the logging process.
bool sd_card_log_record_due(void)
{
  if (log_state != SD_CARD_LOG_STATE_LOGGING)
    return false;
  if (!sd_spi0_card_is_inserted())
  {
    // If the SD card is removed while logging, print a message and abort the logging process with an appropriate error code.
    printf("[SD LOG] Card removal detected; stopping the active log\n");
    (void)sd_card_log_abort(SD_CARD_LOG_STATE_NO_CARD, SD_CARD_LOG_ERR_NO_CARD);
    return false;
  }
  return time_us_64() >= next_record_time_us;
}

// This function appends a new log entry to the currently open log file, including GPS and RSQ information. It formats the data and writes it to the file, handling any errors that may occur during the process.
sd_card_log_result_t sd_card_log_append(const gps_info_t *gps_info,
                                        const sd_card_log_rsq_info_t *rsq_info)
{
  size_t used = 0U;
  UINT bytes_written = 0U;
  FRESULT result;

  // Check if the log state is active and if the log file is open. If not, print a message and return an error code.
  if (log_state != SD_CARD_LOG_STATE_LOGGING || !log_file_open)
  {
    printf("[SD LOG] Append rejected: no active log file\n");
    return SD_CARD_LOG_ERR_FILE;
  }
  // Check if the SD card is still inserted before attempting to append data. If the card has been removed, print a message and abort the logging process with an appropriate error code.
  if (!sd_spi0_card_is_inserted())
  {
    printf("[SD LOG] Append failed: card was removed\n");
    return sd_card_log_abort(SD_CARD_LOG_STATE_NO_CARD, SD_CARD_LOG_ERR_NO_CARD);
  }

  if (!sd_card_log_format_gps(&used, gps_info) ||
      !sd_card_log_format_rsq(&used, rsq_info) ||
      !sd_card_log_append_format(&used, "\r\n"))
  {
    printf("[SD LOG] Internal formatting buffer overflow\n");
    return sd_card_log_abort(SD_CARD_LOG_STATE_ERROR, SD_CARD_LOG_ERR_WRITE);
  }

  // Write the formatted log entry to the log file and check for any errors during the write operation. If an error occurs, print a message and abort the logging process with an appropriate error code.
  result = f_write(&sd_log_file, log_text_buffer, (UINT)used, &bytes_written);
  if (result != FR_OK || bytes_written != (UINT)used)
  {
    // If the write operation fails or the number of bytes written does not match the expected size, print an error message and abort the logging process with an appropriate error code.
    printf("[SD LOG] Write failed (FRESULT=%d, requested=%u, written=%u)\n",
           (int)result, (unsigned)used, (unsigned)bytes_written);
    return sd_card_log_abort(SD_CARD_LOG_STATE_ERROR, SD_CARD_LOG_ERR_WRITE);
  }

  // Synchronize the log file to ensure that all data is written to the SD card. If synchronization fails, print a message and abort the logging process with an appropriate error code.
  result = f_sync(&sd_log_file);
  if (result != FR_OK)
  {
    // If the synchronization operation fails, print an error message and abort the logging process with an appropriate error code.
    printf("[SD LOG] Synchronization failed after append (FRESULT=%d)\n", (int)result);
    return sd_card_log_abort(SD_CARD_LOG_STATE_ERROR, SD_CARD_LOG_ERR_WRITE);
  }

  // Increment the log record number and update the next scheduled record time. Print a message indicating that the log entry has been successfully appended to the log file.
  log_record_number++;
  next_record_time_us = time_us_64() + SD_CARD_LOG_INTERVAL_US;
  // Print a message indicating that the log entry has been successfully appended to the log file, including the record number, number of bytes written, and the log file path.
  printf("[SD LOG] Appended record %lu (%u bytes) to %s\n",
         (unsigned long)log_record_number,
         (unsigned)bytes_written,
         log_file_path);
  return SD_CARD_LOG_OK;
}

// This function stops the SD card logging process, closing the log file and unmounting the filesystem. 
// It handles any errors that may occur during the stop operation and returns an appropriate result code.
sd_card_log_result_t sd_card_log_stop(void)
{
  FRESULT sync_result = FR_OK;
  FRESULT close_result = FR_OK;
  FRESULT unmount_result = FR_OK;

  // If the log file is open, synchronize and close it, handling any errors that may occur during these operations.
  if (log_file_open)
  {
    sync_result = f_sync(&sd_log_file);
    if (sync_result != FR_OK)
    {
      // If the synchronization operation fails, print an error message indicating that the final synchronization failed, along with the FRESULT code.
      printf("[SD LOG] Final synchronization failed (FRESULT=%d)\n", (int)sync_result);
    }
    close_result = f_close(&sd_log_file);
    if (close_result != FR_OK)
    {
      // If the close operation fails, print an error message indicating that the file close failed, along with the FRESULT code.
      printf("[SD LOG] File close failed (FRESULT=%d)\n", (int)close_result);
    }
    else
    {
      // If the log file is successfully closed, print a message indicating that the log file has been closed, along with the number of records written to the file.
      printf("[SD LOG] Closed %s after %lu record(s)\n",
             log_file_path, (unsigned long)log_record_number);
    }
    log_file_open = false;
  }

  // Unmount the filesystem if it was mounted, or unregister the work area if it was not. Handle any errors that may occur during these operations.
  unmount_result = f_mount(NULL, SD_CARD_LOG_DRIVE, 0);
  if (filesystem_mounted)
  {
    if (unmount_result != FR_OK)
    {
      // If the unmount operation fails, print an error message indicating that the FatFs unmount failed, along with the FRESULT code.
      printf("[SD LOG] FatFs unmount failed (FRESULT=%d)\n", (int)unmount_result);
    }
    else
    {
      // If the filesystem is successfully unmounted, print a message indicating that the FatFs volume has been unmounted.
      printf("[SD LOG] FatFs volume unmounted\n");
    }
    filesystem_mounted = false;
  }
  else
  {
    // If the filesystem was not mounted, print a message indicating that the stop request was made while no volume was mounted.
    if (unmount_result != FR_OK)
    {
      printf("[SD LOG] FatFs unregister failed (FRESULT=%d)\n", (int)unmount_result);
    }
    else
    {
      printf("[SD LOG] Stop requested while no volume was mounted\n");
    }
  }
  sd_spi0_deinit();

  // If any of the synchronization, close, or unmount operations failed, set the log state to error and return an appropriate error code.
  if (sync_result != FR_OK || close_result != FR_OK || unmount_result != FR_OK)
  {
    log_state = SD_CARD_LOG_STATE_ERROR;
    return SD_CARD_LOG_ERR_UNMOUNT;
  }
  log_state = SD_CARD_LOG_STATE_IDLE;
  return SD_CARD_LOG_OK;
}

// This function returns the current state of the SD card logging process, which can be one of the defined states in the sd_card_log_state_t enumeration.
sd_card_log_state_t sd_card_log_get_state(void)
{
  return log_state;
}

// This function checks if the SD card logging process is currently active, returning true if it is in the logging state and false otherwise.
bool sd_card_log_is_active(void)
{
  return log_state == SD_CARD_LOG_STATE_LOGGING;
}

// This function checks if an SD card is currently inserted in the system by calling the sd_spi0_card_is_inserted() function, which reads the state of the SD card detect pin.
bool sd_card_log_card_is_inserted(void)
{
  return sd_spi0_card_is_inserted();
}

// This function returns a string representation of the current SD card logging state, which can be used for debugging or display purposes.

const char *sd_card_log_state_string(sd_card_log_state_t state)
{
  switch (state)
  {
  case SD_CARD_LOG_STATE_IDLE:
    return "idle";
  case SD_CARD_LOG_STATE_LOGGING:
    return "logging";
  case SD_CARD_LOG_STATE_NO_CARD:
    return "no card";
  case SD_CARD_LOG_STATE_ERROR:
    return "error";
  default:
    return "unknown";
  }
}
