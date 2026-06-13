/**
 * @file SI4732.h
 * @brief SI4732 receiver driver API for Raspberry Pi Pico SDK.
 */

#ifndef _SI4732_H_
#define _SI4732_H_

#include <stdbool.h>
#include <stdint.h>

#include "main.h"
#include "board_pin_def.h"
#include "board_init.h"

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

/* I2C addressing. The Pico SDK APIs use 7-bit addresses. */
#define SI4732_IIC_WR_ADDR_SEN0 0x22
#define SI4732_IIC_RD_ADDR_SEN0 0x23
#define SI4732_IIC_WR_ADDR_SEN1 0xC6
#define SI4732_IIC_RD_ADDR_SEN1 0xC7
#define SI4732_IIC_ADDR_SEN0    (SI4732_IIC_WR_ADDR_SEN0 >> 1)
#define SI4732_IIC_ADDR_SEN1    (SI4732_IIC_WR_ADDR_SEN1 >> 1)
#define SI4732_IIC_TIMEOUT_US   (1000 * 1000)

/* Frequency and value limits used by the public API. */
#define FM_BAND_BOTTOM_FREQ_10KHZ      6400
#define FM_BAND_TOP_FREQ_10KHZ         10800
#define SW_AM_LW_BAND_BOTTOM_FREQ_1KHZ 149
#define SW_AM_LW_BAND_TOP_FREQ_1KHZ    23000
#define SW_AM_LW_ANT_TUNING_CAP_MAX    6143
#define SI4732_AUDIO_VOLUME_MAX        63
#define SI4732_PRESCALER_MAX           4095

/* Recommended seek ranges from AN332. */
#define SI4732_RECOMMENDED_RANGE_AM_IN_US_TOP_kHz      1710
#define SI4732_RECOMMENDED_RANGE_AM_IN_US_BOTTOM_kHz   520
#define SI4732_RECOMMENDED_RANGE_AM_IN_ASIA_TOP_kHz    1710
#define SI4732_RECOMMENDED_RANGE_AM_IN_ASIA_BOTTOM_kHz 522
#define SI4732_RECOMMENDED_RANGE_SW_TOP_kHz            23000
#define SI4732_RECOMMENDED_RANGE_SW_BOTTOM_kHz         2300
#define SI4732_RECOMMENDED_RANGE_LW_TOP_kHz            279
#define SI4732_RECOMMENDED_RANGE_LW_BOTTOM_kHz         153

/*-------------------------------------- Commands --------------------------------------*/

typedef enum {
  SI4732_CMD_POWER_UP = 0x01,
  SI4732_CMD_GET_REV = 0x10,
  SI4732_CMD_POWER_DOWN = 0x11,
  SI4732_CMD_SET_PROPERTY = 0x12,
  SI4732_CMD_GET_INT_STATUS = 0x14,
  SI4732_CMD_FM_TUNE_FREQ = 0x20,
  SI4732_CMD_FM_SEEK_START = 0x21,
  SI4732_CMD_FM_TUNE_STATUS = 0x22,
  SI4732_CMD_FM_RSQ_STATUS = 0x23,
  SI4732_CMD_FM_RDS_STATUS = 0x24,
  SI4732_CMD_SW_AM_LW_TUNE_FREQ = 0x40,
  SI4732_CMD_SW_AM_LW_SEEK_START = 0x41,
  SI4732_CMD_SW_AM_LW_TUNE_STATUS = 0x42,
  SI4732_CMD_AM_SW_AM_LW_RSQ_STATUS = 0x43,
  SI4732_CMD_GPO_CTL = 0x80,
  SI4732_CMD_GPO_SET = 0x81,
} SI4732_Command;

/*------------------------------------- Properties -------------------------------------*/

typedef enum {
  SI4732_PROP_GPO_IEN = 0x0001,
  SI4732_PROP_DIGITAL_OUTPUT_FORMAT = 0x0102,
  SI4732_PROP_DIGITAL_OUTPUT_SAMPLE_RATE = 0x0104,
  SI4732_PROP_REFCLK_FREQ = 0x0201,
  SI4732_PROP_REFCLK_PRESCALER = 0x0202,
  SI4732_PROP_FM_DEEMPHASIS = 0x1100,
  SI4732_PROP_FM_CHANNEL_FILTER = 0x1102,
  SI4732_PROP_FM_BLEND_STEREO_THRESHOLD = 0x1105,
  SI4732_PROP_FM_BLEND_MONO_THRESHOLD = 0x1106,
  SI4732_PROP_FM_MAX_TUNE_ERROR = 0x1108,
  SI4732_PROP_FM_SEEK_BAND_BOTTOM = 0x1400,
  SI4732_PROP_FM_SEEK_BAND_TOP = 0x1401,
  SI4732_PROP_FM_SEEK_FREQ_SPACING = 0x1402,
  SI4732_PROP_FM_SEEK_TUNE_SNR_THRESHOLD = 0x1403,
  SI4732_PROP_FM_SEEK_TUNE_RSSI_THRESHOLD = 0x1404,
  SI4732_PROP_FM_RDS_CONFIG = 0x1502,
  SI4732_PROP_FM_RDS_CONFIDENCE = 0x1503,
  SI4732_PROP_FM_BLEND_RSSI_STEREO_THRESHOLD = 0x1800,
  SI4732_PROP_FM_BLEND_RSSI_MONO_THRESHOLD = 0x1801,
  SI4732_PROP_FM_BLEND_SNR_STEREO_THRESHOLD = 0x1804,
  SI4732_PROP_FM_BLEND_SNR_MONO_THRESHOLD = 0x1805,
  SI4732_PROP_FM_BLEND_MULTIPATH_STEREO_THRESHOLD = 0x1808,
  SI4732_PROP_FM_BLEND_MULTIPATH_MONO_THRESHOLD = 0x1809,
  SI4732_PROP_FM_HICUT_SNR_HIGH_THRESHOLD = 0x1A00,
  SI4732_PROP_FM_HICUT_SNR_LOW_THRESHOLD = 0x1A01,
  SI4732_PROP_AM_CHANNEL_FILTER = 0x3102,
  SI4732_PROP_SW_AM_LW_SEEK_BAND_BOTTOM = 0x3400,
  SI4732_PROP_SW_AM_LW_SEEK_BAND_TOP = 0x3401,
  SI4732_PROP_SW_AM_LW_SEEK_FREQ_SPACING = 0x3402,
  SI4732_PROP_RX_VOLUME = 0x4000,
  SI4732_PROP_RX_HARD_MUTE = 0x4001,
} SI4732_Property;

/*------------------------------------ Status Types ------------------------------------*/

typedef enum {
  SI4732_SUCCESS = 0,
  SI4732_FAIL = 1,
} SI4732_API_STATUS;

typedef struct {
  uint32_t CTS : 1;
  uint32_t ERR : 1;
  uint32_t : 2;
  uint32_t RSQINT : 1;
  uint32_t RDSINT : 1;
  uint32_t : 1;
  uint32_t STCINT : 1;
} SI4732_StatusResponse_t;

typedef struct {
  i2c_inst_t *i2c;
  uint8_t addr;
  uint reset_pin;
  uint32_t timeout_us;
} SI4732_Device_t;

typedef struct {
  uint32_t BLTF : 1;
  uint32_t : 5;
  uint32_t AFCRL : 1;
  uint32_t VALID : 1;
  uint16_t read_freq_10kHz;
  uint8_t rssi_dbuv;
  uint8_t snr_db;
  uint8_t multipath;
} SI4732_FM_TuneStatus_t;

typedef struct {
  uint32_t BLTF : 1;
  uint32_t : 5;
  uint32_t AFCRL : 1;
  uint32_t VALID : 1;
  uint16_t read_freq_1kHz;
  uint8_t rssi_dbuv;
  uint8_t snr_db;
  uint16_t read_ant_cap_val;
} SI4732_SW_AM_LW_TuneStatus_t;

typedef struct {
  uint32_t BLENDINT : 1;
  uint32_t : 1;
  uint32_t MULTHINT : 1;
  uint32_t MULTLINT : 1;
  uint32_t SNRHINT : 1;
  uint32_t SNRLINT : 1;
  uint32_t RSSIHINT : 1;
  uint32_t RSSILINT : 1;
  uint32_t : 4;
  uint32_t SMUTE : 1;
  uint32_t : 1;
  uint32_t AFCRL : 1;
  uint32_t VALID : 1;
  uint32_t PILOT : 1;
  uint32_t STBLEND : 6;
  uint8_t rssi_dbuv;
  uint8_t snr_db;
  uint8_t multipath;
  uint8_t freq_offset;
} SI4732_FM_RSQ_Status_t;

typedef struct {
  uint32_t : 4;
  uint32_t SNRHINT : 1;
  uint32_t SNRLINT : 1;
  uint32_t RSSIHINT : 1;
  uint32_t RSSILINT : 1;
  uint32_t : 4;
  uint32_t SMUTE : 1;
  uint32_t : 1;
  uint32_t AFCRL : 1;
  uint32_t VALID : 1;
  uint32_t : 8;
  uint8_t rssi_dbuv;
  uint8_t snr_db;
} SI4732_SW_AM_LW_RSQ_Status_t;

typedef struct {
  uint32_t : 2;
  uint32_t RDSNEWBLOCKB : 1;
  uint32_t RDSNEWBLOCKA : 1;
  uint32_t RDSSYNCFOUND : 1;
  uint32_t RDSSYNCLOST : 1;
  uint32_t RDSRECV : 1;
  uint32_t : 4;
  uint32_t GRPLOST : 1;
  uint32_t : 1;
  uint32_t RDSSYNC : 1;
  uint8_t RDSFIFOUSED;
  uint16_t BLOCKA;
  uint16_t BLOCKB;
  uint16_t BLOCKC;
  uint16_t BLOCKD;
  uint32_t BLEA : 2;
  uint32_t BLEB : 2;
  uint32_t BLEC : 2;
  uint32_t BLED : 2;
} SI4732_FM_RDS_Status_t;

/*---------------------------------- Configuration Enums ----------------------------------*/

typedef enum {
  SI4732_SEEK_DOWN = 0,
  SI4732_SEEK_UP = 1,
} SI4732_SeekDirection;

typedef enum {
  SI4732_SEEK_STOP_AT_LIMIT = 0,
  SI4732_SEEK_WRAP_AT_LIMIT = 1,
} SI4732_SeekWrap;

typedef enum {
  SI4732_FREEZE_METRICS_TUNING = 0,
  SI4732_NOT_FREEZE_METRICS_TUNING = 1,
} SI4732_TuningFreeze;

typedef enum {
  SI4732_NORMAL_TUNING = 0,
  SI4732_FAST_TUNING = 1,
} SI4732_FastTuningMode;

typedef enum {
  SI4732_TUNE_STATUS_CONTINUE_SEEK = 0,
  SI4732_TUNE_STATUS_CANCEL_SEEK = 1,
} SI4732_TuneStatusCancel;

typedef enum {
  SI4732_INT_FLAG_KEEP = 0,
  SI4732_INT_FLAG_CLEAR = 1,
} SI4732_INTStatusClear;

typedef enum {
  SI4732_RDS_FIFO_CONTAINS_OLDEST_DATA = 0,
  SI4732_RDS_FIFO_CURRENT_VALID_DATA = 1,
} SI4732_RDSStatusOnly;

typedef enum {
  SI4732_RDS_FIFO_READ_THEN_CLEAR = 0,
  SI4732_RDS_FIFO_CLEAR = 1,
} SI4732_RDSEmptyFIFO;

typedef enum {
  SI4732_DCLK_RISING_EDGE = 0,
  SI4732_DCLK_FALLING_EDGE = 1,
} SI4732_DigitalDCLKEdge;

typedef enum {
  I2S = 0b0000,
  LEFT_JUSTIFIED = 0b0110,
  MSB_AT_SECOND_DCLK_AFTER_DFS_PULSE = 0b1000,
  MSB_AT_FIRST_DCLK_AFTER_DFS_PULSE = 0b1100,
} SI4732_DigitalOutputMode;

typedef enum {
  USE_MONO_STEREO_BLEND = 0,
  FORCE_MONO = 1,
} SI4732_DigitalMonoMode;

typedef enum {
  _16BITS = 0,
  _20BITS = 1,
  _24BITS = 2,
  _8BITS = 3,
} SI4732_DigitalBits;

typedef enum {
  RCLK_AS_CLK_SOURCE = 0,
  DCLK_AS_CLK_SOURCE = 1,
} SI4732_RCLKSEL;

typedef enum {
  SI4732_FM_BW_AUTO = 0,
  SI4732_FM_BW_110KHZ = 1,
  SI4732_FM_BW_84KHZ = 2,
  SI4732_FM_BW_60KHZ = 3,
  SI4732_FM_BW_40KHZ = 4,
} SI4732_FMBandwidth;

typedef enum {
  SI4732_SW_AM_LW_BW_6KHZ = 0,
  SI4732_SW_AM_LW_BW_4KHZ = 1,
  SI4732_SW_AM_LW_BW_3KHZ = 2,
  SI4732_SW_AM_LW_BW_2KHZ = 3,
  SI4732_SW_AM_LW_BW_1KHZ = 4,
  SI4732_SW_AM_LW_BW_1K8HZ = 5,
  SI4732_SW_AM_LW_BW_2K5HZ = 6,
} SI4732_SWAMLWBandwidth;

typedef enum {
  SI4732_SW_AM_LW_DISABLE_POWER_LINE_NOISE_REJECTION_FILTER = 0,
  SI4732_SW_AM_LW_ENABLE_POWER_LINE_NOISE_REJECTION_FILTER = 1,
} SI4732_SWAMLWNoiseRejectFilter;

typedef enum {
  SI4732_FM_SEEK_SPACING_50KHZ = 0x05,
  SI4732_FM_SEEK_SPACING_100KHZ = 0x0A,
  SI4732_FM_SEEK_SPACING_200KHZ = 0x14,
} SI4732_FMSeekSpacing;

typedef enum {
  SI4732_SW_AM_LW_SEEK_SPACING_1KHZ = 0x01,
  SI4732_SW_AM_LW_SEEK_SPACING_5KHZ = 0x05,
  SI4732_SW_AM_LW_SEEK_SPACING_9KHZ = 0x09,
  SI4732_SW_AM_LW_SEEK_SPACING_10KHZ = 0x0A,
} SI4732_SWAMLWSeekSpacing;

/*-------------------------------------- Public API --------------------------------------*/



/**
 * @brief Set an SI4732 property.
 * @param property Property ID.
 * @param property_value Property value.
 * @retval SI4732_SUCCESS on accepted command, otherwise SI4732_FAIL.
 */
SI4732_API_STATUS SI4732_Set_Property(SI4732_Property property, uint16_t property_value);

/**
 * @brief Power up the receiver in FM mode with analog audio output.
 * @retval SI4732_SUCCESS on accepted command, otherwise SI4732_FAIL.
 */
SI4732_API_STATUS SI4732_Power_Up_FM(void);

/**
 * @brief Power up the receiver in AM/SW/LW mode with analog audio output.
 * @retval SI4732_SUCCESS on accepted command, otherwise SI4732_FAIL.
 */
SI4732_API_STATUS SI4732_Power_Up_SW_AM_LW(void);

/**
 * @brief Read the FM component revision response.
 * @param info Destination buffer for 15 response bytes.
 * @retval SI4732_SUCCESS on valid response, otherwise SI4732_FAIL.
 */
SI4732_API_STATUS SI4732_Get_Rev_FM(uint8_t *info);

/**
 * @brief Read the AM/SW/LW component revision response.
 * @param info Destination buffer for 8 response bytes.
 * @retval SI4732_SUCCESS on valid response, otherwise SI4732_FAIL.
 */
SI4732_API_STATUS SI4732_Get_Rev_SW_AM_LW(uint8_t *info);

/**
 * @brief Start FM tuning.
 * @param freq_10khz FM frequency in 10 kHz units.
 * @param tuning_freeze Metric freeze behavior.
 * @param fast_tuning_mode Normal or fast tuning mode.
 * @retval SI4732_SUCCESS on accepted command, otherwise SI4732_FAIL.
 */
SI4732_API_STATUS SI4732_Set_FM_Freq(
    uint16_t freq_10khz,
    SI4732_TuningFreeze tuning_freeze,
    SI4732_FastTuningMode fast_tuning_mode);

/**
 * @brief Start AM/SW/LW tuning.
 * @param freq_1khz Frequency in 1 kHz units.
 * @param ant_tuning_cap Antenna tuning capacitor value; 0 selects automatic tuning.
 * @param fast_tuning_mode Normal or fast tuning mode.
 * @retval SI4732_SUCCESS on accepted command, otherwise SI4732_FAIL.
 */
SI4732_API_STATUS SI4732_Set_SW_AM_LW_Freq(
    uint16_t freq_1khz,
    uint16_t ant_tuning_cap,
    SI4732_FastTuningMode fast_tuning_mode);

/**
 * @brief Read and update interrupt status bits.
 * @param status Destination status structure.
 * @retval None.
 */
void SI4732_Get_INT_Status(SI4732_StatusResponse_t *status);

/**
 * @brief Start an FM seek operation.
 * @param seek_direction Seek direction.
 * @param wrap Seek wrap behavior at band limits.
 * @retval SI4732_SUCCESS on accepted command, otherwise SI4732_FAIL.
 */
SI4732_API_STATUS SI4732_FM_Start_Seek(
    SI4732_SeekDirection seek_direction,
    SI4732_SeekWrap wrap);

/**
 * @brief Start an AM/SW/LW seek operation.
 * @param ant_tuning_cap Antenna tuning capacitor value; 0 selects automatic tuning.
 * @param seek_direction Seek direction.
 * @param wrap Seek wrap behavior at band limits.
 * @retval SI4732_SUCCESS on accepted command, otherwise SI4732_FAIL.
 */
SI4732_API_STATUS SI4732_SW_AM_LW_Start_Seek(
    uint16_t ant_tuning_cap,
    SI4732_SeekDirection seek_direction,
    SI4732_SeekWrap wrap);

/**
 * @brief Read FM tune status.
 * @param cancel_seek Whether to cancel an active seek.
 * @param clear_seek_done_int_flag Whether to clear the seek/tune-complete flag.
 * @param status Destination tune status structure.
 * @retval SI4732_SUCCESS on valid response, otherwise SI4732_FAIL.
 */
SI4732_API_STATUS SI4732_Get_FM_Tune_Status(
    SI4732_TuneStatusCancel cancel_seek,
    SI4732_INTStatusClear clear_seek_done_int_flag,
    SI4732_FM_TuneStatus_t *status);

/**
 * @brief Read AM/SW/LW tune status.
 * @param cancel_seek Whether to cancel an active seek.
 * @param clear_seek_done_int_flag Whether to clear the seek/tune-complete flag.
 * @param status Destination tune status structure.
 * @retval SI4732_SUCCESS on valid response, otherwise SI4732_FAIL.
 */
SI4732_API_STATUS SI4732_Get_SW_AM_LW_Tune_Status(
    SI4732_TuneStatusCancel cancel_seek,
    SI4732_INTStatusClear clear_seek_done_int_flag,
    SI4732_SW_AM_LW_TuneStatus_t *status);

/**
 * @brief Read FM received signal quality status.
 * @param clear_all_int_flag Whether to clear RSQ related interrupt flags.
 * @param status Destination RSQ status structure.
 * @retval SI4732_SUCCESS on valid response, otherwise SI4732_FAIL.
 */
SI4732_API_STATUS SI4732_Get_FM_RSQ_Status(
    SI4732_INTStatusClear clear_all_int_flag,
    SI4732_FM_RSQ_Status_t *status);

/**
 * @brief Read AM/SW/LW received signal quality status.
 * @param clear_all_int_flag Whether to clear RSQ related interrupt flags.
 * @param status Destination RSQ status structure.
 * @retval SI4732_SUCCESS on valid response, otherwise SI4732_FAIL.
 */
SI4732_API_STATUS SI4732_Get_SW_AM_LW_RSQ_Status(
    SI4732_INTStatusClear clear_all_int_flag,
    SI4732_SW_AM_LW_RSQ_Status_t *status);

/**
 * @brief Read FM RDS status and optionally consume or clear the FIFO.
 * @param rds_status_only Selects status-only behavior.
 * @param empty_fifo Selects FIFO read or clear behavior.
 * @param clear_rds_int Whether to clear RDSINT.
 * @param status Destination RDS status structure.
 * @retval SI4732_SUCCESS on valid response, otherwise SI4732_FAIL.
 */
SI4732_API_STATUS SI4732_Get_RDS_Status(
    SI4732_RDSStatusOnly rds_status_only,
    SI4732_RDSEmptyFIFO empty_fifo,
    SI4732_INTStatusClear clear_rds_int,
    SI4732_FM_RDS_Status_t *status);

/**
 * @brief Enable SI4732 GPO1, GPO2, and GPO3 outputs.
 * @retval SI4732_SUCCESS on accepted command, otherwise SI4732_FAIL.
 */
SI4732_API_STATUS SI4732_GPO_Ctrl(void);

/**
 * @brief Configure digital audio output format.
 * @param dclk_edge DCLK sampling edge.
 * @param output_mode Digital serial output mode.
 * @param mono_mode Mono/stereo output selection.
 * @param bits Audio sample width.
 * @retval SI4732_SUCCESS on accepted command, otherwise SI4732_FAIL.
 */
SI4732_API_STATUS SI4732_Set_Digital_Audio_Output_Format(
    SI4732_DigitalDCLKEdge dclk_edge,
    SI4732_DigitalOutputMode output_mode,
    SI4732_DigitalMonoMode mono_mode,
    SI4732_DigitalBits bits);

/**
 * @brief Configure digital audio sample rate.
 * @param sample_rate_sps Sample rate in samples per second; 0 disables digital output.
 * @retval SI4732_SUCCESS on accepted command, otherwise SI4732_FAIL.
 */
SI4732_API_STATUS SI4732_Set_Digital_Audio_Sample_Rate(uint16_t sample_rate_sps);

/**
 * @brief Configure the SI4732 reference clock frequency property.
 * @param refclk_hz Reference clock frequency in Hz.
 * @retval SI4732_SUCCESS on accepted command, otherwise SI4732_FAIL.
 */
SI4732_API_STATUS SI4732_Set_REFCLK_Freq(uint16_t refclk_hz);

/**
 * @brief Configure the SI4732 reference clock prescaler property.
 * @param rclk_source RCLK or DCLK clock source selection.
 * @param prescaler_val Prescaler value.
 * @retval SI4732_SUCCESS on accepted command, otherwise SI4732_FAIL.
 */
SI4732_API_STATUS SI4732_Set_REFCLK_Prescaler(
    SI4732_RCLKSEL rclk_source,
    uint16_t prescaler_val);

/**
 * @brief Configure FM seek channel spacing.
 * @param spacing FM seek spacing.
 * @retval SI4732_SUCCESS on accepted command, otherwise SI4732_FAIL.
 */
SI4732_API_STATUS SI4732_Set_FM_Seek_Spacing(SI4732_FMSeekSpacing spacing);

/**
 * @brief Configure FM seek upper band limit.
 * @param freq_10khz Frequency in 10 kHz units.
 * @retval SI4732_SUCCESS on accepted command, otherwise SI4732_FAIL.
 */
SI4732_API_STATUS SI4732_Set_FM_Seek_Band_Top_Freq(uint16_t freq_10khz);

/**
 * @brief Configure FM seek lower band limit.
 * @param freq_10khz Frequency in 10 kHz units.
 * @retval SI4732_SUCCESS on accepted command, otherwise SI4732_FAIL.
 */
SI4732_API_STATUS SI4732_Set_FM_Seek_Band_Bottom_Freq(uint16_t freq_10khz);

/**
 * @brief Configure AM/SW/LW seek channel spacing.
 * @param spacing AM/SW/LW seek spacing.
 * @retval SI4732_SUCCESS on accepted command, otherwise SI4732_FAIL.
 */
SI4732_API_STATUS SI4732_Set_SW_AM_LW_Seek_Spacing(SI4732_SWAMLWSeekSpacing spacing);

/**
 * @brief Configure AM/SW/LW seek upper band limit.
 * @param freq_1khz Frequency in 1 kHz units.
 * @retval SI4732_SUCCESS on accepted command, otherwise SI4732_FAIL.
 */
SI4732_API_STATUS SI4732_Set_SW_AM_LW_Seek_Band_Top_Freq(uint16_t freq_1khz);

/**
 * @brief Configure AM/SW/LW seek lower band limit.
 * @param freq_1khz Frequency in 1 kHz units.
 * @retval SI4732_SUCCESS on accepted command, otherwise SI4732_FAIL.
 */
SI4732_API_STATUS SI4732_Set_SW_AM_LW_Seek_Band_Bottom_Freq(uint16_t freq_1khz);

/**
 * @brief Configure analog audio output volume.
 * @param volume Volume level from 0 to 63.
 * @retval SI4732_SUCCESS on accepted command, otherwise SI4732_FAIL.
 */
SI4732_API_STATUS SI4732_Set_Audio_Volume(uint8_t volume);

/**
 * @brief Configure analog audio hard mute.
 * @param mute_r True to mute right audio output.
 * @param mute_l True to mute left audio output.
 * @retval SI4732_SUCCESS on accepted command, otherwise SI4732_FAIL.
 */
SI4732_API_STATUS SI4732_Set_Audio_Mute(bool mute_r, bool mute_l);

/**
 * @brief Configure FM channel filter bandwidth.
 * @param fm_bandwidth FM channel filter bandwidth.
 * @retval SI4732_SUCCESS on accepted command, otherwise SI4732_FAIL.
 */
SI4732_API_STATUS SI4732_Set_FM_Channel_Filter(SI4732_FMBandwidth fm_bandwidth);

/**
 * @brief Configure AM/SW/LW channel filter bandwidth and power-line rejection.
 * @param am_bandwidth AM/SW/LW channel filter bandwidth.
 * @param noise_reject_filter Power-line noise rejection filter state.
 * @retval SI4732_SUCCESS on accepted command, otherwise SI4732_FAIL.
 */
SI4732_API_STATUS SI4732_Set_SW_AM_LW_Channel_Filter(
    SI4732_SWAMLWBandwidth am_bandwidth,
    SI4732_SWAMLWNoiseRejectFilter noise_reject_filter);

/**
 * @brief Tune FM, wait for STC, and read tune status.
 * @param freq_10khz FM frequency in 10 kHz units.
 * @param tuning_freeze Metric freeze behavior.
 * @param fast_tuning_mode Normal or fast tuning mode.
 * @param cancel_seek Whether to cancel an active seek while reading status.
 * @param clear_seek_done_int_flag Whether to clear the seek/tune-complete flag.
 * @param status Destination tune status structure.
 * @param timeout Poll timeout in milliseconds.
 * @retval SI4732_SUCCESS on completed tune, otherwise SI4732_FAIL.
 */
SI4732_API_STATUS SI4732_Set_FM_Freq_Blocking_And_Read_Tune_Status(
    uint16_t freq_10khz,
    SI4732_TuningFreeze tuning_freeze,
    SI4732_FastTuningMode fast_tuning_mode,
    SI4732_TuneStatusCancel cancel_seek,
    SI4732_INTStatusClear clear_seek_done_int_flag,
    SI4732_FM_TuneStatus_t *status,
    uint16_t timeout);

/**
 * @brief Tune AM/SW/LW, wait for STC, and read tune status.
 * @param freq_1khz Frequency in 1 kHz units.
 * @param ant_tuning_cap Antenna tuning capacitor value; 0 selects automatic tuning.
 * @param fast_tuning_mode Normal or fast tuning mode.
 * @param cancel_seek Whether to cancel an active seek while reading status.
 * @param clear_seek_done_int_flag Whether to clear the seek/tune-complete flag.
 * @param status Destination tune status structure.
 * @param timeout Poll timeout in milliseconds.
 * @retval SI4732_SUCCESS on completed tune, otherwise SI4732_FAIL.
 */
SI4732_API_STATUS SI4732_Set_SW_AM_LW_Freq_Blocking_And_Read_Tune_Status(
    uint16_t freq_1khz,
    uint16_t ant_tuning_cap,
    SI4732_FastTuningMode fast_tuning_mode,
    SI4732_TuneStatusCancel cancel_seek,
    SI4732_INTStatusClear clear_seek_done_int_flag,
    SI4732_SW_AM_LW_TuneStatus_t *status,
    uint16_t timeout);

/**
 * @brief Convert dBuV to dBm for a 50 ohm system.
 * @param dbuv Signal level in dBuV.
 * @retval Signal level in dBm.
 */
float SI4732_dBuV_to_dBm(float dbuv);

#ifdef __cplusplus
}
#endif

#endif
