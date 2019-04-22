/*
 * Interface.cpp
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

#include "Interface.h"
#include <dsp/Filters.h>
#include <hw/codec.h>
#include <hw/controls.h>
#include <hw/gpio.h>
#include <hw/leds.h>
#include <hw/sensors.h>
#include <hw/signals.h>
#include <hw/eeprom.h>
#include "Channels.h"
#include "MusicBox.h"
#include <stdlib.h>
#include <ctype.h>

unsigned long seconds;

//int progressive_rhythm_factor;
float noise_volume, /*noise_volume_max,*/ noise_boost_by_sensor;
int special_effect, selected_song, selected_melody;
int loop_type = LOOP_TYPE_FILTERS; //0=normal (with filters), 1=simplified (without filters)

bool PROG_enable_filters,
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
	 PROG_wavetable_sample,
	 PROG_mix_sample_from_flash,
	 PROG_mix_input_signal,
	 PROG_melody_by_MIDI,
	 PROG_melody_by_USART;

bool TEST_enable_V1_control_voice,
     TEST_enable_V2_control_drum;

int FILTERS_TYPE_AND_ORDER;
int ACTIVE_FILTERS_PAIRS;
int PROGRESS_UPDATE_FILTERS_RATE;
int DEFAULT_ARPEGGIATOR_FILTER_PAIR;

int SHIFT_CHORD_INTERVAL;

int direct_update_filters_id[WAVES_FILTERS*2];
float direct_update_filters_freq[WAVES_FILTERS*2];

int arpeggiator_loop;

void filters_and_signals_init()
{
	ADC_sample_recv = 0;

	if(PROG_enable_filters)
	{
		//initialize filters
		fil = new Filters(selected_song);

		if(selected_melody > 0)
		{
			fil->set_melody_voice(6, selected_melody);
		}

		fil->setup(FILTERS_TYPE_AND_ORDER);
	}

	if(PROG_audio_input_IR_sensors)
	{
		ADC_configure_IR_sensors_as_audio();
		//return;
	}

#ifdef USE_IR_SENSORS
	//if(!PROG_audio_input_IR_sensors) //only if these are not used as direct audio input
	//{
		ADC_configure_SENSORS(ADCConvertedValues);
		//ADC_test_SENSORS();
	//}
#endif

	if(PROG_magnetic_sensor_test)
	{
		ADC_configure_MAGNETIC_SENSOR();
		//return;
	}

	//--------------------------------------------------------------------------------------
    //load settings from EEPROM
	//--------------------------------------------------------------------------------------

	//set input signal gain

    uint32_t gain_DW = EEPROM_LoadSettings_DW(SETTINGS_INPUT_GAIN);
	if(gain_DW == 0)
	{
		OpAmp_ADC12_signal_conversion_factor = OPAMP_ADC12_CONVERSION_FACTOR_DEFAULT;
	}
	else
	{
		//memcpy(&OpAmp_ADC12_signal_conversion_factor,gain_DW,4);
		((uint32_t*)(&OpAmp_ADC12_signal_conversion_factor))[0]=gain_DW;
	}

	if(FILTERS_TYPE_AND_ORDER & FILTERS_TYPE_LOW_PASS)
	{
		OpAmp_ADC12_signal_conversion_factor *= OPAMP_ADC12_CONVERSION_FACTOR_BOOST_LOW_PASS;
	}

	//set echo delay

	if(PROG_load_echo_setting)
	{
		echo_dynamic_loop_current_step = EEPROM_LoadSettings_B(SETTINGS_DELAY_LENGTH,0);
		echo_dynamic_loop_length = get_echo_length(echo_dynamic_loop_current_step);
		PROG_add_echo = (echo_dynamic_loop_length>0); //enable echo only if loop length is not zero
	}

	//set inputs multiplexer

	int set_mux = 0;
	if(PROG_load_input_setting && (PROG_audio_input_microphones || PROG_audio_input_pickups)) //override only if common inputs pre-configured in channel and allowed to load
	{
		input_mux_current_step = EEPROM_LoadSettings_B(SETTINGS_INPUT_SELECT,0);
		set_mux = 1;
	}
	else if(PROG_audio_input_microphones)
	{
		input_mux_current_step = 0;
		set_mux = 1;
	}
	else if(PROG_audio_input_pickups)
	{
		input_mux_current_step = 1;
		set_mux = 1;
	}

	if(set_mux) //don't set it unless some of inputs is active, otherwise will interfere with magnetic sensor channels
	{
		ADC_set_input_multiplexer(input_mux_current_step);
	}

	if(PROG_load_bpm_setting)
	{
		//set default BPM
		tempo_bpm = (int16_t)EEPROM_LoadSettings_W(SETTINGS_TEMPO_BPM,0) + TEMPO_BPM_DEFAULT;
	}

	//--------------------------------------------------------------------------------------
}

void program_settings_reset()
{
	run_program = 1; //enable the main program loop
	//noise_volume_max = 1.0f;
	noise_boost_by_sensor = 1.0f;
	special_effect = 0;

	selected_song = 1;
	selected_melody = 0;

	PROG_enable_filters = true;
	//PROG_enable_rhythm = true;
	PROG_enable_chord_loop = true;
	PROG_enable_LED_indicators_sequencer = true;
	PROG_enable_LED_indicators_IR_sensors = true;
    PROG_enable_S1_control_noise_boost = false;
    PROG_enable_S2_control_noise_attenuation = false;
	PROG_enable_S3_control_resonance = false;
	PROG_enable_S4_control_arpeggiator = false;
	PROG_add_OpAmp_ADC12_signal = true;
	PROG_add_echo = true;
	PROG_load_echo_setting = true;
	PROG_add_plain_noise = true;
	PROG_audio_input_microphones = true;
    PROG_audio_input_pickups = false;
    PROG_audio_input_IR_sensors = false;
    PROG_audio_input_magnetic_sensor = false;
    PROG_load_input_setting = true;
    PROG_load_bpm_setting = true;
    PROG_buttons_controls_during_play = true;
    PROG_magnetic_sensor_test = false;
    PROG_magnetic_sensor_test_display = true; //if test enabled, display enabled by default
    PROG_noise_effects = false;
    PROG_drum_kit = false;
    PROG_drum_kit_with_echo = false,
    PROG_wavetable_sample = false;
    PROG_mix_sample_from_flash = false;
	PROG_mix_input_signal = false;
    PROG_melody_by_MIDI = false;
    PROG_melody_by_USART = false;

	TEST_enable_V1_control_voice = false;
    TEST_enable_V2_control_drum = false;

    FILTERS_TYPE_AND_ORDER = DEFAULT_FILTERS_TYPE_AND_ORDER;

    ACTIVE_FILTERS_PAIRS = FILTERS/2; //normally, all filter pairs are active

	PROGRESS_UPDATE_FILTERS_RATE = 100; //every 10 ms, at 0.129 ms (57/441)
    DEFAULT_ARPEGGIATOR_FILTER_PAIR = 0;

    SHIFT_CHORD_INTERVAL = 2; //default every 2 seconds

    ECHO_MIXING_GAIN_MUL = 2; //amount of signal to feed back to echo loop, expressed as a fragment
    ECHO_MIXING_GAIN_DIV = 3; //e.g. if MUL=2 and DIV=3, it means 2/3 of signal is mixed in

	seconds = 0;
    sampleCounter = 0;
    arpeggiator_loop = 0;
	//progressive_rhythm_factor = 1;
    noise_volume = 0.5f;//1.0f; //noise_volume_max;

    /*
    if(progression_str != NULL)
    {
    	free(progression_str);
    	progression_str = NULL;
    }
    */

	if(set_chords_map != NULL)
    {
    	free(set_chords_map);
    	set_chords_map = NULL;
    }

	if(temp_song != NULL)
    {
    	free(temp_song);
    	temp_song = NULL;
    }
}

void IR_sensors_level_process(int *sensor_values)
{
    if(PROG_enable_S1_control_noise_boost)
    {
		if(sensor_values[0] > 200)
		{
			//sensor value 1200 -> limit
			noise_boost_by_sensor = (sensor_values[0] - 200) / (1000.0f / (NOISE_BOOST_BY_SENSOR_LIMIT - NOISE_BOOST_BY_SENSOR_DEFAULT)) + NOISE_BOOST_BY_SENSOR_DEFAULT;

			if(noise_boost_by_sensor > NOISE_BOOST_BY_SENSOR_LIMIT)
			{
				noise_boost_by_sensor = NOISE_BOOST_BY_SENSOR_LIMIT;
			}
		}
		else //sensor value <= 200 -> default
		{
			noise_boost_by_sensor = NOISE_BOOST_BY_SENSOR_DEFAULT;
		}
    }

    if(PROG_enable_S2_control_noise_attenuation)
    {
		if(sensor_values[1] > IR_sensors_THRESHOLD_2) { //250

			//noise_volume_max = (1000 - sensor_values[1]) / 1000.0f;
			//if(noise_volume_max < 0) {
			//	noise_volume_max = 0;
			//}
			noise_boost_by_sensor = (1000 - sensor_values[1]) / 1000.0f;
			if(noise_boost_by_sensor < 0) {
				noise_boost_by_sensor = 0;
			}
		}
		else
		{
			//noise_volume_max = 1;
		}
    }
}

int current_chord = 0;
int temp_total_chords = 8; //can be 1-8 with multiplier 1-4 (so, total of 1-32)
int total_chords_row1 = 4;
int total_chords_row2 = 2; //default 4x2 = 8 chords

int *current_chord_LEDs; //pointer into another array
int current_melody_LED;
float current_melody_freq;
int preview_chord_running = 0;

void custom_song_programming_mode(int chords_capture_method)
{
	temp_total_chords = set_number_of_chords();
	temp_song = (int (*)[NOTES_PER_CHORD])malloc(temp_total_chords * sizeof(*temp_song));
	memset(temp_song,0,temp_total_chords * sizeof(*temp_song));
	set_chords_map = (int *)malloc(temp_total_chords * sizeof(int));
	memset(set_chords_map,0,temp_total_chords * sizeof(int));

	//reset_button_pressed_timings();
	wait_till_all_buttons_released();

	display_chord_position();

	if(chords_capture_method==CHORDS_CAPTURE_BUTTONS)
	{
		edit_chords_by_buttons();
	}
	else if(chords_capture_method==CHORDS_CAPTURE_MIC
		 || chords_capture_method==CHORDS_CAPTURE_PICKUPS
		 || chords_capture_method==CHORDS_CAPTURE_MAGNETIC_RING)
	{
		int auto_corr_preamp_boost;

		if(chords_capture_method==CHORDS_CAPTURE_MIC)
		{
			ADC_configure_MIC(BOARD_MIC_ADC_CFG_STRING); //use built-in microphones
			//goertzel_octaves = GOERTZEL_OCTAVES_678;
			//goertzel_octaves = GOERTZEL_OCTAVES_567;
			auto_corr_preamp_boost = AUTOCORRELATION_PREAMP_BOOST_MIC;
		}
		else if(chords_capture_method==CHORDS_CAPTURE_PICKUPS)
		{
			ADC_configure_PICKUPS();	//use LineIn instead of microphones
			//goertzel_octaves = GOERTZEL_OCTAVES_456;
			auto_corr_preamp_boost = AUTOCORRELATION_PREAMP_BOOST_LINEIN;
		}
		else if(chords_capture_method==CHORDS_CAPTURE_MAGNETIC_RING)
		{
			ADC_configure_MAGNETIC_SENSOR();
		}

		capture_chords_from_ADC(chords_capture_method, auto_corr_preamp_boost);
		process_preview_chord();
	}

	codec_reset();
	seconds = 0;

	//indicate enough chords set/collected
	for(int j=0;j<2;j++)
	{
		display_number_of_chords(total_chords_row1, total_chords_row2);
		Delay(500);
		LED_R8_set_byte(0);
		LED_O4_set_byte(0);
		Delay(500);
	}
}

void display_chord_position()
{
	//int current_chord = 0;
	//int temp_total_chords = 8; //can be 1-8 with multiplier 1-4 (so, total of 1-32)
	//int total_chords_row1 = 8;
	//int total_chords_row2 = 1;

	LED_R8_all_OFF();

	//light up a Red LED according to current chord's position
	LED_R8_set(current_chord % total_chords_row1, 1);

	LED_O4_all_OFF();

	//light up an Orange LED according to current chord's position
	LED_O4_set(current_chord / total_chords_row1, 1);
}

int set_number_of_chords(int *row1, int *row2) //define number of chords, that returns more detailed information
{
	int chords = set_number_of_chords();
	*row1 = total_chords_row1;
	*row2 = total_chords_row2;
	return chords;
}

int set_number_of_chords() //define number of chords
{
	int button_pressed;

	display_number_of_chords(total_chords_row1, total_chords_row2);

	while(1)
	{
		if((button_pressed = get_button_pressed()))
		{
			if(button_pressed == 1) //U1 button pressed -> decrement number of chords
			{
				total_chords_row1--;
				if(total_chords_row1==0)
				{
					total_chords_row1 = 8;
				}
			}
			if(button_pressed == 2) //U2 button pressed -> increment number of chords
			{
				total_chords_row1++;
				if(total_chords_row1==9)
				{
					total_chords_row1 = 1;
				}
			}
			if(button_pressed == 3) //U3 button pressed -> decrement multiplier
			{
				total_chords_row2--;
				if(total_chords_row2==0)
				{
					total_chords_row2 = 4;
				}
			}
			if(button_pressed == 4) //U4 button pressed -> increment multiplier
			{
				total_chords_row2++;
				if(total_chords_row2==5)
				{
					total_chords_row2 = 1;
				}
			}
			/*if(button_pressed == 4) //U4 button pressed -> set defaults
			{
				total_chords_row1 = 8;
				total_chords_row2 = 1;
			}*/

			display_number_of_chords(total_chords_row1, total_chords_row2);

			if(button_pressed == 5) //SET pressed -> next step
			{
				button_pressed = 0; //clear so will not trigger button press  event in next programming step
				return total_chords_row1 * total_chords_row2;
			 }
		}
	}
}

void goto_previous_chord()
{
	current_chord--;

	if(current_chord == -1)
	{
		current_chord = temp_total_chords - 1;
	}
}

void goto_next_chord()
{
	current_chord++;

	if(current_chord >= temp_total_chords)
	{
		current_chord = 0;
	}
}

int chord_blink_timer = 0;
void blink_current_chord(int chord)
{
	if(chord_blink_timer==0)
	{
		LED_R8_all_OFF();
		LED_O4_all_OFF();
	}
	if(chord_blink_timer%40000==0)
	{
		//get correct combination of leds
		int led_row1 = current_chord % total_chords_row1;
		int led_row2 = current_chord / total_chords_row1;
		LED_R8_set(led_row1, 1); //turn on LED, numbered from 0
		LED_O4_set(led_row2, 1); //turn on LED, numbered from 0
	}
	if(chord_blink_timer%40000==32000)
	{
		LED_R8_all_OFF();
		LED_O4_all_OFF();
	}
	chord_blink_timer++;
}

void start_playing_current_chord(int (*temp_song)[NOTES_PER_CHORD], int current_chord)
{
	int chord_notes[NOTES_PER_CHORD];

	for(int i=0;i<NOTES_PER_CHORD;i++)
	{
		chord_notes[i] = temp_song[current_chord][i];
	}
	if(chord_notes[0]==0) //no chord defined yet
	{

	}
}

void change_current_chord(int (*temp_song)[NOTES_PER_CHORD], int direction, int alteration)
{
	FEED_DAC_WITH_SILENCE;

	int new_chord_set = 0; //indicates whether the chord was set to previously empty space, and not copied over

	if(temp_song[current_chord][0]==0) //if no chord at this position yet, set the default one
	{
		if(current_chord==0 || temp_song[current_chord-1][0]==0) //if first chord or none to copy from, fill in default
		{
			temp_song[current_chord][0] = 301; //c3
			temp_song[current_chord][1] = 305; //e3
			temp_song[current_chord][2] = 308; //g3

			new_chord_set = 1;
		}
		else //copy over previous chord
		{
			temp_song[current_chord][0] = temp_song[current_chord-1][0];
			temp_song[current_chord][1] = temp_song[current_chord-1][1];
			temp_song[current_chord][2] = temp_song[current_chord-1][2];
		}
		set_chords_map[current_chord]++;
	}

	FEED_DAC_WITH_SILENCE;

	if(direction!=0 && !new_chord_set)
	{
		//check boundaries
		if((direction > 0) && (temp_song[current_chord][0] >= 500))
		{

		}
		else if((direction < 0) && (temp_song[current_chord][0] <= 201))
		{

		}
		else
		{
			for(int i=0;i<NOTES_PER_CHORD;i++)
			{
				temp_song[current_chord][i] += direction;

				if(temp_song[current_chord][i] % 100 == 0) //carry down
				{
					temp_song[current_chord][i] = 100*(temp_song[current_chord][i]/100-1)+12;
				}
				if(temp_song[current_chord][i] % 100 == 13) //carry up
				{
					temp_song[current_chord][i] = 100*(temp_song[current_chord][i]/100+1)+1;
				}

				FEED_DAC_WITH_SILENCE;
			}
		}
	}
	else if(alteration > 0)
	{
		FEED_DAC_WITH_SILENCE;

		if(alteration==12) //major->minor
		{
			if(temp_song[current_chord][1] - temp_song[current_chord][0] == 4) //currently major
			{
				temp_song[current_chord][1] --; //shift to minor
			}
			else //if(temp_song[current_chord][1] - temp_song[current_chord][0] == 3) //currently minor, exotic chords remain unchanged
			{
				//temp_song[current_chord][1] ++;
				temp_song[current_chord][1] = temp_song[current_chord][0] + 4; //back to major, resets exotic chords too
				temp_song[current_chord][2] = temp_song[current_chord][0] + 7;
			}
		}
		if(alteration==13) //move the 2nd note around
		{
			temp_song[current_chord][1] ++;

			//check if ended up conflicting with 3rd note
			if(temp_song[current_chord][1] == temp_song[current_chord][2])
			{
				temp_song[current_chord][1] = temp_song[current_chord][0] + 2; //go to two half-tones above key note
			}

			if(temp_song[current_chord][1] - temp_song[current_chord][0] > 5) //if more than tertia
			{
				temp_song[current_chord][1] = temp_song[current_chord][0] + 2; //go to two half-tones above key note
			}
		}
		if(alteration==14) //move the 3rd note around
		{
			temp_song[current_chord][2] ++;
			if(temp_song[current_chord][2] - temp_song[current_chord][0] > 10) //if more than three half-tones above kvinta
			{
				temp_song[current_chord][2] = temp_song[current_chord][0] + 5; //go to 2 half-tones under kvinta

				//check if ended up conflicting with 2nd note
				if(temp_song[current_chord][2] == temp_song[current_chord][1])
				{
					temp_song[current_chord][2]++; //move one half-tone up
				}
			}
		}
	}
	FEED_DAC_WITH_SILENCE;
}

void set_current_chord(int (*temp_song)[NOTES_PER_CHORD], int *notes, int notes_n)
{
	for(int i=0;i<notes_n;i++)
	{
		temp_song[current_chord][i] = notes[i]; //301==c3, 305==e3, 308==g3
	}
	set_chords_map[current_chord]++;

	display_and_preview_current_chord(temp_song);
}

int count_unset_chords()
{
	int unset = 0;
	for(int i=0;i<temp_total_chords;i++)
	{
		if(set_chords_map[i]==0)
		{
			unset++;
		}
	}
	return unset;
}

void display_and_preview_current_chord(int (*temp_song)[NOTES_PER_CHORD])
{
	int led_numbers[NOTES_PER_CHORD];

	notes_to_LEDs(temp_song[current_chord],led_numbers,NOTES_PER_CHORD);
	display_chord(led_numbers, NOTES_PER_CHORD); //array of 3, 0-7 for whites, 10-14 for blues
	start_preview_chord(temp_song[current_chord], NOTES_PER_CHORD);
}

void start_preview_chord(int *notes, int notes_per_chord)
{
	if(notes[0]==0)
	{
		return;
	}

	//codec_reset(); //in case it was just running
	//int running = 1;

	FEED_DAC_WITH_SILENCE;
	preview_chord_running = 0; //stop previous preview if still running

	float bases[notes_per_chord];
	float bases_expanded[CHORD_MAX_VOICES];
	int octave,note,octave_shift;
	double note_freq;
	double halftone_step_coef = HALFTONE_STEP_COEF;

	//convert notes to frequencies
	for(int n=0;n<notes_per_chord;n++)
	{
		FEED_DAC_WITH_SILENCE;
		octave = notes[n] / 100;
		note = notes[n] % 100 - 1; //note numbered from 0 to 11
		//add halftones... 12 halftones is plus one octave -> frequency * 2

		//note_freq = pow(halftone_step_coef, note+3);

		//pow is slow, substituted with multiplications so we can feed dack during calculation
		note_freq = 1;

		for(int h=0;h<note+3;h++)
		{
			FEED_DAC_WITH_SILENCE;
			note_freq *= halftone_step_coef;
		}

		FEED_DAC_WITH_SILENCE;
		note_freq *= NOTE_FREQ_A4;

		//shift according to the octave
		octave_shift = octave - 4; //ref note is A4 - fourth octave

		if(octave_shift > 0) //shift up
		{
			do
			{
				FEED_DAC_WITH_SILENCE;
				note_freq *= 2;
				octave_shift--;
			}
			while(octave_shift!=0);
		}
		else if(octave_shift < 0) //shift down
		{
			do
			{
				FEED_DAC_WITH_SILENCE;
				note_freq /= 2;
				octave_shift++;
			}
			while(octave_shift!=0);
		}

		bases[n] = note_freq;
	}

	for(int t=0;t<CHORD_MAX_VOICES;t++)
	{
		FEED_DAC_WITH_SILENCE;
		if(fil->chord->expand_multipliers[t][1] > 0)
		{
			bases_expanded[t] = bases[fil->chord->expand_multipliers[t][0]] * (float)fil->chord->expand_multipliers[t][1];
		}
		else
		{
			bases_expanded[t] = bases[fil->chord->expand_multipliers[t][0]] / (float)(-fil->chord->expand_multipliers[t][1]);
		}
		//if(bases_expanded[t]>5000)
		//{
		//	bases_expanded[t] /= 2;
		//}
	}

	for(int i=0;i<FILTERS;i++)
	{
		FEED_DAC_WITH_SILENCE;
		fil->iir2[i].setResonanceKeepFeedback(0.9995f);
		fil->iir2[i].setCutoffAndLimits(bases_expanded[i%CHORD_MAX_VOICES] * FILTERS_FREQ_CORRECTION / (float)I2S_AUDIOFREQ * 2 * 3);
		//reset volumes, they might be turned down by magnetic ring capture method
		fil->fp.mixing_volumes[i] = 1.0f;
		fil->fp.mixing_volumes[FILTERS/2 + i] = 1.0f;
	}

	FEED_DAC_WITH_SILENCE;
	sampleCounter = 0;
	seconds = 0;
	preview_chord_running = 1;
}

//int preview_chord_mute = 0;
void process_preview_chord()
{
	float r;

	if(preview_chord_running)
	{
		if (!(sampleCounter & 0x00000001)) //left or right channel
		{
			r = PseudoRNG1a_next_float();
			memcpy(&random_value, &r, sizeof(random_value));
		}

		if (sampleCounter & 0x00000001) //left or right channel
		{
			sample_f[0] = ( (float)(32768 - (int16_t)random_value) / 32768.0f) * noise_volume;
			sample_mix = IIR_Filter::iir_filter_multi_sum(sample_f[0],fil->iir2,FILTERS/2,fil->fp.mixing_volumes)*MAIN_VOLUME; //volume2;
			//sample_mix = sample_f[0] * volume2 / 5;
		}
		else
		{
			sample_f[1] = ( (float)(32768 - (int16_t)(random_value>>16)) / 32768.0f) * noise_volume;
			sample_mix = IIR_Filter::iir_filter_multi_sum(sample_f[1],fil->iir2+FILTERS/2,FILTERS/2,fil->fp.mixing_volumes+FILTERS/2)*MAIN_VOLUME; //volume2;
			//sample_mix = sample_f[1] * volume2 / 5;
		}

		sample_i16 = (int16_t)(sample_mix);

		while (!SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE)) {};
		//if(preview_chord_mute)
		//{
		//	SPI_I2S_SendData(CODEC_I2S, 0);
		//	preview_chord_mute--;
		//}
		//else
		//{
			SPI_I2S_SendData(CODEC_I2S, sample_i16);
		//}

		sampleCounter++;

		if (sampleCounter == 2*I2S_AUDIOFREQ - AUDIOFREQ_DIV_CORRECTION)
		{
			sampleCounter = 0;
			seconds++;

			if(seconds == 2)
			{
				preview_chord_running = 0;
				//codec_reset();

				//turn off codec volume
				//mute_master_playback(1);

				seconds = 0;
			}
		}
	}
	else
	{
		FEED_DAC_WITH_SILENCE;
		//preview_chord_mute = 10000;
	}
}

void indicate_not_enough_chords()
{
	process_preview_chord();

	//LED_R8_set_byte(int value);
	//LED_O4_set_byte(int value);

	int i,j;
	for(j=0;j<3;j++)
	{
		display_number_of_chords(total_chords_row1, total_chords_row2);
		for(i=0;i<5000;i++)
		{
			process_preview_chord();
		}
		LED_R8_set_byte(0);
		LED_O4_set_byte(0);
		for(i=0;i<5000;i++)
		{
			process_preview_chord();
		}
	}
}

void edit_chords_by_buttons()
{
	int finished = 0;
	int button_pressed;

	//check if the first chord is currently defined and play it

	if(temp_song[current_chord][2]>0)
	{
		display_and_preview_current_chord(temp_song);
		//start_playing_current_chord(temp_song,current_chord);
	}

	while(!finished)
	{
		button_pressed = get_button_combination();
		process_preview_chord();

		if(button_pressed)
		{
			if(button_pressed > 10)
			{
				//while(any_button_held()) //wait till all released, while feeding the DAC
				while(user_button_status(button_pressed%10-1)) //wait till 2nd button is released
				{
					process_preview_chord();
				}
				change_current_chord(temp_song, 0, button_pressed); //current chord with alteration
				display_and_preview_current_chord(temp_song);
			}
			if(button_pressed == 1) //previous chord
			{
				change_current_chord(temp_song, -1, 0);
				display_and_preview_current_chord(temp_song);
			}
			if(button_pressed == 2) //next chord
			{
				change_current_chord(temp_song, +1, 0);
				display_and_preview_current_chord(temp_song);
			}
			if(button_pressed == 3)
			{
				goto_previous_chord();
				display_chord_position();
				display_and_preview_current_chord(temp_song);
			}
			if(button_pressed == 4)
			{
				goto_next_chord();
				display_chord_position();
				display_and_preview_current_chord(temp_song);
			}
			if(button_pressed == 5) //SET pressed
			{
				if(count_unset_chords())
				{
					//some chords not set, indicate error
					indicate_not_enough_chords();
					display_chord_position();
				}
				else //if(!preview_chord_running)
				{
					//preview finished and all chords are set, move on to the next step
					finished = 1;
				}
			}
			//process_preview_chord();

			/*
			blink_current_chord(current_chord);
			*/
		}

		process_preview_chord();
	}
}

void capture_chords_from_ADC(int chords_capture_method, int auto_corr_preamp_boost)
{
	int finished = 0;
	int button_pressed;
	int found_notes[3];
	//char *goertzel_octaves;
	int continuous_capture = 1;

	while(!finished)
	{
		button_pressed = get_button_pressed();
		process_preview_chord();

		if(button_pressed == 1 || button_pressed == 2) //next or previous chord
		{
			//if(chords_capture_method==CHORDS_CAPTURE_MAGNETIC_RING)
			//{
			//	MagneticRing_capture(3, found_notes);
			//}
			//else
			//{
			//	AutoCorrelation_capture(3, found_notes, auto_corr_preamp_boost);
			//}
			codec_reset();
			continuous_capture = 1;
		}
		else if(button_pressed == 3)
		{
			goto_previous_chord();
			display_chord_position();
			display_and_preview_current_chord(temp_song);
			continuous_capture = 0;
		}
		else if(button_pressed == 4)
		{
			goto_next_chord();
			display_chord_position();
			display_and_preview_current_chord(temp_song);
			continuous_capture = 0;
		}
		else if(button_pressed == 5) //SET pressed
		{
			if(count_unset_chords())
			{
				//some chords not set, indicate error
				indicate_not_enough_chords();
				display_chord_position();
			}
			else //if(!preview_chord_running)
			{
				//preview finished and all chords are set, move on to the next step
				finished = 1;
			}
		}
		else if(continuous_capture && !ANY_USER_BUTTON_ON)
		{
			display_chord_position();

			/*
			if(chords_capture_method==CHORDS_CAPTURE_MIC)
			{
				ADC_configure_MIC(BOARD_MIC_ADC_CFG_STRING); //use built-in microphones
				//goertzel_octaves = GOERTZEL_OCTAVES_678;
				//goertzel_octaves = GOERTZEL_OCTAVES_567;
				auto_corr_preamp_boost = AUTOCORRELATION_PREAMP_BOOST_MIC;
			}
			else if(chords_capture_method==CHORDS_CAPTURE_PICKUPS)
			{
				ADC_configure_PICKUPS();	//use LineIn instead of microphones
				//goertzel_octaves = GOERTZEL_OCTAVES_456;
				auto_corr_preamp_boost = AUTOCORRELATION_PREAMP_BOOST_LINEIN;
			}
			else if(chords_capture_method==CHORDS_CAPTURE_MAGNETIC_RING)
			{
				ADC_configure_MAGNETIC_SENSOR();
			}
			*/

			if(chords_capture_method==CHORDS_CAPTURE_MAGNETIC_RING)
			{
				MagneticRing_capture(3, found_notes);
			}
			else
			{
				//Goertzel_detectors_capture(3, found_notes, goertzel_octaves);
				AutoCorrelation_capture(3, found_notes, auto_corr_preamp_boost);
			}

			//ADC_configure_ADC12_mute();
			//ADC_DeInit();

			if(found_notes[2]>0) //if 3 notes were captured
			{
				//change_current_chord(temp_song, +1, 0);
				set_current_chord(temp_song, found_notes, 3);

				codec_restart();
				display_and_preview_current_chord(temp_song);

				while(preview_chord_running)
				{
					process_preview_chord();
				}
				goto_next_chord();
			}
			else //user exited the capturing mode by a button
			{
				//stop capturing only if the button pressed was B1-B4 and not SET
				if(!BUTTON_SET_ON)
				{
					continuous_capture = 0;
					codec_restart();
				}
			}
		}

	}
}

void custom_song_edit_mode(char *progression_str)
{
	temp_total_chords = fil->chord->get_song_total_chords(progression_str);

	//todo: better logic
	if(temp_total_chords%3==0)
	{
		total_chords_row1 = temp_total_chords/3;
		total_chords_row2 = 3;
	}
	else if(temp_total_chords%2==0)
	{
		total_chords_row1 = temp_total_chords/2;
		total_chords_row2 = 2;
	}
	else
	{
		total_chords_row1 = temp_total_chords;
		total_chords_row2 = 1;
	}

	temp_song = (int (*)[NOTES_PER_CHORD])malloc(temp_total_chords * sizeof(*temp_song));
	memset(temp_song,0,temp_total_chords * sizeof(*temp_song));

	//parse song to the required format
	char *temp_song_ptr = progression_str;
	int read_note;

	for(int i=0;i<temp_total_chords;i++)
	{
		temp_song_ptr = read_next_note(temp_song_ptr, &read_note);
		temp_song[i][0] = read_note;
		temp_song_ptr = read_next_note(temp_song_ptr, &read_note);
		temp_song[i][1] = read_note;
		temp_song_ptr = read_next_note(temp_song_ptr, &read_note);
		temp_song[i][2] = read_note;
	}

	//initialize "set_chords_map" array
	set_chords_map = (int *)malloc(temp_total_chords * sizeof(int));
	memset(set_chords_map,0,temp_total_chords * sizeof(int));
	//fill the array as if all chords are already set
	for(int i=0;i<temp_total_chords;i++)
	{
		set_chords_map[i] = 1;
	}

	//we don't the need original string anymore, free the memory
	free(progression_str);

	wait_till_all_buttons_released();

	display_chord_position();

	codec_restart();
	edit_chords_by_buttons();

	codec_reset();
	seconds = 0;

	//indicate enough chords set/collected
	for(int j=0;j<2;j++)
	{
		display_number_of_chords(total_chords_row1, total_chords_row2);
		Delay(500);
		LED_R8_set_byte(0);
		LED_O4_set_byte(0);
		Delay(500);
	}
}

void set_tempo_by_buttons()
{
	//set default BPM
	tempo_bpm = (int16_t)EEPROM_LoadSettings_W(SETTINGS_TEMPO_BPM,0) + TEMPO_BPM_DEFAULT;
	int finished = 0;
	int button_pressed;

	while(!finished)
	{
		display_tempo_indicator(tempo_bpm);
		button_pressed = get_button_combination();

		if(button_pressed)
		{
			if(button_pressed > 10)
			{
			}
			if(button_pressed == 1) //button 1 turns tempo down
			{
				if(tempo_bpm>TEMPO_BPM_MIN)
				{
					tempo_bpm -= TEMPO_BPM_STEP;
				}
			}
			if(button_pressed == 2) //button 2 turns tempo up
			{
				if(tempo_bpm<TEMPO_BPM_MAX)
				{
					tempo_bpm += TEMPO_BPM_STEP;
				}
			}
			if(button_pressed == 3) //button 3 sets tempo to default value
			{
				tempo_bpm = TEMPO_BPM_DEFAULT;
			}
			if(button_pressed == 4)
			{
			}
			if(button_pressed == 5) //SET pressed
			{
				finished = 1;
			}
		}
	}
	EEPROM_StoreSettings_W(SETTINGS_TEMPO_BPM,tempo_bpm-TEMPO_BPM_DEFAULT);
}
