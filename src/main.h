/*
 * Gecho Loopsynth official firmware
 *
 *  Created on: Apr 10, 2016
 *      Author: mario (http://gechologic.com/contact)
 *
 *  This file is part of the Gecho Loopsynth Firmware Development Framework.
 *  It can be used within the terms of CC-BY-NC-SA license.
 *
 *  Find more information at: http://gechologic.com/gechologists/
 */

//Includes

#include <AudioLoopFilters.h>
#include <AudioLoopNoFilters.h>
#include <board_config.h>
#include <Channels.h>
#include <Interface.h>
#include <Test_fn.h>
#include <dsp/Detectors.h>
#include <dsp/Filters.h>
#include <hw/codec.h>
#include <hw/controls.h>
#include <hw/eeprom.h>
#include <hw/gpio.h>
#include <hw/sensors.h>
#include <hw/signals.h>
#include <hw/leds.h>
#include <hw/system_fn.h>
#include <Interface.h>
#include <extensions/Drums.h>
#include <extensions/Samples.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


//-----------------------------------------------------------------------------------

volatile int16_t sample_i16 = 0;
//volatile uint16_t sample_u16 = 0;

Filters *fil;

float sample_f[2],sample_mix;
int16_t ADC_sample_recv;

int run_program = 1;
int tempo = 1;
int chord = 0;

//int MIDI_new_note = 0;
//int MIDI_new_volume = 0;
//int MIDI_new_note_freq = 0;
//int MIDI_notes_to_freq[100];
