#ifndef RADIO_GUI_DEMO_BACKEND_H
#define RADIO_GUI_DEMO_BACKEND_H

#include "gui/radio_gui.h"

/** Return callback wiring for the SDL-only mock backend. */
radio_gui_config_t radio_gui_demo_backend_config(void);

/** Load representative receiver, waveform, hardware, and GPS values into the GUI. */
void radio_gui_demo_backend_populate(radio_gui_t * gui);

#endif /* RADIO_GUI_DEMO_BACKEND_H */
