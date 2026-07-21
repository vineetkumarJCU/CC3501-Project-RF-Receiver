#ifndef GPS_NMEA_PARSER_H
#define GPS_NMEA_PARSER_H

#include "GPS_module.h"

#define GPS_NMEA_MAX_SENTENCE_LENGTH 128U

typedef struct
{
  gps_info_t building;
  gps_info_t completed;
  bool completed_ready;
  char sentence[GPS_NMEA_MAX_SENTENCE_LENGTH];
  size_t sentence_length;
  bool collecting_sentence;
  bool sentence_overflow;
} gps_nmea_parser_t;

void gps_nmea_parser_init(gps_nmea_parser_t *parser);
void gps_nmea_parser_feed_byte(gps_nmea_parser_t *parser, uint8_t byte);
bool gps_nmea_parser_has_frame_data(const gps_nmea_parser_t *parser);
bool gps_nmea_parser_take_completed_frame(gps_nmea_parser_t *parser, gps_info_t *gps_info);
bool gps_nmea_parser_finish_frame(gps_nmea_parser_t *parser, gps_info_t *gps_info);

#endif /* GPS_NMEA_PARSER_H */
