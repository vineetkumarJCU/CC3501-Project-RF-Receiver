#include "GPS_module.h"

#include "GPS_nmea_parser.h"
#include "board_init.h"
#include "board_pin_def.h"

#include "hardware/gpio.h"
#include "hardware/sync.h"
#include "hardware/timer.h"
#include "hardware/uart.h"
#include "pico/critical_section.h"

#include <stdio.h>
#include <string.h>

// GPS module state and configuration

#define GPS_FRAME_IDLE_TIME_US 150000ULL

// Internal state variables for GPS module

static gps_nmea_parser_t gps_parser;
static gps_info_t latest_gps_info;
static critical_section_t latest_info_lock;
static volatile bool module_initialized;
static bool latest_info_available;
static volatile bool module_powered;
static volatile uint32_t pps_count;
static volatile uint64_t last_pps_time_us;
static uint64_t last_uart_byte_time_us;
static uint32_t frame_number;
static bool parser_power_state;

static void gps_pps_irq_callback(uint gpio, uint32_t events)
{
  if(gpio == GPS_PPS_PIN && (events & GPIO_IRQ_EDGE_RISE) != 0U) {
    last_pps_time_us = time_us_64();
    pps_count++;
  }
}

// Publish a completed GPS frame to the latest info structure
static void publish_frame(gps_info_t *gps_info)
{
  uint32_t interrupt_state;

  if(gps_info == NULL) return;
  gps_info->frame_number = ++frame_number;
  gps_info->frame_received_time_us = time_us_64();

  /* The 64-bit PPS timestamp is written in an IRQ on this same core. */
  interrupt_state = save_and_disable_interrupts();
  gps_info->pps_count = pps_count;
  gps_info->last_pps_time_us = last_pps_time_us;
  restore_interrupts(interrupt_state);
  gps_info->pps_seen = gps_info->pps_count > 0U;
  gps_info->pps_level = gpio_get(GPS_PPS_PIN);

  critical_section_enter_blocking(&latest_info_lock);
  latest_gps_info = *gps_info;
  latest_info_available = true;
  critical_section_exit(&latest_info_lock);
}

// Initialize the GPS module and its internal state

void GPS_module_init(void)
{
  if(module_initialized) return;

  gps_nmea_parser_init(&gps_parser);
  critical_section_init(&latest_info_lock);
  memset(&latest_gps_info, 0, sizeof(latest_gps_info));
  latest_info_available = false;
  frame_number = 0U;
  pps_count = 0U;
  last_pps_time_us = 0U;
  last_uart_byte_time_us = 0U;
  module_powered = gpio_get(GPS_POWER_UP_PIN) == 0U;
  parser_power_state = module_powered;

  while(uart_is_readable(GPS_UART_HANDLE)) (void)uart_getc(GPS_UART_HANDLE);
  gpio_set_irq_enabled_with_callback(GPS_PPS_PIN,
                                     GPIO_IRQ_EDGE_RISE,
                                     true,
                                     gps_pps_irq_callback);
  module_initialized = true;
}

// Process incoming GPS data and update the latest info structure if a complete frame is received

bool GPS_module_process(void)
{
  static gps_info_t completed_frame;
  bool received_byte = false;
  bool published = false;
  uint64_t now;

  // Ensure the GPS module is initialized before processing data
  if(!module_initialized) GPS_module_init();

  if(parser_power_state != module_powered) {
    gps_nmea_parser_init(&gps_parser);
    last_uart_byte_time_us = 0U;
    while(uart_is_readable(GPS_UART_HANDLE)) (void)uart_getc(GPS_UART_HANDLE);
    parser_power_state = module_powered;
  }
  if(!module_powered) return false;

  while(uart_is_readable(GPS_UART_HANDLE)) {
    gps_nmea_parser_feed_byte(&gps_parser, uart_getc(GPS_UART_HANDLE));
    received_byte = true;
    last_uart_byte_time_us = time_us_64();
    if(gps_nmea_parser_take_completed_frame(&gps_parser, &completed_frame)) {
      publish_frame(&completed_frame);
      published = true;
    }
  }

  // If no bytes were received and the parser has been idle for a sufficient time, finalize the current frame if available
  now = time_us_64();
  if(!received_byte && last_uart_byte_time_us != 0U &&
     (now - last_uart_byte_time_us) >= GPS_FRAME_IDLE_TIME_US &&
     gps_nmea_parser_finish_frame(&gps_parser, &completed_frame)) {
    publish_frame(&completed_frame);
    last_uart_byte_time_us = 0U;
    published = true;
  }
  return published;
}

// Retrieve the latest GPS information if available

bool GPS_module_get_latest_info(gps_info_t *gps_info)
{
  bool available;

  if(gps_info == NULL || !module_initialized) return false;
  critical_section_enter_blocking(&latest_info_lock);
  available = latest_info_available;
  if(available) *gps_info = latest_gps_info;
  critical_section_exit(&latest_info_lock);
  return available;
}

// Log the GPS information to the console for debugging and monitoring purposes

static const char *gps_bool_text(bool value)
{
  return value ? "true" : "false";
}

static char gps_log_char(char value)
{
  return value != '\0' ? value : '-';
}

static const char *gps_constellation_text(gps_constellation_t constellation)
{
  switch(constellation) {
    case GPS_CONSTELLATION_GPS: return "GPS";
    case GPS_CONSTELLATION_GLONASS: return "GLONASS";
    case GPS_CONSTELLATION_BEIDOU: return "BeiDou";
    case GPS_CONSTELLATION_GALILEO: return "Galileo";
    case GPS_CONSTELLATION_QZSS: return "QZSS";
    case GPS_CONSTELLATION_NAVIC: return "NavIC";
    case GPS_CONSTELLATION_MIXED: return "Mixed";
    case GPS_CONSTELLATION_UNKNOWN:
    default: return "Unknown";
  }
}

// Log the GPS information to the console for debugging and monitoring purposes

static const char *gps_antenna_status_text(gps_antenna_status_t status)
{
  switch(status) {
    case GPS_ANTENNA_OK: return "OK";
    case GPS_ANTENNA_OPEN: return "OPEN";
    case GPS_ANTENNA_SHORT: return "SHORT";
    case GPS_ANTENNA_UNKNOWN:
    default: return "UNKNOWN";
  }
}

// Log the GPS information to the console for debugging and monitoring purposes

static void gps_log_time(const char *section, const gps_utc_time_t *time)
{
  printf("[GPS][%s][UTC] valid=%s time=%02u:%02u:%02u.%03u\n",
         section,
         gps_bool_text(time->valid),
         (unsigned)time->hour,
         (unsigned)time->minute,
         (unsigned)time->second,
         (unsigned)time->millisecond);
}

// Log the GPS information to the console for debugging and monitoring purposes

static void gps_log_date(const char *section, const gps_utc_date_t *date)
{
  printf("[GPS][%s][DATE] valid=%s date=%04u-%02u-%02u\n",
         section,
         gps_bool_text(date->valid),
         (unsigned)date->year,
         (unsigned)date->month,
         (unsigned)date->day);
}

// Log the GPS information to the console for debugging and monitoring purposes

static void gps_log_position(const char *section, const gps_position_t *position)
{
  printf("[GPS][%s][POSITION] valid=%s latitude=%.8f hemisphere=%c longitude=%.8f hemisphere=%c\n",
         section,
         gps_bool_text(position->valid),
         position->latitude_deg,
         gps_log_char(position->latitude_hemisphere),
         position->longitude_deg,
         gps_log_char(position->longitude_hemisphere));
}

// Log the GPS information to the console for debugging and monitoring purposes

void GPS_module_log_all_info(const gps_info_t *gps_info)
{
  uint8_t count;

  if(gps_info == NULL) return;

  // Log the summary information of the GPS frame, including frame number, state, satellite counts, and PPS information
  printf("\n[GPS] ==================== FRAME %lu ====================\n",
         (unsigned long)gps_info->frame_number);
  printf("[GPS][SUMMARY] frame=%lu state=%s satellites=%u/%u checksum_errors=%u PPS=%lu\n",
         (unsigned long)gps_info->frame_number,
         gps_info->position_valid ? "locked" : "unlocked",
         (unsigned)gps_info->satellites_used,
         (unsigned)gps_info->satellites_in_view,
         (unsigned)gps_info->checksum_error_count,
         (unsigned long)gps_info->pps_count);
  printf("[GPS][FRAME] received_time_us=%llu received_mask=0x%08lX valid_mask=0x%08lX\n",
         (unsigned long long)gps_info->frame_received_time_us,
         (unsigned long)gps_info->received_sentence_mask,
         (unsigned long)gps_info->valid_sentence_mask);
  printf("[GPS][FRAME] received=%u valid=%u checksum_errors=%u parse_errors=%u unsupported=%u\n",
         (unsigned)gps_info->received_sentence_count,
         (unsigned)gps_info->valid_sentence_count,
         (unsigned)gps_info->checksum_error_count,
         (unsigned)gps_info->parse_error_count,
         (unsigned)gps_info->unsupported_sentence_count);
  printf("[GPS][PPS] seen=%s level=%s count=%lu last_time_us=%llu\n",
         gps_bool_text(gps_info->pps_seen),
         gps_bool_text(gps_info->pps_level),
         (unsigned long)gps_info->pps_count,
         (unsigned long long)gps_info->last_pps_time_us);

  printf("[GPS][SUMMARY] position_valid=%s fix_quality=%u fix_type=%u satellites_used=%u satellites_in_view=%u constellation_mask=0x%08lX\n",
         gps_bool_text(gps_info->position_valid),
         (unsigned)gps_info->fix_quality,
         (unsigned)gps_info->fix_type,
         (unsigned)gps_info->satellites_used,
         (unsigned)gps_info->satellites_in_view,
         (unsigned long)gps_info->constellation_mask);
  gps_log_position("SUMMARY", &gps_info->position);
  gps_log_time("SUMMARY", &gps_info->utc_time);
  gps_log_date("SUMMARY", &gps_info->utc_date);
  printf("[GPS][SUMMARY] altitude_msl_present=%s altitude_msl_m=%.3f altitude_ellipsoid_present=%s altitude_ellipsoid_m=%.3f\n",
         gps_bool_text(gps_info->has_altitude_msl),
         gps_info->altitude_msl_m,
         gps_bool_text(gps_info->has_altitude_ellipsoid),
         gps_info->altitude_ellipsoid_m);
  printf("[GPS][SUMMARY] speed_knots_present=%s speed_knots=%.3f speed_kmh_present=%s speed_kmh=%.3f\n",
         gps_bool_text(gps_info->has_speed_knots),
         gps_info->speed_knots,
         gps_bool_text(gps_info->has_speed_kmh),
         gps_info->speed_kmh);
  printf("[GPS][SUMMARY] course_present=%s course_true_deg=%.3f magnetic_variation_present=%s magnetic_variation_deg=%.3f direction=%c\n",
         gps_bool_text(gps_info->has_course_true),
         gps_info->course_true_deg,
         gps_bool_text(gps_info->has_magnetic_variation),
         gps_info->magnetic_variation_deg,
         gps_log_char(gps_info->magnetic_variation_direction));
  printf("[GPS][SUMMARY] location_mode=%c location_status=%c navigation_status=%c antenna=%s\n",
         gps_log_char(gps_info->location_mode),
         gps_log_char(gps_info->location_status),
         gps_log_char(gps_info->navigation_status),
         gps_antenna_status_text(gps_info->antenna_status));

  printf("[GPS][GGA] present=%s fix_quality=%u satellites_used=%u hdop_present=%s hdop=%.3f\n",
         gps_bool_text(gps_info->gga.present),
         (unsigned)gps_info->gga.fix_quality,
         (unsigned)gps_info->gga.satellites_used,
         gps_bool_text(gps_info->gga.has_hdop),
         gps_info->gga.hdop);
  gps_log_time("GGA", &gps_info->gga.utc_time);
  gps_log_position("GGA", &gps_info->gga.position);
  printf("[GPS][GGA] altitude_msl_present=%s altitude_msl_m=%.3f geoid_present=%s geoid_separation_m=%.3f\n",
         gps_bool_text(gps_info->gga.has_altitude_msl),
         gps_info->gga.altitude_msl_m,
         gps_bool_text(gps_info->gga.has_geoid_separation),
         gps_info->gga.geoid_separation_m);
  printf("[GPS][GGA] differential_age_present=%s differential_age_s=%.3f station_id_present=%s station_id=%u\n",
         gps_bool_text(gps_info->gga.has_differential_age),
         gps_info->gga.differential_age_s,
         gps_bool_text(gps_info->gga.has_differential_station_id),
         (unsigned)gps_info->gga.differential_station_id);

  printf("[GPS][GLL] present=%s data_status=%c mode=%c\n",
         gps_bool_text(gps_info->gll.present),
         gps_log_char(gps_info->gll.data_status),
         gps_log_char(gps_info->gll.mode));
  gps_log_position("GLL", &gps_info->gll.position);
  gps_log_time("GLL", &gps_info->gll.utc_time);

  // Log the GSA records, including selection mode, fix type, constellation, system ID, satellite PRNs, and dilution of precision values

  count = gps_info->gsa_count < GPS_MAX_GSA_RECORDS ? gps_info->gsa_count : GPS_MAX_GSA_RECORDS;
  printf("[GPS][GSA] record_count=%u\n", (unsigned)gps_info->gsa_count);
  for(uint8_t i = 0U; i < count; i++) {
    const gps_gsa_info_t *gsa = &gps_info->gsa[i];
    uint8_t satellite_count = gsa->satellite_count < GPS_MAX_GSA_SATELLITES ?
                              gsa->satellite_count : GPS_MAX_GSA_SATELLITES;
    printf("[GPS][GSA %u] present=%s selection_mode=%c fix_type=%u constellation=%s system_id=%u satellite_count=%u\n",
           (unsigned)i,
           gps_bool_text(gsa->present),
           gps_log_char(gsa->selection_mode),
           (unsigned)gsa->fix_type,
           gps_constellation_text(gsa->constellation),
           (unsigned)gsa->system_id,
           (unsigned)gsa->satellite_count);
    printf("[GPS][GSA %u] pdop_present=%s pdop=%.3f hdop_present=%s hdop=%.3f vdop_present=%s vdop=%.3f\n",
           (unsigned)i,
           gps_bool_text(gsa->has_pdop),
           gsa->pdop,
           gps_bool_text(gsa->has_hdop),
           gsa->hdop,
           gps_bool_text(gsa->has_vdop),
           gsa->vdop);
    for(uint8_t satellite = 0U; satellite < satellite_count; satellite++) {
      printf("[GPS][GSA %u][USED %u] PRN=%u\n",
             (unsigned)i,
             (unsigned)satellite,
             (unsigned)gsa->satellite_prn[satellite]);
    }
  }

  // Log the GSV groups, including talker ID, constellation, total messages, received message mask, satellites in view, and signal ID information

  count = gps_info->gsv_group_count < GPS_MAX_GSV_GROUPS ?
          gps_info->gsv_group_count : GPS_MAX_GSV_GROUPS;
  printf("[GPS][GSV] group_count=%u\n", (unsigned)gps_info->gsv_group_count);
  for(uint8_t i = 0U; i < count; i++) {
    const gps_gsv_group_info_t *group = &gps_info->gsv_groups[i];
    printf("[GPS][GSV %u] present=%s talker=%s constellation=%s total_messages=%u received_mask=0x%08lX satellites_in_view=%u signal_id_present=%s signal_id=%u\n",
           (unsigned)i,
           gps_bool_text(group->present),
           group->talker,
           gps_constellation_text(group->constellation),
           (unsigned)group->total_messages,
           (unsigned long)group->received_message_mask,
           (unsigned)group->satellites_in_view,
           gps_bool_text(group->has_signal_id),
           (unsigned)group->signal_id);
  }

  // Log the satellite information, including constellation, PRN, usage in fix, elevation, azimuth, SNR, and signal ID

  count = gps_info->satellite_count < GPS_MAX_SATELLITES ?
          gps_info->satellite_count : GPS_MAX_SATELLITES;
  printf("[GPS][SATELLITES] record_count=%u\n", (unsigned)gps_info->satellite_count);
  for(uint8_t i = 0U; i < count; i++) {
    const gps_satellite_info_t *satellite = &gps_info->satellites[i];
    printf("[GPS][SAT %u] constellation=%s PRN=%u used=%s elevation_present=%s elevation_deg=%d azimuth_present=%s azimuth_deg=%u snr_present=%s snr_dbhz=%u signal_id_present=%s signal_id=%u\n",
           (unsigned)i,
           gps_constellation_text(satellite->constellation),
           (unsigned)satellite->prn,
           gps_bool_text(satellite->used_in_fix),
           gps_bool_text(satellite->has_elevation),
           (int)satellite->elevation_deg,
           gps_bool_text(satellite->has_azimuth),
           (unsigned)satellite->azimuth_deg,
           gps_bool_text(satellite->has_snr),
           (unsigned)satellite->snr_dbhz,
           gps_bool_text(satellite->has_signal_id),
           (unsigned)satellite->signal_id);
  }

  // Log the RMC information, including presence, data status, mode, navigation status, UTC time and date, position, speed, course, and magnetic variation

  printf("[GPS][RMC] present=%s data_status=%c mode=%c navigation_status=%c\n",
         gps_bool_text(gps_info->rmc.present),
         gps_log_char(gps_info->rmc.data_status),
         gps_log_char(gps_info->rmc.mode),
         gps_log_char(gps_info->rmc.navigation_status));
  gps_log_time("RMC", &gps_info->rmc.utc_time);
  gps_log_date("RMC", &gps_info->rmc.utc_date);
  gps_log_position("RMC", &gps_info->rmc.position);
  printf("[GPS][RMC] speed_present=%s speed_knots=%.3f course_present=%s course_true_deg=%.3f magnetic_variation_present=%s magnetic_variation_deg=%.3f direction=%c\n",
         gps_bool_text(gps_info->rmc.has_speed_knots),
         gps_info->rmc.speed_knots,
         gps_bool_text(gps_info->rmc.has_course_true),
         gps_info->rmc.course_true_deg,
         gps_bool_text(gps_info->rmc.has_magnetic_variation),
         gps_info->rmc.magnetic_variation_deg,
         gps_log_char(gps_info->rmc.magnetic_variation_direction));

  printf("[GPS][VTG] present=%s course_true_present=%s course_true_deg=%.3f course_magnetic_present=%s course_magnetic_deg=%.3f\n",
         gps_bool_text(gps_info->vtg.present),
         gps_bool_text(gps_info->vtg.has_course_true),
         gps_info->vtg.course_true_deg,
         gps_bool_text(gps_info->vtg.has_course_magnetic),
         gps_info->vtg.course_magnetic_deg);
  printf("[GPS][VTG] speed_knots_present=%s speed_knots=%.3f speed_kmh_present=%s speed_kmh=%.3f mode=%c\n",
         gps_bool_text(gps_info->vtg.has_speed_knots),
         gps_info->vtg.speed_knots,
         gps_bool_text(gps_info->vtg.has_speed_kmh),
         gps_info->vtg.speed_kmh,
         gps_log_char(gps_info->vtg.mode));

  printf("[GPS][ZDA] present=%s local_zone_hours=%d local_zone_minutes=%u\n",
         gps_bool_text(gps_info->zda.present),
         (int)gps_info->zda.local_zone_hours,
         (unsigned)gps_info->zda.local_zone_minutes);
  gps_log_time("ZDA", &gps_info->zda.utc_time);
  gps_log_date("ZDA", &gps_info->zda.utc_date);

  count = gps_info->txt_count < GPS_MAX_TXT_MESSAGES ? gps_info->txt_count : GPS_MAX_TXT_MESSAGES;
  printf("[GPS][TXT] record_count=%u\n", (unsigned)gps_info->txt_count);
  for(uint8_t i = 0U; i < count; i++) {
    const gps_txt_info_t *txt = &gps_info->txt[i];
    printf("[GPS][TXT %u] present=%s total_messages=%u message_number=%u message_type=%u text=\"%s\"\n",
           (unsigned)i,
           gps_bool_text(txt->present),
           (unsigned)txt->total_messages,
           (unsigned)txt->message_number,
           (unsigned)txt->message_type,
           txt->text);
  }
  printf("[GPS] ================== END FRAME %lu ==================\n\n",
         (unsigned long)gps_info->frame_number);
}

// Power up the GPS module by setting the appropriate GPIO pin and updating the internal state
void GPS_module_powerup(void)
{
  gpio_put(GPS_POWER_UP_PIN, 0);
  module_powered = true;
}

void GPS_module_powerdown(void)
{
  gpio_put(GPS_POWER_UP_PIN, 1);
  
  module_powered = false;
}

bool GPS_module_is_powered(void)
{
  return module_powered;
}
