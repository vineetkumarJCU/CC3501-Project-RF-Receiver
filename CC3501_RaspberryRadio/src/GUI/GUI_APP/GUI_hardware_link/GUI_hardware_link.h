#ifndef GUI_HARDWARE_LINK_H
#define GUI_HARDWARE_LINK_H

#include "GPS_module.h"
#include "SD_card_log.h"
#include "radio_gui.h"

/** Initialize the cross-core SD command/status queues and RSQ snapshot lock. */
bool radio_gui_hardware_link_init(void);

/** Return callback wiring for the RP2040 hardware backend. */
radio_gui_config_t radio_gui_link_hardware_config(void);

/** Update GUI widgets from hardware/driver snapshots. Call only from the LVGL task. */
void radio_gui_update_si4732_rsq_info(radio_gui_t *gui);
void radio_gui_sample_si4732_rsq_info(radio_gui_t *gui, bool update_widgets);
void radio_gui_update_battery_voltage(radio_gui_t *gui);
void radio_gui_update_waveform(radio_gui_t *gui, const int8_t *time_domain_waveform_data, const int8_t *spec_waveform_data);
void radio_gui_update_gps_info(radio_gui_t *gui, const gps_info_t *gps_info);
void radio_gui_startup_hardware(radio_gui_t *gui);

/** Cross-core SD logging bridge: GUI submits, Core 0 consumes and reports. */
bool radio_gui_take_sd_logging_request(bool *enabled);
void radio_gui_report_sd_logging_state(radio_gui_sd_status_t status,
                                       bool logging_active);
void radio_gui_apply_sd_logging_state(radio_gui_t *gui);

/** Copy the latest SI4732 sample captured on the LVGL/hardware owner task. */
bool radio_gui_get_latest_si4732_rsq_info(sd_card_log_rsq_info_t *rsq_info);

#endif /* GUI_HARDWARE_LINK_H */
