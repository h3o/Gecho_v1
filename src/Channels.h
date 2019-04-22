/*
 * Channels.h
 *
 *  Created on: Nov 26, 2016
 *      Author: mayo
 */

#ifndef CHANNELS_H_
#define CHANNELS_H_

#include <dsp/Filters.h>
#include <extensions/Antarctica.h>
#include <hw/gpio.h>
#include <Test_fn.h>

#define CHORDS_CAPTURE_BUTTONS			1
#define CHORDS_CAPTURE_MAGNETIC_RING	2
#define CHORDS_CAPTURE_MIC				3
#define CHORDS_CAPTURE_PICKUPS			4

#define CHANNEL_21_INTERACTIVE_NOISE_EFFECT				21
#define CHANNEL_22_INTERACTIVE_NOISE_EFFECT				22
#define CHANNEL_23_INTERACTIVE_NOISE_EFFECT				23
#define CHANNEL_24_INTERACTIVE_NOISE_EFFECT				24

#define CHANNEL_31_THEREMIN_BY_MAGNETIC_RING			31
#define CHANNEL_32_THEREMIN_BY_IR_SENSORS				32
#define CHANNEL_33_DCO_SYNTH_DIRECT						33
#define CHANNEL_34_DCO_SYNTH_PROGRAMMABLE				34

#define CHANNEL_111_RECENTLY_GENERATED_SONG				111
#define CHANNEL_112_RECENTLY_GENERATED_SONG_W_MELODY	112
#define CHANNEL_113_DIRECT_PLAY_BY_USART				113

#define CHANNEL_222_GENERATE_SONG_FROM_NOISE				222

extern uint64_t channel;

extern char *melody_str;
extern char *progression_str;
extern int progression_str_length;
extern char *settings_str;

void custom_program_init(uint64_t prog);
void custom_effect_init(uint64_t prog);
void custom_effect_filter_process();
void alt_mode_hi_pass_with_water();

extern int mixed_sample_buffer_ptr_L, mixed_sample_buffer_ptr_R;
extern int16_t *mixed_sample_buffer;
extern int MIXED_SAMPLE_BUFFER_LENGTH;

#endif /* CHANNELS_H_ */
