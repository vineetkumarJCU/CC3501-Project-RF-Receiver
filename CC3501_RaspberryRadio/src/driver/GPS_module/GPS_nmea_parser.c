#include "GPS_nmea_parser.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define GPS_NMEA_MAX_FIELDS 32U

static uint32_t sentence_mask_from_type(const char *type);
static gps_constellation_t constellation_from_talker(const char *talker);
static gps_constellation_t constellation_from_system_id(uint8_t system_id,
                                                         gps_constellation_t fallback);
static uint32_t constellation_mask(gps_constellation_t constellation);
static bool parse_sentence(gps_nmea_parser_t *parser, const char *sentence);
static bool parse_gga(gps_info_t *info, char *const fields[], size_t count);
static bool parse_gll(gps_info_t *info, char *const fields[], size_t count);
static bool parse_gsa(gps_info_t *info, char *const fields[], size_t count,
                      gps_constellation_t talker_constellation);
static bool parse_gsv(gps_info_t *info, char *const fields[], size_t count,
                      const char *talker, gps_constellation_t talker_constellation);
static bool parse_rmc(gps_info_t *info, char *const fields[], size_t count);
static bool parse_vtg(gps_info_t *info, char *const fields[], size_t count);
static bool parse_zda(gps_info_t *info, char *const fields[], size_t count);
static bool parse_txt(gps_info_t *info, char *const fields[], size_t count);
static void build_summary(gps_info_t *info);

static bool parse_uint32(const char *text, uint32_t *value)
{
  char *end = NULL;
  unsigned long parsed;

  if(text == NULL || text[0] == '\0' || value == NULL) return false;
  parsed = strtoul(text, &end, 10);
  if(end == text || *end != '\0') return false;
  *value = (uint32_t)parsed;
  return true;
}

static bool parse_int32(const char *text, int32_t *value)
{
  char *end = NULL;
  long parsed;

  if(text == NULL || text[0] == '\0' || value == NULL) return false;
  parsed = strtol(text, &end, 10);
  if(end == text || *end != '\0') return false;
  *value = (int32_t)parsed;
  return true;
}

static bool parse_float_value(const char *text, float *value)
{
  char *end = NULL;
  float parsed;

  if(text == NULL || text[0] == '\0' || value == NULL) return false;
  parsed = strtof(text, &end);
  if(end == text || *end != '\0') return false;
  *value = parsed;
  return true;
}

static bool parse_double_value(const char *text, double *value)
{
  char *end = NULL;
  double parsed;

  if(text == NULL || text[0] == '\0' || value == NULL) return false;
  parsed = strtod(text, &end);
  if(end == text || *end != '\0') return false;
  *value = parsed;
  return true;
}

static bool parse_utc_time(const char *text, gps_utc_time_t *utc_time)
{
  double raw;
  int whole;
  int second;
  int millisecond;

  if(utc_time == NULL || !parse_double_value(text, &raw) || raw < 0.0) return false;
  whole = (int)raw;
  utc_time->hour = (uint8_t)(whole / 10000);
  utc_time->minute = (uint8_t)((whole / 100) % 100);
  second = whole % 100;
  millisecond = (int)(((raw - (double)whole) * 1000.0) + 0.5);
  if(millisecond >= 1000) {
    millisecond = 0;
    second++;
  }
  if(utc_time->hour > 23U || utc_time->minute > 59U || second > 60) return false;
  utc_time->second = (uint8_t)second;
  utc_time->millisecond = (uint16_t)millisecond;
  utc_time->valid = true;
  return true;
}

static bool parse_rmc_date(const char *text, gps_utc_date_t *utc_date)
{
  uint8_t day;
  uint8_t month;
  uint8_t year;

  if(text == NULL || utc_date == NULL || strlen(text) != 6U) return false;
  for(size_t i = 0U; i < 6U; i++) {
    if(!isdigit((unsigned char)text[i])) return false;
  }
  day = (uint8_t)((text[0] - '0') * 10 + (text[1] - '0'));
  month = (uint8_t)((text[2] - '0') * 10 + (text[3] - '0'));
  year = (uint8_t)((text[4] - '0') * 10 + (text[5] - '0'));
  if(day == 0U || day > 31U || month == 0U || month > 12U) return false;
  utc_date->day = day;
  utc_date->month = month;
  utc_date->year = year >= 80U ? (uint16_t)(1900U + year) : (uint16_t)(2000U + year);
  utc_date->valid = true;
  return true;
}

static bool parse_position(const char *latitude,
                           const char *latitude_hemisphere,
                           const char *longitude,
                           const char *longitude_hemisphere,
                           gps_position_t *position)
{
  double latitude_raw;
  double longitude_raw;
  int latitude_degrees;
  int longitude_degrees;
  double latitude_minutes;
  double longitude_minutes;
  char lat_hemi;
  char lon_hemi;

  if(position == NULL || !parse_double_value(latitude, &latitude_raw) ||
     !parse_double_value(longitude, &longitude_raw) ||
     latitude_hemisphere == NULL || longitude_hemisphere == NULL ||
     latitude_hemisphere[0] == '\0' || longitude_hemisphere[0] == '\0') {
    return false;
  }

  lat_hemi = latitude_hemisphere[0];
  lon_hemi = longitude_hemisphere[0];
  if((lat_hemi != 'N' && lat_hemi != 'S') || (lon_hemi != 'E' && lon_hemi != 'W')) return false;

  latitude_degrees = (int)(latitude_raw / 100.0);
  longitude_degrees = (int)(longitude_raw / 100.0);
  latitude_minutes = latitude_raw - ((double)latitude_degrees * 100.0);
  longitude_minutes = longitude_raw - ((double)longitude_degrees * 100.0);
  if(latitude_degrees > 90 || longitude_degrees > 180 ||
     latitude_minutes < 0.0 || latitude_minutes >= 60.0 ||
     longitude_minutes < 0.0 || longitude_minutes >= 60.0) {
    return false;
  }

  position->latitude_deg = (double)latitude_degrees + latitude_minutes / 60.0;
  position->longitude_deg = (double)longitude_degrees + longitude_minutes / 60.0;
  if(lat_hemi == 'S') position->latitude_deg = -position->latitude_deg;
  if(lon_hemi == 'W') position->longitude_deg = -position->longitude_deg;
  position->latitude_hemisphere = lat_hemi;
  position->longitude_hemisphere = lon_hemi;
  position->valid = true;
  return true;
}

static int hex_value(char ch)
{
  if(ch >= '0' && ch <= '9') return ch - '0';
  if(ch >= 'A' && ch <= 'F') return 10 + ch - 'A';
  if(ch >= 'a' && ch <= 'f') return 10 + ch - 'a';
  return -1;
}

static bool checksum_is_valid(const char *sentence, const char **star_out)
{
  const char *star;
  uint8_t checksum = 0U;
  int high;
  int low;

  if(sentence == NULL || sentence[0] != '$') return false;
  star = strchr(sentence, '*');
  if(star == NULL || star[1] == '\0' || star[2] == '\0' || star[3] != '\0') return false;
  high = hex_value(star[1]);
  low = hex_value(star[2]);
  if(high < 0 || low < 0) return false;
  for(const char *cursor = sentence + 1; cursor < star; cursor++) checksum ^= (uint8_t)*cursor;
  if(star_out != NULL) *star_out = star;
  return checksum == (uint8_t)((high << 4) | low);
}

static size_t split_fields(char *payload, char *fields[GPS_NMEA_MAX_FIELDS])
{
  size_t count = 0U;
  char *cursor = payload;

  while(count < GPS_NMEA_MAX_FIELDS) {
    fields[count++] = cursor;
    cursor = strchr(cursor, ',');
    if(cursor == NULL) break;
    *cursor = '\0';
    cursor++;
  }
  return count;
}

static uint32_t sentence_mask_from_type(const char *type)
{
  if(type == NULL) return 0U;
  if(strcmp(type, "GGA") == 0) return GPS_SENTENCE_GGA;
  if(strcmp(type, "GLL") == 0) return GPS_SENTENCE_GLL;
  if(strcmp(type, "GSA") == 0) return GPS_SENTENCE_GSA;
  if(strcmp(type, "GSV") == 0) return GPS_SENTENCE_GSV;
  if(strcmp(type, "RMC") == 0) return GPS_SENTENCE_RMC;
  if(strcmp(type, "VTG") == 0) return GPS_SENTENCE_VTG;
  if(strcmp(type, "ZDA") == 0) return GPS_SENTENCE_ZDA;
  if(strcmp(type, "TXT") == 0) return GPS_SENTENCE_TXT;
  return 0U;
}

static gps_constellation_t constellation_from_talker(const char *talker)
{
  if(talker == NULL) return GPS_CONSTELLATION_UNKNOWN;
  if(talker[0] == 'G' && talker[1] == 'P') return GPS_CONSTELLATION_GPS;
  if((talker[0] == 'B' && talker[1] == 'D') || (talker[0] == 'G' && talker[1] == 'B')) return GPS_CONSTELLATION_BEIDOU;
  if(talker[0] == 'G' && talker[1] == 'L') return GPS_CONSTELLATION_GLONASS;
  if(talker[0] == 'G' && talker[1] == 'A') return GPS_CONSTELLATION_GALILEO;
  if(talker[0] == 'G' && talker[1] == 'Q') return GPS_CONSTELLATION_QZSS;
  if(talker[0] == 'G' && talker[1] == 'I') return GPS_CONSTELLATION_NAVIC;
  if(talker[0] == 'G' && talker[1] == 'N') return GPS_CONSTELLATION_MIXED;
  return GPS_CONSTELLATION_UNKNOWN;
}

static gps_constellation_t constellation_from_system_id(uint8_t system_id,
                                                         gps_constellation_t fallback)
{
  switch(system_id) {
    case 1U: return GPS_CONSTELLATION_GPS;
    case 2U: return GPS_CONSTELLATION_GLONASS;
    case 3U: return GPS_CONSTELLATION_GALILEO;
    case 4U: return GPS_CONSTELLATION_BEIDOU;
    case 5U: return GPS_CONSTELLATION_QZSS;
    case 6U: return GPS_CONSTELLATION_NAVIC;
    default: return fallback;
  }
}

static uint32_t constellation_mask(gps_constellation_t constellation)
{
  switch(constellation) {
    case GPS_CONSTELLATION_GPS: return GPS_SYSTEM_GPS;
    case GPS_CONSTELLATION_GLONASS: return GPS_SYSTEM_GLONASS;
    case GPS_CONSTELLATION_BEIDOU: return GPS_SYSTEM_BEIDOU;
    case GPS_CONSTELLATION_GALILEO: return GPS_SYSTEM_GALILEO;
    case GPS_CONSTELLATION_QZSS: return GPS_SYSTEM_QZSS;
    case GPS_CONSTELLATION_NAVIC: return GPS_SYSTEM_NAVIC;
    case GPS_CONSTELLATION_MIXED: return GPS_SYSTEM_MIXED;
    default: return 0U;
  }
}

static bool parse_gga(gps_info_t *info, char *const fields[], size_t count)
{
  gps_gga_info_t *gga = &info->gga;
  uint32_t value;

  if(count < 15U) return false;
  memset(gga, 0, sizeof(*gga));
  gga->present = true;
  (void)parse_utc_time(fields[1], &gga->utc_time);
  (void)parse_position(fields[2], fields[3], fields[4], fields[5], &gga->position);
  if(parse_uint32(fields[6], &value)) gga->fix_quality = (uint8_t)value;
  if(parse_uint32(fields[7], &value)) gga->satellites_used = (uint8_t)value;
  gga->has_hdop = parse_float_value(fields[8], &gga->hdop);
  gga->has_altitude_msl = parse_float_value(fields[9], &gga->altitude_msl_m);
  gga->has_geoid_separation = parse_float_value(fields[11], &gga->geoid_separation_m);
  gga->has_differential_age = parse_float_value(fields[13], &gga->differential_age_s);
  gga->has_differential_station_id = parse_uint32(fields[14], &value);
  if(gga->has_differential_station_id) gga->differential_station_id = (uint16_t)value;
  return true;
}

static bool parse_gll(gps_info_t *info, char *const fields[], size_t count)
{
  gps_gll_info_t *gll = &info->gll;

  if(count < 7U) return false;
  memset(gll, 0, sizeof(*gll));
  gll->present = true;
  (void)parse_position(fields[1], fields[2], fields[3], fields[4], &gll->position);
  (void)parse_utc_time(fields[5], &gll->utc_time);
  gll->data_status = fields[6][0];
  if(count > 7U) gll->mode = fields[7][0];
  return true;
}

static bool parse_gsa(gps_info_t *info, char *const fields[], size_t count,
                      gps_constellation_t talker_constellation)
{
  gps_gsa_info_t *gsa;
  uint32_t value;

  if(count < 18U || info->gsa_count >= GPS_MAX_GSA_RECORDS) return false;
  gsa = &info->gsa[info->gsa_count];
  memset(gsa, 0, sizeof(*gsa));
  gsa->present = true;
  gsa->selection_mode = fields[1][0];
  if(parse_uint32(fields[2], &value)) gsa->fix_type = (uint8_t)value;
  for(size_t index = 3U; index <= 14U; index++) {
    if(parse_uint32(fields[index], &value) && gsa->satellite_count < GPS_MAX_GSA_SATELLITES) {
      gsa->satellite_prn[gsa->satellite_count++] = (uint16_t)value;
    }
  }
  gsa->has_pdop = parse_float_value(fields[15], &gsa->pdop);
  gsa->has_hdop = parse_float_value(fields[16], &gsa->hdop);
  gsa->has_vdop = parse_float_value(fields[17], &gsa->vdop);
  if(count > 18U && parse_uint32(fields[18], &value)) gsa->system_id = (uint8_t)value;
  gsa->constellation = constellation_from_system_id(gsa->system_id, talker_constellation);
  info->gsa_count++;
  return true;
}

static int find_gsv_group(const gps_info_t *info,
                          gps_constellation_t constellation,
                          bool has_signal_id,
                          uint8_t signal_id)
{
  for(uint8_t i = 0U; i < info->gsv_group_count; i++) {
    const gps_gsv_group_info_t *group = &info->gsv_groups[i];
    if(group->constellation == constellation && group->has_signal_id == has_signal_id &&
       (!has_signal_id || group->signal_id == signal_id)) return (int)i;
  }
  return -1;
}

static bool parse_gsv(gps_info_t *info, char *const fields[], size_t count,
                      const char *talker, gps_constellation_t talker_constellation)
{
  size_t trailing_fields;
  size_t satellite_fields;
  size_t satellite_group_count;
  bool has_signal_id;
  uint8_t signal_id = 0U;
  uint32_t value;
  uint8_t total_messages;
  uint8_t message_number;
  uint16_t satellites_in_view;
  int group_index;
  gps_gsv_group_info_t *group;

  if(count < 4U || !parse_uint32(fields[1], &value)) return false;
  total_messages = (uint8_t)value;
  if(!parse_uint32(fields[2], &value)) return false;
  message_number = (uint8_t)value;
  if(!parse_uint32(fields[3], &value)) return false;
  satellites_in_view = (uint16_t)value;

  trailing_fields = count - 4U;
  has_signal_id = (trailing_fields % 4U) == 1U;
  satellite_fields = trailing_fields - (has_signal_id ? 1U : 0U);
  satellite_group_count = satellite_fields / 4U;
  if(has_signal_id && parse_uint32(fields[count - 1U], &value)) signal_id = (uint8_t)value;

  group_index = find_gsv_group(info, talker_constellation, has_signal_id, signal_id);
  if(group_index < 0) {
    if(info->gsv_group_count >= GPS_MAX_GSV_GROUPS) return false;
    group_index = (int)info->gsv_group_count++;
    memset(&info->gsv_groups[group_index], 0, sizeof(info->gsv_groups[group_index]));
  }
  group = &info->gsv_groups[group_index];
  group->present = true;
  group->talker[0] = talker[0];
  group->talker[1] = talker[1];
  group->talker[2] = '\0';
  group->constellation = talker_constellation;
  group->total_messages = total_messages;
  group->satellites_in_view = satellites_in_view;
  group->has_signal_id = has_signal_id;
  group->signal_id = signal_id;
  if(message_number > 0U && message_number <= 32U) group->received_message_mask |= 1UL << (message_number - 1U);

  for(size_t satellite_index = 0U; satellite_index < satellite_group_count; satellite_index++) {
    size_t base = 4U + satellite_index * 4U;
    gps_satellite_info_t *satellite;
    int32_t signed_value;

    if(!parse_uint32(fields[base], &value)) continue;
    if(info->satellite_count >= GPS_MAX_SATELLITES) break;
    satellite = &info->satellites[info->satellite_count++];
    memset(satellite, 0, sizeof(*satellite));
    satellite->constellation = talker_constellation;
    satellite->prn = (uint16_t)value;
    satellite->has_elevation = parse_int32(fields[base + 1U], &signed_value);
    if(satellite->has_elevation) satellite->elevation_deg = (int8_t)signed_value;
    satellite->has_azimuth = parse_uint32(fields[base + 2U], &value);
    if(satellite->has_azimuth) satellite->azimuth_deg = (uint16_t)value;
    satellite->has_snr = parse_uint32(fields[base + 3U], &value);
    if(satellite->has_snr) satellite->snr_dbhz = (uint8_t)value;
    satellite->has_signal_id = has_signal_id;
    satellite->signal_id = signal_id;
  }
  return true;
}

static bool parse_rmc(gps_info_t *info, char *const fields[], size_t count)
{
  gps_rmc_info_t *rmc = &info->rmc;
  float magnetic_variation;

  if(count < 12U) return false;
  memset(rmc, 0, sizeof(*rmc));
  rmc->present = true;
  (void)parse_utc_time(fields[1], &rmc->utc_time);
  rmc->data_status = fields[2][0];
  (void)parse_position(fields[3], fields[4], fields[5], fields[6], &rmc->position);
  rmc->has_speed_knots = parse_float_value(fields[7], &rmc->speed_knots);
  rmc->has_course_true = parse_float_value(fields[8], &rmc->course_true_deg);
  (void)parse_rmc_date(fields[9], &rmc->utc_date);
  rmc->has_magnetic_variation = parse_float_value(fields[10], &magnetic_variation);
  rmc->magnetic_variation_direction = fields[11][0];
  if(rmc->has_magnetic_variation) {
    rmc->magnetic_variation_deg = magnetic_variation;
    if(rmc->magnetic_variation_direction == 'W') rmc->magnetic_variation_deg = -magnetic_variation;
  }
  if(count > 12U) rmc->mode = fields[12][0];
  if(count > 13U) rmc->navigation_status = fields[13][0];
  return true;
}

static bool parse_vtg(gps_info_t *info, char *const fields[], size_t count)
{
  gps_vtg_info_t *vtg = &info->vtg;

  if(count < 9U) return false;
  memset(vtg, 0, sizeof(*vtg));
  vtg->present = true;
  vtg->has_course_true = parse_float_value(fields[1], &vtg->course_true_deg);
  vtg->has_course_magnetic = parse_float_value(fields[3], &vtg->course_magnetic_deg);
  vtg->has_speed_knots = parse_float_value(fields[5], &vtg->speed_knots);
  vtg->has_speed_kmh = parse_float_value(fields[7], &vtg->speed_kmh);
  if(count > 9U) vtg->mode = fields[9][0];
  return true;
}

static bool parse_zda(gps_info_t *info, char *const fields[], size_t count)
{
  gps_zda_info_t *zda = &info->zda;
  uint32_t value;
  int32_t signed_value;

  if(count < 7U) return false;
  memset(zda, 0, sizeof(*zda));
  zda->present = true;
  (void)parse_utc_time(fields[1], &zda->utc_time);
  if(parse_uint32(fields[2], &value)) zda->utc_date.day = (uint8_t)value;
  if(parse_uint32(fields[3], &value)) zda->utc_date.month = (uint8_t)value;
  if(parse_uint32(fields[4], &value)) zda->utc_date.year = (uint16_t)value;
  zda->utc_date.valid = zda->utc_date.year > 0U && zda->utc_date.month > 0U &&
                        zda->utc_date.month <= 12U && zda->utc_date.day > 0U &&
                        zda->utc_date.day <= 31U;
  if(parse_int32(fields[5], &signed_value)) zda->local_zone_hours = (int8_t)signed_value;
  if(parse_uint32(fields[6], &value)) zda->local_zone_minutes = (uint8_t)value;
  return true;
}

static bool parse_txt(gps_info_t *info, char *const fields[], size_t count)
{
  gps_txt_info_t *txt;
  uint32_t value;
  size_t length = 0U;

  if(count < 5U || info->txt_count >= GPS_MAX_TXT_MESSAGES) return false;
  txt = &info->txt[info->txt_count];
  memset(txt, 0, sizeof(*txt));
  txt->present = true;
  if(parse_uint32(fields[1], &value)) txt->total_messages = (uint8_t)value;
  if(parse_uint32(fields[2], &value)) txt->message_number = (uint8_t)value;
  if(parse_uint32(fields[3], &value)) txt->message_type = (uint8_t)value;
  for(size_t i = 4U; i < count && length < GPS_TXT_TEXT_LENGTH - 1U; i++) {
    if(i > 4U && length < GPS_TXT_TEXT_LENGTH - 1U) txt->text[length++] = ',';
    for(size_t j = 0U; fields[i][j] != '\0' && length < GPS_TXT_TEXT_LENGTH - 1U; j++) {
      txt->text[length++] = fields[i][j];
    }
  }
  txt->text[length] = '\0';
  info->txt_count++;
  return true;
}

static bool parse_sentence(gps_nmea_parser_t *parser, const char *sentence)
{
  gps_info_t *info = &parser->building;
  const char *star = NULL;
  char payload[GPS_NMEA_MAX_SENTENCE_LENGTH];
  char *fields[GPS_NMEA_MAX_FIELDS];
  char type[4] = {0};
  char talker[3] = {0};
  size_t payload_length;
  size_t field_count;
  uint32_t mask = 0U;
  bool parsed = false;

  if(sentence != NULL && strlen(sentence) >= 6U && sentence[0] == '$') {
    type[0] = sentence[3];
    type[1] = sentence[4];
    type[2] = sentence[5];
    mask = sentence_mask_from_type(type);
    info->received_sentence_mask |= mask;
  }
  if(!checksum_is_valid(sentence, &star)) {
    info->received_sentence_count++;
    info->received_sentence_mask |= mask;
    info->checksum_error_count++;
    return false;
  }

  payload_length = (size_t)(star - (sentence + 1));
  if(payload_length == 0U || payload_length >= sizeof(payload)) {
    info->parse_error_count++;
    return false;
  }
  memcpy(payload, sentence + 1, payload_length);
  payload[payload_length] = '\0';
  field_count = split_fields(payload, fields);
  if(field_count == 0U || strlen(fields[0]) != 5U) {
    info->parse_error_count++;
    return false;
  }
  talker[0] = fields[0][0];
  talker[1] = fields[0][1];
  memcpy(type, fields[0] + 2, 3U);
  mask = sentence_mask_from_type(type);

  /* GGA is the first sentence in the module's burst. If bursts run back to
   * back without a detectable UART idle gap, a new GGA still closes the
   * preceding one-second frame deterministically. */
  if(strcmp(type, "GGA") == 0 && info->gga.present && info->received_sentence_count > 0U) {
    build_summary(info);
    parser->completed = *info;
    parser->completed_ready = true;
    memset(info, 0, sizeof(*info));
  }

  info->received_sentence_count++;
  info->received_sentence_mask |= mask;
  info->valid_sentence_count++;
  info->valid_sentence_mask |= mask;

  if(strcmp(type, "GGA") == 0) parsed = parse_gga(info, fields, field_count);
  else if(strcmp(type, "GLL") == 0) parsed = parse_gll(info, fields, field_count);
  else if(strcmp(type, "GSA") == 0) parsed = parse_gsa(info, fields, field_count, constellation_from_talker(talker));
  else if(strcmp(type, "GSV") == 0) parsed = parse_gsv(info, fields, field_count, talker, constellation_from_talker(talker));
  else if(strcmp(type, "RMC") == 0) parsed = parse_rmc(info, fields, field_count);
  else if(strcmp(type, "VTG") == 0) parsed = parse_vtg(info, fields, field_count);
  else if(strcmp(type, "ZDA") == 0) parsed = parse_zda(info, fields, field_count);
  else if(strcmp(type, "TXT") == 0) parsed = parse_txt(info, fields, field_count);
  else {
    info->unsupported_sentence_count++;
    return true;
  }

  if(!parsed) info->parse_error_count++;
  return parsed;
}

static bool satellite_is_used(const gps_info_t *info, const gps_satellite_info_t *satellite)
{
  for(uint8_t gsa_index = 0U; gsa_index < info->gsa_count; gsa_index++) {
    const gps_gsa_info_t *gsa = &info->gsa[gsa_index];
    if(gsa->constellation != GPS_CONSTELLATION_MIXED &&
       gsa->constellation != GPS_CONSTELLATION_UNKNOWN &&
       gsa->constellation != satellite->constellation) continue;
    for(uint8_t satellite_index = 0U; satellite_index < gsa->satellite_count; satellite_index++) {
      if(gsa->satellite_prn[satellite_index] == satellite->prn) return true;
    }
  }
  return false;
}

static void build_summary(gps_info_t *info)
{
  uint16_t constellation_view_count[GPS_CONSTELLATION_MIXED + 1U] = {0};

  if(info->rmc.present && info->rmc.position.valid) info->position = info->rmc.position;
  else if(info->gga.present && info->gga.position.valid) info->position = info->gga.position;
  else if(info->gll.present && info->gll.position.valid) info->position = info->gll.position;

  info->position_valid = info->position.valid &&
                         ((info->gga.present && info->gga.fix_quality > 0U) ||
                          (info->rmc.present && info->rmc.data_status == 'A') ||
                          (info->gll.present && info->gll.data_status == 'A'));

  if(info->zda.utc_time.valid) info->utc_time = info->zda.utc_time;
  else if(info->rmc.utc_time.valid) info->utc_time = info->rmc.utc_time;
  else if(info->gga.utc_time.valid) info->utc_time = info->gga.utc_time;
  else if(info->gll.utc_time.valid) info->utc_time = info->gll.utc_time;
  if(info->zda.utc_date.valid) info->utc_date = info->zda.utc_date;
  else if(info->rmc.utc_date.valid) info->utc_date = info->rmc.utc_date;

  if(info->gga.present) {
    info->fix_quality = info->gga.fix_quality;
    info->satellites_used = info->gga.satellites_used;
    info->has_altitude_msl = info->gga.has_altitude_msl;
    info->altitude_msl_m = info->gga.altitude_msl_m;
    if(info->gga.has_altitude_msl && info->gga.has_geoid_separation) {
      info->has_altitude_ellipsoid = true;
      info->altitude_ellipsoid_m = info->gga.altitude_msl_m + info->gga.geoid_separation_m;
    }
  }

  for(uint8_t i = 0U; i < info->gsa_count; i++) {
    const gps_gsa_info_t *gsa = &info->gsa[i];
    if(gsa->fix_type > info->fix_type) info->fix_type = gsa->fix_type;
    info->constellation_mask |= constellation_mask(gsa->constellation);
  }

  for(uint8_t i = 0U; i < info->gsv_group_count; i++) {
    const gps_gsv_group_info_t *group = &info->gsv_groups[i];
    info->constellation_mask |= constellation_mask(group->constellation);
    if(group->constellation <= GPS_CONSTELLATION_MIXED &&
       group->satellites_in_view > constellation_view_count[group->constellation]) {
      constellation_view_count[group->constellation] = group->satellites_in_view;
    }
  }
  for(size_t i = 0U; i <= GPS_CONSTELLATION_MIXED; i++) info->satellites_in_view += constellation_view_count[i];
  for(uint8_t i = 0U; i < info->satellite_count; i++) info->satellites[i].used_in_fix = satellite_is_used(info, &info->satellites[i]);

  if(info->vtg.has_speed_knots) {
    info->has_speed_knots = true;
    info->speed_knots = info->vtg.speed_knots;
  } else if(info->rmc.has_speed_knots) {
    info->has_speed_knots = true;
    info->speed_knots = info->rmc.speed_knots;
  }
  if(info->vtg.has_speed_kmh) {
    info->has_speed_kmh = true;
    info->speed_kmh = info->vtg.speed_kmh;
  } else if(info->has_speed_knots) {
    info->has_speed_kmh = true;
    info->speed_kmh = info->speed_knots * 1.852f;
  }
  if(info->vtg.has_course_true) {
    info->has_course_true = true;
    info->course_true_deg = info->vtg.course_true_deg;
  } else if(info->rmc.has_course_true) {
    info->has_course_true = true;
    info->course_true_deg = info->rmc.course_true_deg;
  }
  if(info->rmc.has_magnetic_variation) {
    info->has_magnetic_variation = true;
    info->magnetic_variation_deg = info->rmc.magnetic_variation_deg;
    info->magnetic_variation_direction = info->rmc.magnetic_variation_direction;
  }

  info->location_mode = info->rmc.mode != '\0' ? info->rmc.mode :
                        (info->gll.mode != '\0' ? info->gll.mode : info->vtg.mode);
  info->location_status = info->rmc.data_status != '\0' ? info->rmc.data_status : info->gll.data_status;
  info->navigation_status = info->rmc.navigation_status;

  for(uint8_t i = 0U; i < info->txt_count; i++) {
    if(strstr(info->txt[i].text, "ANTENNA OK") != NULL) info->antenna_status = GPS_ANTENNA_OK;
    else if(strstr(info->txt[i].text, "ANTENNA OPEN") != NULL) info->antenna_status = GPS_ANTENNA_OPEN;
    else if(strstr(info->txt[i].text, "ANTENNA SHORT") != NULL) info->antenna_status = GPS_ANTENNA_SHORT;
  }
}

void gps_nmea_parser_init(gps_nmea_parser_t *parser)
{
  if(parser == NULL) return;
  memset(parser, 0, sizeof(*parser));
}

void gps_nmea_parser_feed_byte(gps_nmea_parser_t *parser, uint8_t byte)
{
  if(parser == NULL) return;

  if(byte == '$') {
    parser->sentence[0] = '$';
    parser->sentence_length = 1U;
    parser->collecting_sentence = true;
    parser->sentence_overflow = false;
    return;
  }
  if(!parser->collecting_sentence) return;
  if(byte == '\r' || byte == '\n') {
    if(!parser->sentence_overflow && parser->sentence_length > 1U) {
      parser->sentence[parser->sentence_length] = '\0';
      (void)parse_sentence(parser, parser->sentence);
    } else if(parser->sentence_overflow) {
      parser->building.received_sentence_count++;
      parser->building.parse_error_count++;
    }
    parser->collecting_sentence = false;
    parser->sentence_length = 0U;
    return;
  }
  if(parser->sentence_length < GPS_NMEA_MAX_SENTENCE_LENGTH - 1U) {
    parser->sentence[parser->sentence_length++] = (char)byte;
  } else {
    parser->sentence_overflow = true;
  }
}

bool gps_nmea_parser_has_frame_data(const gps_nmea_parser_t *parser)
{
  return parser != NULL && parser->building.received_sentence_count > 0U;
}

bool gps_nmea_parser_take_completed_frame(gps_nmea_parser_t *parser, gps_info_t *gps_info)
{
  if(parser == NULL || gps_info == NULL || !parser->completed_ready) return false;
  *gps_info = parser->completed;
  parser->completed_ready = false;
  memset(&parser->completed, 0, sizeof(parser->completed));
  return true;
}

bool gps_nmea_parser_finish_frame(gps_nmea_parser_t *parser, gps_info_t *gps_info)
{
  if(parser == NULL || gps_info == NULL || !gps_nmea_parser_has_frame_data(parser)) return false;
  build_summary(&parser->building);
  *gps_info = parser->building;
  memset(&parser->building, 0, sizeof(parser->building));
  return true;
}
