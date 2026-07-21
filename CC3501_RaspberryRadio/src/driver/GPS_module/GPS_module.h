#ifndef GPS_MODULE_H
#define GPS_MODULE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GPS_MAX_GSA_RECORDS                 8U
#define GPS_MAX_GSV_GROUPS                  8U
#define GPS_MAX_SATELLITES                  48U
#define GPS_MAX_GSA_SATELLITES              12U
#define GPS_MAX_TXT_MESSAGES                4U
#define GPS_TXT_TEXT_LENGTH                 64U

#define GPS_SENTENCE_GGA                    (1UL << 0)
#define GPS_SENTENCE_GLL                    (1UL << 1)
#define GPS_SENTENCE_GSA                    (1UL << 2)
#define GPS_SENTENCE_GSV                    (1UL << 3)
#define GPS_SENTENCE_RMC                    (1UL << 4)
#define GPS_SENTENCE_VTG                    (1UL << 5)
#define GPS_SENTENCE_ZDA                    (1UL << 6)
#define GPS_SENTENCE_TXT                    (1UL << 7)

#define GPS_SYSTEM_GPS                      (1UL << 0)
#define GPS_SYSTEM_GLONASS                  (1UL << 1)
#define GPS_SYSTEM_BEIDOU                   (1UL << 2)
#define GPS_SYSTEM_GALILEO                  (1UL << 3)
#define GPS_SYSTEM_QZSS                     (1UL << 4)
#define GPS_SYSTEM_NAVIC                    (1UL << 5)
#define GPS_SYSTEM_MIXED                    (1UL << 6)

typedef enum
{
  GPS_CONSTELLATION_UNKNOWN = 0,
  GPS_CONSTELLATION_GPS,
  GPS_CONSTELLATION_GLONASS,
  GPS_CONSTELLATION_BEIDOU,
  GPS_CONSTELLATION_GALILEO,
  GPS_CONSTELLATION_QZSS,
  GPS_CONSTELLATION_NAVIC,
  GPS_CONSTELLATION_MIXED
} gps_constellation_t;

typedef enum
{
  GPS_ANTENNA_UNKNOWN = 0,
  GPS_ANTENNA_OK,
  GPS_ANTENNA_OPEN,
  GPS_ANTENNA_SHORT
} gps_antenna_status_t;

typedef struct
{
  bool valid;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
  uint16_t millisecond;
} gps_utc_time_t;

typedef struct
{
  bool valid;
  uint16_t year;
  uint8_t month;
  uint8_t day;
} gps_utc_date_t;

typedef struct
{
  bool valid;
  double latitude_deg;
  double longitude_deg;
  char latitude_hemisphere;
  char longitude_hemisphere;
} gps_position_t;

typedef struct
{
  bool present;
  gps_utc_time_t utc_time;
  gps_position_t position;
  uint8_t fix_quality;
  uint8_t satellites_used;
  bool has_hdop;
  float hdop;
  bool has_altitude_msl;
  float altitude_msl_m;
  bool has_geoid_separation;
  float geoid_separation_m;
  bool has_differential_age;
  float differential_age_s;
  bool has_differential_station_id;
  uint16_t differential_station_id;
} gps_gga_info_t;

typedef struct
{
  bool present;
  gps_position_t position;
  gps_utc_time_t utc_time;
  char data_status;
  char mode;
} gps_gll_info_t;

typedef struct
{
  bool present;
  char selection_mode;
  uint8_t fix_type;
  gps_constellation_t constellation;
  uint8_t system_id;
  uint16_t satellite_prn[GPS_MAX_GSA_SATELLITES];
  uint8_t satellite_count;
  bool has_pdop;
  bool has_hdop;
  bool has_vdop;
  float pdop;
  float hdop;
  float vdop;
} gps_gsa_info_t;

typedef struct
{
  gps_constellation_t constellation;
  uint16_t prn;
  bool used_in_fix;
  bool has_elevation;
  int8_t elevation_deg;
  bool has_azimuth;
  uint16_t azimuth_deg;
  bool has_snr;
  uint8_t snr_dbhz;
  bool has_signal_id;
  uint8_t signal_id;
} gps_satellite_info_t;

typedef struct
{
  bool present;
  char talker[3];
  gps_constellation_t constellation;
  uint8_t total_messages;
  uint32_t received_message_mask;
  uint16_t satellites_in_view;
  bool has_signal_id;
  uint8_t signal_id;
} gps_gsv_group_info_t;

typedef struct
{
  bool present;
  gps_utc_time_t utc_time;
  char data_status;
  gps_position_t position;
  bool has_speed_knots;
  float speed_knots;
  bool has_course_true;
  float course_true_deg;
  gps_utc_date_t utc_date;
  bool has_magnetic_variation;
  float magnetic_variation_deg;
  char magnetic_variation_direction;
  char mode;
  char navigation_status;
} gps_rmc_info_t;

typedef struct
{
  bool present;
  bool has_course_true;
  float course_true_deg;
  bool has_course_magnetic;
  float course_magnetic_deg;
  bool has_speed_knots;
  float speed_knots;
  bool has_speed_kmh;
  float speed_kmh;
  char mode;
} gps_vtg_info_t;

typedef struct
{
  bool present;
  gps_utc_time_t utc_time;
  gps_utc_date_t utc_date;
  int8_t local_zone_hours;
  uint8_t local_zone_minutes;
} gps_zda_info_t;

typedef struct
{
  bool present;
  uint8_t total_messages;
  uint8_t message_number;
  uint8_t message_type;
  char text[GPS_TXT_TEXT_LENGTH];
} gps_txt_info_t;

/** One complete, checksum-verified one-second receiver snapshot. */
typedef struct
{
  uint32_t frame_number;
  uint64_t frame_received_time_us;
  uint32_t received_sentence_mask;
  uint32_t valid_sentence_mask;
  uint16_t received_sentence_count;
  uint16_t valid_sentence_count;
  uint16_t checksum_error_count;
  uint16_t parse_error_count;
  uint16_t unsupported_sentence_count;

  bool pps_seen;
  bool pps_level;
  uint32_t pps_count;
  uint64_t last_pps_time_us;

  bool position_valid;
  gps_position_t position;
  gps_utc_time_t utc_time;
  gps_utc_date_t utc_date;
  uint8_t fix_quality;
  uint8_t fix_type;
  uint8_t satellites_used;
  uint16_t satellites_in_view;
  uint32_t constellation_mask;
  bool has_altitude_msl;
  float altitude_msl_m;
  bool has_altitude_ellipsoid;
  float altitude_ellipsoid_m;
  bool has_speed_knots;
  float speed_knots;
  bool has_speed_kmh;
  float speed_kmh;
  bool has_course_true;
  float course_true_deg;
  bool has_magnetic_variation;
  float magnetic_variation_deg;
  char magnetic_variation_direction;
  char location_mode;
  char location_status;
  char navigation_status;
  gps_antenna_status_t antenna_status;

  gps_gga_info_t gga;
  gps_gll_info_t gll;
  gps_gsa_info_t gsa[GPS_MAX_GSA_RECORDS];
  uint8_t gsa_count;
  gps_gsv_group_info_t gsv_groups[GPS_MAX_GSV_GROUPS];
  uint8_t gsv_group_count;
  gps_satellite_info_t satellites[GPS_MAX_SATELLITES];
  uint8_t satellite_count;
  gps_rmc_info_t rmc;
  gps_vtg_info_t vtg;
  gps_zda_info_t zda;
  gps_txt_info_t txt[GPS_MAX_TXT_MESSAGES];
  uint8_t txt_count;
} gps_info_t;

/** Initialize the UART receiver/parser state and PPS rising-edge capture. */
void GPS_module_init(void);

/** Drain UART1 and publish a frame after the module's one-second burst goes idle. */
bool GPS_module_process(void);

/** Copy the latest completed frame. Returns false until the first frame arrives. */
bool GPS_module_get_latest_info(gps_info_t *gps_info);

/** Print every decoded field in one GPS frame using printf(). */
void GPS_module_log_all_info(const gps_info_t *gps_info);

/** Control the already-initialized active-low module power pin. */
void GPS_module_powerup(void);
void GPS_module_powerdown(void);
bool GPS_module_is_powered(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* GPS_MODULE_H */
