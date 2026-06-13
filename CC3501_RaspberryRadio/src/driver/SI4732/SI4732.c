#include "SI4732.h"

/*------------------------------------- Private Constants -------------------------------------*/

#define SI4732_STATUS_RESPONSE_LEN        1
#define SI4732_GET_REV_FM_RESPONSE_LEN    16
#define SI4732_GET_REV_AM_RESPONSE_LEN    9
#define SI4732_TUNE_STATUS_RESPONSE_LEN   8
#define SI4732_FM_RSQ_RESPONSE_LEN        8
#define SI4732_AM_RSQ_RESPONSE_LEN        6
#define SI4732_FM_RDS_RESPONSE_LEN        13
#define SI4732_SHORT_COMMAND_WAIT_MS      1
#define SI4732_SET_PROPERTY_WAIT_MS       10
#define SI4732_POWER_UP_OSC_WAIT_MS       500

/*------------------------------------- Private Device -------------------------------------*/

static const SI4732_Device_t si4732 = {
  .i2c = SI4732_IIC_HANDLE,
  .addr = SI4732_IIC_ADDR_SEN0,
  .reset_pin = SI4732_RST_PIN,
  .timeout_us = SI4732_IIC_TIMEOUT_US,
};

/*------------------------------------- Private Helpers -------------------------------------*/

static void si4732_wait(void)
{
  sleep_ms(SI4732_SHORT_COMMAND_WAIT_MS);
}

static uint8_t si4732_u16_msb(uint16_t value)
{
  return (uint8_t)((value & 0xff00u) >> 8);
}

static uint8_t si4732_u16_lsb(uint16_t value)
{
  return (uint8_t)(value & 0x00ffu);
}

static uint8_t si4732_seek_arg(
    SI4732_SeekDirection seek_direction,
    SI4732_SeekWrap wrap)
{
  return (uint8_t)((seek_direction << 3) | (wrap << 2));
}

static uint8_t si4732_tune_status_arg(
    SI4732_TuneStatusCancel cancel_seek,
    SI4732_INTStatusClear clear_tune_seek_flag)
{
  return (uint8_t)((cancel_seek << 1) | clear_tune_seek_flag);
}

static uint8_t si4732_tune_mode_arg(
    SI4732_TuningFreeze tuning_freeze,
    SI4732_FastTuningMode fast_tuning_mode)
{
  return (uint8_t)(
      (tuning_freeze == SI4732_FREEZE_METRICS_TUNING ? 0x02 : 0x00) |
      (fast_tuning_mode == SI4732_FAST_TUNING ? 0x01 : 0x00));
}

static SI4732_API_STATUS si4732_status_to_api(const SI4732_StatusResponse_t *status)
{
  if ((status->CTS == 1) && (status->ERR == 0)) {
    return SI4732_SUCCESS;
  }

  return SI4732_FAIL;
}

/*------------------------------------- Private Bus IO -------------------------------------*/

static void si4732_write(const uint8_t *data, uint8_t length)
{
  (void)i2c_write_timeout_us(
      si4732.i2c,
      si4732.addr,
      data,
      length,
      false,
      si4732.timeout_us);
}

static void si4732_read(uint8_t *data, uint8_t length)
{
  (void)i2c_read_timeout_us(
      si4732.i2c,
      si4732.addr,
      data,
      length,
      false,
      si4732.timeout_us);
}

static void si4732_read_response(
    SI4732_StatusResponse_t *status,
    uint8_t response_len,
    uint8_t *response)
{
  si4732_read(response, response_len);

  status->CTS = (response[0] & 0x80) ? 1 : 0;
  status->ERR = (response[0] & 0x40) ? 1 : 0;
  status->RSQINT = (response[0] & 0x08) ? 1 : 0;
  status->RDSINT = (response[0] & 0x04) ? 1 : 0;
  status->STCINT = (response[0] & 0x01) ? 1 : 0;
}

static void si4732_set_interface_i2c(void)
{
  gpio_put(si4732.reset_pin, 0);
  si4732_wait();
  gpio_put(si4732.reset_pin, 1);
}

static void si4732_reset(void)
{
  si4732_set_interface_i2c();
}

/*------------------------------------- Raw Communication API -------------------------------------*/

SI4732_API_STATUS SI4732_Set_Property(
    SI4732_Property property,
    uint16_t property_value)
{
  SI4732_StatusResponse_t status;
  uint8_t response[SI4732_STATUS_RESPONSE_LEN];
  uint8_t tx_seq[6] = {
    SI4732_CMD_SET_PROPERTY,
    0x00,
    si4732_u16_msb(property),
    si4732_u16_lsb(property),
    si4732_u16_msb(property_value),
    si4732_u16_lsb(property_value),
  };

  si4732_write(tx_seq, sizeof(tx_seq));
  sleep_ms(SI4732_SET_PROPERTY_WAIT_MS);
  si4732_read_response(&status, sizeof(response), response);

  return si4732_status_to_api(&status);
}

/*------------------------------------- Power and Revision -------------------------------------*/

SI4732_API_STATUS SI4732_Power_Up_FM(void)
{
  SI4732_StatusResponse_t status;
  uint8_t response[SI4732_STATUS_RESPONSE_LEN];
  uint8_t tx_seq[3] = {
    SI4732_CMD_POWER_UP,
    0xd0,
    0x05,
  };

  si4732_reset();
  si4732_write(tx_seq, sizeof(tx_seq));
  sleep_ms(SI4732_POWER_UP_OSC_WAIT_MS);
  si4732_read_response(&status, sizeof(response), response);

  return si4732_status_to_api(&status);
}

SI4732_API_STATUS SI4732_Power_Up_SW_AM_LW(void)
{
  SI4732_StatusResponse_t status;
  uint8_t response[SI4732_STATUS_RESPONSE_LEN];
  uint8_t tx_seq[3] = {
    SI4732_CMD_POWER_UP,
    0xd1,
    0x05,
  };

  si4732_reset();
  si4732_write(tx_seq, sizeof(tx_seq));
  sleep_ms(SI4732_POWER_UP_OSC_WAIT_MS);
  si4732_read_response(&status, sizeof(response), response);

  return si4732_status_to_api(&status);
}

SI4732_API_STATUS SI4732_Get_Rev_FM(uint8_t *info)
{
  SI4732_StatusResponse_t status;
  uint8_t response[SI4732_GET_REV_FM_RESPONSE_LEN];
  uint8_t tx_seq[1] = {
    SI4732_CMD_GET_REV,
  };

  si4732_write(tx_seq, sizeof(tx_seq));
  si4732_wait();
  si4732_read_response(&status, sizeof(response), response);

  for (uint8_t i = 0; i < 15; i++) {
    info[i] = response[i + 1];
  }

  return si4732_status_to_api(&status);
}

SI4732_API_STATUS SI4732_Get_Rev_SW_AM_LW(uint8_t *info)
{
  SI4732_StatusResponse_t status;
  uint8_t response[SI4732_GET_REV_AM_RESPONSE_LEN];
  uint8_t tx_seq[1] = {
    SI4732_CMD_GET_REV,
  };

  si4732_write(tx_seq, sizeof(tx_seq));
  si4732_wait();
  si4732_read_response(&status, sizeof(response), response);

  for (uint8_t i = 0; i < 8; i++) {
    info[i] = response[i + 1];
  }

  return si4732_status_to_api(&status);
}

/*------------------------------------- Tune and Seek Commands -------------------------------------*/

SI4732_API_STATUS SI4732_Set_FM_Freq(
    uint16_t freq_10khz,
    SI4732_TuningFreeze tuning_freeze,
    SI4732_FastTuningMode fast_tuning_mode)
{
  SI4732_StatusResponse_t status;
  uint8_t response[SI4732_STATUS_RESPONSE_LEN];
  uint8_t tx_seq[5] = {
    SI4732_CMD_FM_TUNE_FREQ,
    si4732_tune_mode_arg(tuning_freeze, fast_tuning_mode),
    si4732_u16_msb(freq_10khz),
    si4732_u16_lsb(freq_10khz),
    0x00,
  };

  if ((freq_10khz < FM_BAND_BOTTOM_FREQ_10KHZ) ||
      (freq_10khz > FM_BAND_TOP_FREQ_10KHZ)) {
    return SI4732_FAIL;
  }

  si4732_write(tx_seq, sizeof(tx_seq));
  si4732_wait();
  si4732_read_response(&status, sizeof(response), response);

  return si4732_status_to_api(&status);
}

SI4732_API_STATUS SI4732_Set_SW_AM_LW_Freq(
    uint16_t freq_1khz,
    uint16_t ant_tuning_cap,
    SI4732_FastTuningMode fast_tuning_mode)
{
  SI4732_StatusResponse_t status;
  uint8_t response[SI4732_STATUS_RESPONSE_LEN];
  uint8_t tx_seq[6] = {
    SI4732_CMD_SW_AM_LW_TUNE_FREQ,
    (uint8_t)(fast_tuning_mode == SI4732_FAST_TUNING ? 0x01 : 0x00),
    si4732_u16_msb(freq_1khz),
    si4732_u16_lsb(freq_1khz),
    si4732_u16_msb(ant_tuning_cap),
    si4732_u16_lsb(ant_tuning_cap),
  };

  if ((freq_1khz < SW_AM_LW_BAND_BOTTOM_FREQ_1KHZ) ||
      (freq_1khz > SW_AM_LW_BAND_TOP_FREQ_1KHZ) ||
      (ant_tuning_cap > SW_AM_LW_ANT_TUNING_CAP_MAX)) {
    return SI4732_FAIL;
  }

  si4732_write(tx_seq, sizeof(tx_seq));
  si4732_wait();
  si4732_read_response(&status, sizeof(response), response);

  return si4732_status_to_api(&status);
}

void SI4732_Get_INT_Status(SI4732_StatusResponse_t *status)
{
  uint8_t response[SI4732_STATUS_RESPONSE_LEN];
  uint8_t tx_seq[1] = {
    SI4732_CMD_GET_INT_STATUS,
  };

  si4732_write(tx_seq, sizeof(tx_seq));
  si4732_wait();
  si4732_read_response(status, sizeof(response), response);
}

SI4732_API_STATUS SI4732_FM_Start_Seek(
    SI4732_SeekDirection seek_direction,
    SI4732_SeekWrap wrap)
{
  SI4732_StatusResponse_t status;
  uint8_t response[SI4732_STATUS_RESPONSE_LEN];
  uint8_t tx_seq[2] = {
    SI4732_CMD_FM_SEEK_START,
    si4732_seek_arg(seek_direction, wrap),
  };

  si4732_write(tx_seq, sizeof(tx_seq));
  si4732_wait();
  si4732_read_response(&status, sizeof(response), response);

  return si4732_status_to_api(&status);
}

SI4732_API_STATUS SI4732_SW_AM_LW_Start_Seek(
    uint16_t ant_tuning_cap,
    SI4732_SeekDirection seek_direction,
    SI4732_SeekWrap wrap)
{
  SI4732_StatusResponse_t status;
  uint8_t response[SI4732_STATUS_RESPONSE_LEN];
  uint8_t tx_seq[6] = {
    SI4732_CMD_SW_AM_LW_SEEK_START,
    si4732_seek_arg(seek_direction, wrap),
    0x00,
    0x00,
    si4732_u16_msb(ant_tuning_cap),
    si4732_u16_lsb(ant_tuning_cap),
  };

  if (ant_tuning_cap > SW_AM_LW_ANT_TUNING_CAP_MAX) {
    return SI4732_FAIL;
  }

  si4732_write(tx_seq, sizeof(tx_seq));
  si4732_wait();
  si4732_read_response(&status, sizeof(response), response);

  return si4732_status_to_api(&status);
}

/*------------------------------------- Status Readers -------------------------------------*/

SI4732_API_STATUS SI4732_Get_FM_Tune_Status(
    SI4732_TuneStatusCancel cancel_seek,
    SI4732_INTStatusClear clear_seek_done_int_flag,
    SI4732_FM_TuneStatus_t *status)
{
  SI4732_StatusResponse_t response_status;
  uint8_t response[SI4732_TUNE_STATUS_RESPONSE_LEN];
  uint8_t tx_seq[2] = {
    SI4732_CMD_FM_TUNE_STATUS,
    si4732_tune_status_arg(cancel_seek, clear_seek_done_int_flag),
  };

  si4732_write(tx_seq, sizeof(tx_seq));
  si4732_wait();
  si4732_read_response(&response_status, sizeof(response), response);

  status->BLTF = (response[1] & 0x80) ? 1 : 0;
  status->AFCRL = (response[1] & 0x02) ? 1 : 0;
  status->VALID = (response[1] & 0x01) ? 1 : 0;
  status->read_freq_10kHz = (uint16_t)((response[2] << 8) | response[3]);
  status->rssi_dbuv = response[4];
  status->snr_db = response[5];
  status->multipath = response[6];

  return si4732_status_to_api(&response_status);
}

SI4732_API_STATUS SI4732_Get_SW_AM_LW_Tune_Status(
    SI4732_TuneStatusCancel cancel_seek,
    SI4732_INTStatusClear clear_seek_done_int_flag,
    SI4732_SW_AM_LW_TuneStatus_t *status)
{
  SI4732_StatusResponse_t response_status;
  uint8_t response[SI4732_TUNE_STATUS_RESPONSE_LEN];
  uint8_t tx_seq[2] = {
    SI4732_CMD_SW_AM_LW_TUNE_STATUS,
    si4732_tune_status_arg(cancel_seek, clear_seek_done_int_flag),
  };

  si4732_write(tx_seq, sizeof(tx_seq));
  si4732_wait();
  si4732_read_response(&response_status, sizeof(response), response);

  status->BLTF = (response[1] & 0x80) ? 1 : 0;
  status->AFCRL = (response[1] & 0x02) ? 1 : 0;
  status->VALID = (response[1] & 0x01) ? 1 : 0;
  status->read_freq_1kHz = (uint16_t)((response[2] << 8) | response[3]);
  status->rssi_dbuv = response[4];
  status->snr_db = response[5];
  status->read_ant_cap_val = (uint16_t)((response[6] << 8) | response[7]);

  return si4732_status_to_api(&response_status);
}

SI4732_API_STATUS SI4732_Get_FM_RSQ_Status(
    SI4732_INTStatusClear clear_all_int_flag,
    SI4732_FM_RSQ_Status_t *status)
{
  SI4732_StatusResponse_t response_status;
  uint8_t response[SI4732_FM_RSQ_RESPONSE_LEN];
  uint8_t tx_seq[2] = {
    SI4732_CMD_FM_RSQ_STATUS,
    (uint8_t)(clear_all_int_flag == SI4732_INT_FLAG_CLEAR ? 0x01 : 0x00),
  };

  si4732_write(tx_seq, sizeof(tx_seq));
  si4732_wait();
  si4732_read_response(&response_status, sizeof(response), response);

  status->BLENDINT = (response[1] & 0x80) ? 1 : 0;
  status->MULTHINT = (response[1] & 0x20) ? 1 : 0;
  status->MULTLINT = (response[1] & 0x10) ? 1 : 0;
  status->SNRHINT = (response[1] & 0x08) ? 1 : 0;
  status->SNRLINT = (response[1] & 0x04) ? 1 : 0;
  status->RSSIHINT = (response[1] & 0x02) ? 1 : 0;
  status->RSSILINT = (response[1] & 0x01) ? 1 : 0;
  status->SMUTE = (response[2] & 0x08) ? 1 : 0;
  status->AFCRL = (response[2] & 0x02) ? 1 : 0;
  status->VALID = (response[2] & 0x01) ? 1 : 0;
  status->PILOT = (response[3] & 0x80) ? 1 : 0;
  status->STBLEND = response[3] & 0x7f;
  status->rssi_dbuv = response[4];
  status->snr_db = response[5];
  status->multipath = response[6];
  status->freq_offset = response[7];

  return si4732_status_to_api(&response_status);
}

SI4732_API_STATUS SI4732_Get_SW_AM_LW_RSQ_Status(
    SI4732_INTStatusClear clear_all_int_flag,
    SI4732_SW_AM_LW_RSQ_Status_t *status)
{
  SI4732_StatusResponse_t response_status;
  uint8_t response[SI4732_AM_RSQ_RESPONSE_LEN];
  uint8_t tx_seq[2] = {
    SI4732_CMD_AM_SW_AM_LW_RSQ_STATUS,
    (uint8_t)(clear_all_int_flag == SI4732_INT_FLAG_CLEAR ? 0x01 : 0x00),
  };

  si4732_write(tx_seq, sizeof(tx_seq));
  si4732_wait();
  si4732_read_response(&response_status, sizeof(response), response);

  status->SNRHINT = (response[1] & 0x08) ? 1 : 0;
  status->SNRLINT = (response[1] & 0x04) ? 1 : 0;
  status->RSSIHINT = (response[1] & 0x02) ? 1 : 0;
  status->RSSILINT = (response[1] & 0x01) ? 1 : 0;
  status->SMUTE = (response[2] & 0x08) ? 1 : 0;
  status->AFCRL = (response[2] & 0x02) ? 1 : 0;
  status->VALID = (response[2] & 0x01) ? 1 : 0;
  status->rssi_dbuv = response[4];
  status->snr_db = response[5];

  return si4732_status_to_api(&response_status);
}

SI4732_API_STATUS SI4732_Get_RDS_Status(
    SI4732_RDSStatusOnly rds_status_only,
    SI4732_RDSEmptyFIFO empty_fifo,
    SI4732_INTStatusClear clear_rds_int,
    SI4732_FM_RDS_Status_t *status)
{
  SI4732_StatusResponse_t response_status;
  uint8_t response[SI4732_FM_RDS_RESPONSE_LEN];
  uint8_t tx_seq[2] = {
    SI4732_CMD_FM_RDS_STATUS,
    (uint8_t)((rds_status_only == SI4732_RDS_FIFO_CURRENT_VALID_DATA ? 0x04 : 0x00) |
              (empty_fifo == SI4732_RDS_FIFO_CLEAR ? 0x02 : 0x00) |
              (clear_rds_int == SI4732_INT_FLAG_CLEAR ? 0x01 : 0x00)),
  };

  si4732_write(tx_seq, sizeof(tx_seq));
  si4732_wait();
  si4732_read_response(&response_status, sizeof(response), response);

  status->RDSNEWBLOCKB = (response[1] & 0x20) ? 1 : 0;
  status->RDSNEWBLOCKA = (response[1] & 0x10) ? 1 : 0;
  status->RDSSYNCFOUND = (response[1] & 0x04) ? 1 : 0;
  status->RDSSYNCLOST = (response[1] & 0x02) ? 1 : 0;
  status->RDSRECV = (response[1] & 0x01) ? 1 : 0;
  status->GRPLOST = (response[2] & 0x04) ? 1 : 0;
  status->RDSSYNC = (response[2] & 0x01) ? 1 : 0;
  status->RDSFIFOUSED = response[3];
  status->BLOCKA = (uint16_t)((response[4] << 8) | response[5]);
  status->BLOCKB = (uint16_t)((response[6] << 8) | response[7]);
  status->BLOCKC = (uint16_t)((response[8] << 8) | response[9]);
  status->BLOCKD = (uint16_t)((response[10] << 8) | response[11]);
  status->BLEA = (response[12] & 0xc0) >> 6;
  status->BLEB = (response[12] & 0x30) >> 4;
  status->BLEC = (response[12] & 0x0c) >> 2;
  status->BLED = response[12] & 0x03;

  return si4732_status_to_api(&response_status);
}

/*------------------------------------- Device Output Configuration -------------------------------------*/

SI4732_API_STATUS SI4732_GPO_Ctrl(void)
{
  SI4732_StatusResponse_t status;
  uint8_t response[SI4732_STATUS_RESPONSE_LEN];
  uint8_t tx_seq[2] = {
    SI4732_CMD_GPO_CTL,
    0x0e,
  };

  si4732_write(tx_seq, sizeof(tx_seq));
  si4732_wait();
  si4732_read_response(&status, sizeof(response), response);

  return si4732_status_to_api(&status);
}

SI4732_API_STATUS SI4732_Set_Digital_Audio_Output_Format(
    SI4732_DigitalDCLKEdge dclk_edge,
    SI4732_DigitalOutputMode output_mode,
    SI4732_DigitalMonoMode mono_mode,
    SI4732_DigitalBits bits)
{
  uint16_t property_value =
      (uint16_t)((dclk_edge << 7) |
                 (output_mode << 3) |
                 (mono_mode << 2) |
                 bits);

  return SI4732_Set_Property(SI4732_PROP_DIGITAL_OUTPUT_FORMAT, property_value);
}

SI4732_API_STATUS SI4732_Set_Digital_Audio_Sample_Rate(uint16_t sample_rate_sps)
{
  return SI4732_Set_Property(
      SI4732_PROP_DIGITAL_OUTPUT_SAMPLE_RATE,
      sample_rate_sps);
}

SI4732_API_STATUS SI4732_Set_Audio_Volume(uint8_t volume)
{
  if (volume > SI4732_AUDIO_VOLUME_MAX) {
    return SI4732_FAIL;
  }

  return SI4732_Set_Property(SI4732_PROP_RX_VOLUME, volume);
}

SI4732_API_STATUS SI4732_Set_Audio_Mute(bool mute_r, bool mute_l)
{
  uint16_t property_value = (uint16_t)(((mute_l ? 1 : 0) << 1) |
                                       (mute_r ? 1 : 0));

  return SI4732_Set_Property(SI4732_PROP_RX_HARD_MUTE, property_value);
}

/*------------------------------------- Clock Configuration -------------------------------------*/

SI4732_API_STATUS SI4732_Set_REFCLK_Freq(uint16_t refclk_hz)
{
  return SI4732_Set_Property(SI4732_PROP_REFCLK_FREQ, refclk_hz);
}

SI4732_API_STATUS SI4732_Set_REFCLK_Prescaler(
    SI4732_RCLKSEL rclk_source,
    uint16_t prescaler_val)
{
  uint16_t property_value;

  if ((prescaler_val == 0) || (prescaler_val > SI4732_PRESCALER_MAX)) {
    return SI4732_FAIL;
  }

  property_value = (uint16_t)((rclk_source << 12) | prescaler_val);

  return SI4732_Set_Property(SI4732_PROP_REFCLK_PRESCALER, property_value);
}

/*------------------------------------- Seek Configuration -------------------------------------*/

SI4732_API_STATUS SI4732_Set_FM_Seek_Spacing(SI4732_FMSeekSpacing spacing)
{
  return SI4732_Set_Property(SI4732_PROP_FM_SEEK_FREQ_SPACING, spacing);
}

SI4732_API_STATUS SI4732_Set_FM_Seek_Band_Top_Freq(uint16_t freq_10khz)
{
  uint16_t remainder;

  if ((freq_10khz < FM_BAND_BOTTOM_FREQ_10KHZ) ||
      (freq_10khz > FM_BAND_TOP_FREQ_10KHZ)) {
    return SI4732_FAIL;
  }

  remainder = freq_10khz % 5;
  if (remainder != 0) {
    freq_10khz = (uint16_t)(freq_10khz - remainder);
  }

  return SI4732_Set_Property(SI4732_PROP_FM_SEEK_BAND_TOP, freq_10khz);
}

SI4732_API_STATUS SI4732_Set_FM_Seek_Band_Bottom_Freq(uint16_t freq_10khz)
{
  uint16_t remainder;

  if ((freq_10khz < FM_BAND_BOTTOM_FREQ_10KHZ) ||
      (freq_10khz > FM_BAND_TOP_FREQ_10KHZ)) {
    return SI4732_FAIL;
  }

  remainder = freq_10khz % 5;
  if (remainder != 0) {
    freq_10khz = (uint16_t)(freq_10khz - remainder);
  }

  return SI4732_Set_Property(SI4732_PROP_FM_SEEK_BAND_BOTTOM, freq_10khz);
}

SI4732_API_STATUS SI4732_Set_SW_AM_LW_Seek_Spacing(SI4732_SWAMLWSeekSpacing spacing)
{
  return SI4732_Set_Property(SI4732_PROP_SW_AM_LW_SEEK_FREQ_SPACING, spacing);
}

SI4732_API_STATUS SI4732_Set_SW_AM_LW_Seek_Band_Top_Freq(uint16_t freq_1khz)
{
  if ((freq_1khz < SW_AM_LW_BAND_BOTTOM_FREQ_1KHZ) ||
      (freq_1khz > SW_AM_LW_BAND_TOP_FREQ_1KHZ)) {
    return SI4732_FAIL;
  }

  return SI4732_Set_Property(SI4732_PROP_SW_AM_LW_SEEK_BAND_TOP, freq_1khz);
}

SI4732_API_STATUS SI4732_Set_SW_AM_LW_Seek_Band_Bottom_Freq(uint16_t freq_1khz)
{
  if ((freq_1khz < SW_AM_LW_BAND_BOTTOM_FREQ_1KHZ) ||
      (freq_1khz > SW_AM_LW_BAND_TOP_FREQ_1KHZ)) {
    return SI4732_FAIL;
  }

  return SI4732_Set_Property(SI4732_PROP_SW_AM_LW_SEEK_BAND_BOTTOM, freq_1khz);
}

/*------------------------------------- Channel Filter Configuration -------------------------------------*/

SI4732_API_STATUS SI4732_Set_FM_Channel_Filter(SI4732_FMBandwidth fm_bandwidth)
{
  return SI4732_Set_Property(SI4732_PROP_FM_CHANNEL_FILTER, fm_bandwidth);
}

SI4732_API_STATUS SI4732_Set_SW_AM_LW_Channel_Filter(
    SI4732_SWAMLWBandwidth am_bandwidth,
    SI4732_SWAMLWNoiseRejectFilter noise_reject_filter)
{
  uint16_t property_value =
      (uint16_t)(((noise_reject_filter ==
                   SI4732_SW_AM_LW_ENABLE_POWER_LINE_NOISE_REJECTION_FILTER
                       ? 1
                       : 0)
                  << 8) |
                 (am_bandwidth & 0x000f));

  return SI4732_Set_Property(SI4732_PROP_AM_CHANNEL_FILTER, property_value);
}

/*------------------------------------- Blocking Tune Helpers -------------------------------------*/

SI4732_API_STATUS SI4732_Set_FM_Freq_Blocking_And_Read_Tune_Status(
    uint16_t freq_10khz,
    SI4732_TuningFreeze tuning_freeze,
    SI4732_FastTuningMode fast_tuning_mode,
    SI4732_TuneStatusCancel cancel_seek,
    SI4732_INTStatusClear clear_seek_done_int_flag,
    SI4732_FM_TuneStatus_t *status,
    uint16_t timeout)
{
  uint16_t timeout_count = 0;
  SI4732_StatusResponse_t int_status;

  if (SI4732_Set_FM_Freq(freq_10khz, tuning_freeze, fast_tuning_mode) ==
      SI4732_FAIL) {
    return SI4732_FAIL;
  }

  do {
    si4732_wait();
    timeout_count++;
    if (timeout_count >= timeout) {
      return SI4732_FAIL;
    }
    SI4732_Get_INT_Status(&int_status);
  } while ((int_status.CTS == 0) ||
           (int_status.ERR == 1) ||
           (int_status.STCINT == 0));

  return SI4732_Get_FM_Tune_Status(
      cancel_seek,
      clear_seek_done_int_flag,
      status);
}

SI4732_API_STATUS SI4732_Set_SW_AM_LW_Freq_Blocking_And_Read_Tune_Status(
    uint16_t freq_1khz,
    uint16_t ant_tuning_cap,
    SI4732_FastTuningMode fast_tuning_mode,
    SI4732_TuneStatusCancel cancel_seek,
    SI4732_INTStatusClear clear_seek_done_int_flag,
    SI4732_SW_AM_LW_TuneStatus_t *status,
    uint16_t timeout)
{
  uint16_t timeout_count = 0;
  SI4732_StatusResponse_t int_status;

  if (SI4732_Set_SW_AM_LW_Freq(freq_1khz, ant_tuning_cap, fast_tuning_mode) ==
      SI4732_FAIL) {
    return SI4732_FAIL;
  }

  do {
    si4732_wait();
    timeout_count++;
    if (timeout_count >= timeout) {
      return SI4732_FAIL;
    }
    SI4732_Get_INT_Status(&int_status);
  } while ((int_status.CTS == 0) ||
           (int_status.ERR == 1) ||
           (int_status.STCINT == 0));

  return SI4732_Get_SW_AM_LW_Tune_Status(
      cancel_seek,
      clear_seek_done_int_flag,
      status);
}

/*------------------------------------- Utility -------------------------------------*/

float SI4732_dBuV_to_dBm(float dbuv)
{
  return dbuv - 106.98f;
}
