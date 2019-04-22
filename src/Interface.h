/*
 * Interface.h
 *
 *  Created on: Apr 27, 2016
 *      Author: mayo
 *
 *  This file is part of the Gecho Loopsynth Firmware Development Framework.
 *  It can be used within the terms of CC-BY-NC-SA license.
 *  It must not be distributed separately.
 *
 *  Find more information at: http://gechologic.com/gechologists/
 */

#ifndef INTERFACE_H_
#define INTERFACE_H_

#include <dsp/Filters.h>
#include "notes.h"

extern unsigned long seconds;

//extern int progressive_rhythm_factor;
//#define PROGRESSIVE_RHYTHM_BOOST_RATIO 2
//#define PROGRESSIVE_RHYTHM_RAMP_COEF 0.999f

extern float noise_volume, noise_volume_max, noise_boost_by_sensor;
extern int special_effect, selected_song, selected_melody;

#define LOOP_TYPE_FILTERS 0
#define LOOP_TYPE_NO_FILTERS 1
extern int loop_type;

extern bool
	PROG_enable_filters,
	//PROG_enable_rhythm,
	PROG_enable_chord_loop,
	PROG_enable_LED_indicators_sequencer,
	PROG_enable_LED_indicators_IR_sensors,
	PROG_enable_S1_control_noise_boost,
	PROG_enable_S2_control_noise_attenuation, //only when rhythm active
	PROG_enable_S3_control_resonance,
	PROG_enable_S4_control_arpeggiator,
	PROG_add_OpAmp_ADC12_signal,
	PROG_add_echo,
	PROG_load_echo_setting,
	PROG_add_plain_noise,
	PROG_audio_input_microphones,
	PROG_audio_input_pickups,
	PROG_audio_input_IR_sensors,
	PROG_audio_input_magnetic_sensor,
	PROG_load_input_setting,
	PROG_load_bpm_setting,
	PROG_buttons_controls_during_play,
	PROG_magnetic_sensor_test,
	PROG_magnetic_sensor_test_display,
	PROG_noise_effects,
	PROG_drum_kit,
	PROG_drum_kit_with_echo,
	PROG_mix_sample_from_flash,
	PROG_mix_input_signal,
	PROG_wavetable_sample,
	PROG_melody_by_MIDI,
	PROG_melody_by_USART;

extern bool
	TEST_enable_V1_control_voice,
    TEST_enable_V2_control_drum;

#define FILTERS_TYPE_LOW_PASS 0x01
#define FILTERS_TYPE_HIGH_PASS 0x02
#define FILTERS_ORDER_2 0x10
#define FILTERS_ORDER_4 0x20
#define DEFAULT_FILTERS_TYPE_AND_ORDER (FILTERS_TYPE_LOW_PASS + FILTERS_ORDER_4)
extern int FILTERS_TYPE_AND_ORDER;

extern int ACTIVE_FILTERS_PAIRS;
extern int PROGRESS_UPDATE_FILTERS_RATE;
extern int DEFAULT_ARPEGGIATOR_FILTER_PAIR;

extern int SHIFT_CHORD_INTERVAL;

extern int *current_chord_LEDs;
extern int current_melody_LED;
extern float current_melody_freq;

extern float sample_f[2],sample_mix, volume2;
extern int16_t ADC_sample_recv;
extern uint32_t random_value;
extern volatile int16_t sample_i16;
extern Filters *fil;

#define WAVES_FILTERS 4
extern int direct_update_filters_id[WAVES_FILTERS*2];
extern float direct_update_filters_freq[WAVES_FILTERS*2];

extern int arpeggiator_loop;

void filters_and_signals_init();
void program_settings_reset();

void IR_sensors_level_process(int *sensor_values);
void custom_song_programming_mode(int chords_capture_method);
int set_number_of_chords(int *row1, int *row2);
int set_number_of_chords();
void display_chord_position();
void goto_previous_chord();
void goto_next_chord();
void blink_current_chord(int chord);
void start_playing_current_chord(int (*temp_song)[NOTES_PER_CHORD], int current_chord);
void change_current_chord(int (*temp_song)[NOTES_PER_CHORD], int direction, int alteration);
void set_current_chord(int (*temp_song)[NOTES_PER_CHORD], int *notes, int notes_n);
int count_unset_chords();
void display_and_preview_current_chord(int (*temp_song)[NOTES_PER_CHORD]);
void start_preview_chord(int *notes, int notes_per_chord);
void process_preview_chord();
void indicate_not_enough_chords();
void edit_chords_by_buttons();
void capture_chords_from_ADC(int chords_capture_method, int auto_corr_preamp_boost);
void custom_song_edit_mode(char *progression_str);
void set_tempo_by_buttons();

#endif /* INTERFACE_H_ */
