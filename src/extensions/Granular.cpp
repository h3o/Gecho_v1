/*
 * Granular.cpp
 *
 *  Created on: 19 Jan 2018
 *      Author: mario (http://gechologic.com/contact)
 *
 *  As explained in the coding tutorial: http://gechologic.com/granular-sampler
 *
 *  This file is part of the Gecho Loopsynth Firmware Development Framework.
 *  It can be used within the terms of CC-BY-NC-SA license.
 *
 *  Find more information at: http://gechologic.com/gechologists/
 */

#include <extensions/Granular.h>
#include <MusicBox.h>
#include <songs.h>
#include <hw/eeprom.h>

void granular_sampler(int mode)
{
	//--------------------------------------------------------------------------
	//defines, constants and variables

	#define GRAIN_SAMPLES     (I2S_AUDIOFREQ/2)	 //memory will be allocated for up to half second grain
	#define GRAIN_LENGTH_MIN  (I2S_AUDIOFREQ/16) //one 16th of second will be shortest allowed grain length

	int16_t *grain_left, *grain_right;	//two buffers for stereo samples

	#define GRAIN_VOICES_MIN  3		//one basic chord
	#define GRAIN_VOICES_MAX  33	//voices per stereo channel (42 looks like maximum)

	//following parameters will change by sensors S1,S3 and S4 (while S2 controls sampling of new sound)
	int grain_voices = GRAIN_VOICES_MIN;	//voices currently active
	int grain_length = GRAIN_SAMPLES;		//amount of samples to process from the grain
	int grain_start = 0;	//position where the grain starts within the buffer
	int grain_voices_used = GRAIN_VOICES_MAX;

	float freq[GRAIN_VOICES_MAX], step[GRAIN_VOICES_MAX];

	//there is an external sampleCounter variable already, it will be used to timing and counting seconds
	sampleCounter = 0;
	seconds = 0;

	//we need a separate counter for right channel to process it independently from the left
	unsigned int sampleCounter1;
	unsigned int sampleCounter2;

	//set input gain
	const float op_gain = PREAMP_BOOST * UNFILTERED_SIGNAL_VOLUME * 2;

	//--------------------------------------------------------------------------
	//variables to hold grain playback pitch values

	//define some handy constants
	#define FREQ_C5 523.25f	//frequency of note C5
	#define FREQ_E5 659.25f //frequency of note E5
	#define FREQ_G5 783.99f //frequency of note G5

	#define MAJOR_THIRD (FREQ_E5/FREQ_C5)	//ratio between notes C and E
	#define PERFECT_FIFTH (FREQ_G5/FREQ_C5)	//ratio between notes C and G

	//define frequencies for all voices

	if(mode)
	{
		//#define GRAIN_VOICES_MAX  27
		grain_voices_used = 27;
		float base_chord_27[3] = {FREQ_C5,FREQ_C5*MAJOR_THIRD,FREQ_C5*PERFECT_FIFTH};
		update_grain_freqs(freq, base_chord_27, grain_voices_used, 0); //update all at once

		//grain_voices_used = 24;
		//float base_chord_24[3] = {FREQ_C5,FREQ_C5*MAJOR_THIRD,FREQ_C5*PERFECT_FIFTH};
		//update_grain_freqs(freq, base_chord_24, grain_voices_used, 0); //update all at once
	}
	else
	{
		//#define GRAIN_VOICES_MAX  33
		grain_voices_used = 33;
		float base_chord_33[3] = {FREQ_C5,FREQ_C5*MAJOR_THIRD,FREQ_C5*PERFECT_FIFTH};
		update_grain_freqs(freq, base_chord_33, grain_voices_used, 0); //update all at once
	}

	//-------------------------------------------------------------------
	MusicBox *chord;
	if(mode>=1)
	{
		int selected_song;
		if(mode==1)
		{
			wait_till_all_buttons_released(); //button SET probably still held down from selecting channel
			LED_O4_all_OFF();

			int song_select = select_channel(); //re-use the mechanism as in selecting a channel
			if(song_select==111 || song_select==112)
			{
				EEPROM_LoadSongAndMelody(&progression_str, &melody_str);
				selected_song = 111;
			}
			else
			{
				int song_melody = channel_to_song_and_melody(song_select);
				selected_song = song_melody % 1000;
			}
		}
		else
		{
			selected_song = mode;
		}

		//generate chords for the selected song
		chord = new MusicBox(selected_song);

		int notes_parsed = parse_notes(chord->base_notes, chord->bases_parsed, chord->led_indicators);

		if(notes_parsed != chord->total_chords * NOTES_PER_CHORD) //integrity check
		{
			//we don't have correct amount of notes
			if(notes_parsed < chord->total_chords * NOTES_PER_CHORD) //some notes missing
			{
				do //let's add zeros, so it won't break the playback
				{
					chord->bases_parsed[notes_parsed++] = 0;
				}
				while(notes_parsed < chord->total_chords * NOTES_PER_CHORD);
			}
		}
		//chord->generate(CHORD_MAX_VOICES); //no need to expand bases
	}
	//-------------------------------------------------------------------

	//allocate memory for grain buffers
	grain_left = (int16_t*)malloc(GRAIN_SAMPLES * sizeof(int16_t));
	grain_right = (int16_t*)malloc(GRAIN_SAMPLES * sizeof(int16_t));
	memset(grain_left, 0, GRAIN_SAMPLES * sizeof(int16_t));
	memset(grain_right, 0, GRAIN_SAMPLES * sizeof(int16_t));

	//reset counters
	sampleCounter1 = 0;
	sampleCounter2 = 0;

	KEY_LED_all_off(); //turn off all LEDs
	while(ANY_USER_BUTTON_ON); //wait till all buttons released (some may still be pressed from selecting a channel)
	ADC_configure_SENSORS(ADCConvertedValues); //configure IR proximity sensors

	//init and configure audio codec
	codec_init();
	codec_ctrl_init();
	I2S_Cmd(CODEC_I2S, ENABLE);

	//reset pointers
	for(int i=1;i<grain_voices_used;i++)
	{
		step[i] = 0;
	}

	//initialize echo without using init_echo_buffer() which allocates too much memory
	#define GRANULAR_SAMPLER_ECHO_BUFFER_SIZE I2S_AUDIOFREQ //half second (buffer is shared for both stereo channels)
	echo_dynamic_loop_length = GRANULAR_SAMPLER_ECHO_BUFFER_SIZE; //set default value (can be changed dynamically)
	echo_buffer = (int16_t*)malloc(echo_dynamic_loop_length*sizeof(int16_t)); //allocate memory
	memset(echo_buffer,0,echo_dynamic_loop_length*sizeof(int16_t)); //clear memory
	echo_buffer_ptr0 = 0; //reset pointer
	int freqs_updating = 0;

	while(1) //function loop
	{
		sampleCounter++;
		if (TIMING_BY_SAMPLE_ONE_SECOND_W_CORRECTION) //one full second passed
		{
			seconds++;
			sampleCounter = 0;

			if(mode)
			{
				//replace the grain frequencies by chord
				seconds %= chord->total_chords; //wrap around to total number of chords
				freqs_updating = 2;
			}
		}

		if(TIMING_BY_SAMPLE_EVERY_1_MS)
		{
			if(freqs_updating>0)
			{
				update_grain_freqs(freq, chord->bases_parsed + seconds*3, grain_voices_used, 3-freqs_updating); //update one group only
				freqs_updating--;
			}
		}

		//sensor S2 sets offset of right stereo channel against left
		if (ADC_last_result[1] > IR_sensors_THRESHOLD_9)
		{
			sampleCounter2 = sampleCounter1 + GRAIN_SAMPLES / 2;
		}
		else if (ADC_last_result[1] > IR_sensors_THRESHOLD_8)
		{
			sampleCounter2 = sampleCounter1 + GRAIN_SAMPLES / 3;
		}
		else if (ADC_last_result[1] > IR_sensors_THRESHOLD_6)
		{
			sampleCounter2 = sampleCounter1 + GRAIN_SAMPLES / 4;
		}
		else if (ADC_last_result[1] > IR_sensors_THRESHOLD_4)
		{
			sampleCounter2 = sampleCounter1 + GRAIN_SAMPLES / 6;
		}
		else if (ADC_last_result[1] > IR_sensors_THRESHOLD_2)
		{
			sampleCounter2 = sampleCounter1;
		}

		//increment counters and wrap around if completed full cycle
		if (++sampleCounter1 >= (unsigned int)grain_length)
    	{
        	sampleCounter1 = 0;
    	}
		if (++sampleCounter2 >= (unsigned int)grain_length)
		{
			sampleCounter2 = 0;
		}

		//sensor S2 enables recording - when triggered, sample the data into the grain buffer (left channel first)
		if (ADC_last_result[1] > IR_sensors_THRESHOLD_1)
		{
			grain_left[sampleCounter1] = (int16_t)(4096/2 - (int16_t)ADC1_read()) / 4096.0f * op_gain;
        }

        //mix samples for all voices (left channel)
		sample_mix = 0;

		for(int i=0;i<grain_voices;i++)
		{
			step[i] += freq[i] / FREQ_C5;
			if ((int)step[i]>=grain_length)
			{
				step[i] = grain_start;
			}
			sample_mix += grain_left[(int)step[i]];
		}

		//send data to codec, with added echo
		while (!SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE));
        SPI_I2S_SendData(CODEC_I2S, add_echo((int16_t)(sample_mix)));

		//sample data into the right channel grain buffer, when S2 triggered
		if (ADC_last_result[1] > IR_sensors_THRESHOLD_1)
        {
			grain_right[sampleCounter2] = (int16_t)(4096/2 - (int16_t)ADC2_read()) / 4096.0f * op_gain;
        }

        //mix samples for all voices (right channel)
        sample_mix = 0;

        for(int i=0;i<grain_voices;i++)
    	{
    		sample_mix += grain_right[(int)step[i]];
    	}

		//send data to codec, with added echo
        while (!SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE));
        SPI_I2S_SendData(CODEC_I2S, add_echo((int16_t)(sample_mix)));

        //if sensor S1 triggered, increase number of active voices
        if (ADC_last_result[0] > 200)
		{
			//when sensor value is 2200, enable all voices
			grain_voices = GRAIN_VOICES_MIN + ((ADC_last_result[0] - 200) / 2000.0f) * (grain_voices_used - GRAIN_VOICES_MIN);
			if (grain_voices > grain_voices_used)
			{
				grain_voices = grain_voices_used;
			}
		}
		else //if sensor not triggered, use basic setting
		{
			grain_voices = GRAIN_VOICES_MIN;
		}

		//if sensor S4 triggered, decrease grain length
		if (ADC_last_result[3] > 200)
		{
			//when sensor value is 2200 or higher, set length to minimum
			grain_length = GRAIN_SAMPLES - (float(ADC_last_result[3] - 200) / 2000.0f) * float(GRAIN_SAMPLES - GRAIN_LENGTH_MIN);
			//grain_length &= 0xfffffffc; //round to multiples of 4 by clearing two least significant bits

			if (grain_length < GRAIN_LENGTH_MIN)
			{
				grain_length = GRAIN_LENGTH_MIN;
			}
		}
		else //if sensor not triggered, use basic setting
		{
			grain_length = GRAIN_SAMPLES;
		}

		//if sensor S3 triggered, move the grain start higher up in the buffer
		if (ADC_last_result[2] > 200)
		{
			//when sensor value is 2200 or higher, set the start to be closest to the end
			grain_start = (float(ADC_last_result[2] - 200) / 2000.0f) * (float)grain_length;
			//grain_start &= 0xfffffffc; //round to multiples of 4 by clearing two least significant bits

			if (grain_start < 0)
			{
				grain_start = 0;
			}
			if (grain_start > grain_length - GRAIN_LENGTH_MIN)
			{
				grain_start = grain_length - GRAIN_LENGTH_MIN;
			}
		}
		else //if sensor not triggered, use basic setting
		{
			grain_start = 0;
		}

		if (TIMING_BY_SAMPLE_EVERY_10_MS == 0) //100Hz periodically, at 0ms
		{
			if (ADC_process_sensors()==1) //process the sensors
			{
				//values from S3 and S4 are inverted (by hardware)
				ADC_last_result[2] = -ADC_last_result[2];
				ADC_last_result[3] = -ADC_last_result[3];

				CLEAR_ADC_RESULT_RDY_FLAG;
				sensors_loop++;

				//enable indicating of sensor levels by LEDs
				IR_sensors_LED_indicators(ADC_last_result);
			}
		}

		//we will enable default controls too (user buttons B1-B4 to control volume, inputs and echo on/off)
		if (TIMING_BY_SAMPLE_EVERY_100_MS==1234) //10Hz periodically, at sample #1234
		{
			buttons_controls_during_play();

			//if the setting goes beyond allocated buffer size, set to maximum
			if(echo_dynamic_loop_length > GRANULAR_SAMPLER_ECHO_BUFFER_SIZE)
			{
				echo_dynamic_loop_length = GRANULAR_SAMPLER_ECHO_BUFFER_SIZE; //set default value
			}

			//it does not make sense to have microphones off here, skip this setting and re-enable them
			if(input_mux_current_step==INPUT_MUX_OFF)
			{
				ADC_set_input_multiplexer(input_mux_current_step = INPUT_MUX_MICS);
			}
		}

		//process any I2C commands in queue (e.g. volume change by buttons)
		queue_codec_ctrl_process();

	}	//end while(1)
}

int16_t add_echo(int16_t sample)
{
	if(!PROG_add_echo)
	{
		return sample;
	}

	//wrap the echo loop
	echo_buffer_ptr0++;
	if (echo_buffer_ptr0 >= echo_dynamic_loop_length)
	{
		echo_buffer_ptr0 = 0;
	}

	echo_buffer_ptr = echo_buffer_ptr0 + 1;
	if (echo_buffer_ptr >= echo_dynamic_loop_length)
	{
		echo_buffer_ptr = 0;
	}

	//add echo from the loop
	echo_mix_f = float(sample) + float(echo_buffer[echo_buffer_ptr]) * ECHO_MIXING_GAIN_MUL / ECHO_MIXING_GAIN_DIV;

	if (echo_mix_f > COMPUTED_SAMPLE_MIXING_LIMIT_UPPER)
	{
		echo_mix_f = COMPUTED_SAMPLE_MIXING_LIMIT_UPPER;
	}

	if (echo_mix_f < COMPUTED_SAMPLE_MIXING_LIMIT_LOWER)
	{
		echo_mix_f = COMPUTED_SAMPLE_MIXING_LIMIT_LOWER;
	}

	sample = (int16_t)echo_mix_f;

	//store result to echo, the amount defined by a fragment
	//echo_mix_f = ((float)sample_i16 * ECHO_MIXING_GAIN_MUL / ECHO_MIXING_GAIN_DIV);
	//echo_mix_f *= ECHO_MIXING_GAIN_MUL / ECHO_MIXING_GAIN_DIV;
	//echo_buffer[echo_buffer_ptr0] = (int16_t)echo_mix_f;
	echo_buffer[echo_buffer_ptr0] = sample;

	return sample;
}

void update_grain_freqs(float *freq, float *bases, int voices_used, int part) //part: 0=all
{
//#if GRAIN_VOICES_MAX == 33
	if(voices_used==33)
	{
	if(part!=2)
	{
	freq[0] = bases[0];//FREQ_C5;
	freq[1] = bases[1];//freq[0]*MAJOR_THIRD;
	freq[2] = bases[2];//freq[0]*PERFECT_FIFTH;
	freq[3] = freq[0]*0.99;
	freq[4] = freq[1]*0.99;
	freq[5] = freq[2]*0.99;
	freq[6] = freq[0]*1.01;
	freq[7] = freq[1]*1.01;
	freq[8] = freq[2]*1.01;
	freq[9] = freq[0]*0.98/2;
	freq[10] = freq[1]*0.98/2;
	freq[11] = freq[2]*0.98/2;
	freq[12] = freq[0]*1.02/2;
	freq[13] = freq[1]*1.02/2;
	freq[14] = freq[2]*1.02/2;
	freq[15] = freq[0]*0.97*2;
	}
	if(part!=1)
	{
	freq[16] = freq[1]*0.97*2;
	freq[17] = freq[2]*0.97*2;
	freq[18] = freq[0]*1.03*2;
	freq[19] = freq[1]*1.03*2;
	freq[20] = freq[2]*1.03*2;
	freq[21] = freq[0]*0.96/4;
	freq[22] = freq[1]*0.96/4;
	freq[23] = freq[2]*0.96/4;
	freq[24] = freq[0]*1.04*4;
	freq[25] = freq[1]*1.04*4;
	freq[26] = freq[2]*1.04*4;
	freq[27] = freq[0]*0.95/8;
	freq[28] = freq[1]*0.95/8;
	freq[29] = freq[2]*0.95/8;
	freq[30] = freq[0]*1.05*8;
	freq[31] = freq[1]*1.05*8;
	freq[32] = freq[2]*1.05*8;
	}
//#endif
	}
	else if(voices_used>=24)
	{
/*
#if GRAIN_VOICES_MAX == 27
	freq[0] = bases[0];//FREQ0;
	freq[1] = freq[0]*0.97;
	freq[2] = freq[0]*0.93;
	freq[3] = bases[1];//freq[0]*MAJOR_THIRD;
	freq[4] = bases[2];//freq[0]*PERFECT_FIFTH;
	freq[5] = freq[3]*0.96;
	freq[6] = freq[4]*0.94;
	freq[7] = freq[0]/2;
	freq[8] = freq[1]/2;
	freq[9] = freq[2]/2;
	freq[10] = freq[3]/2;
	freq[11] = freq[4]/2;
	freq[12] = freq[5]/2;
	freq[13] = freq[6]/2;
	freq[14] = freq[0]/4;
	freq[15] = freq[1]/4;
	freq[16] = freq[2]/4;
	freq[17] = freq[3]/4;
	freq[18] = freq[4]/4;
	freq[19] = freq[5]/4;
	freq[20] = freq[6]/4;
	freq[21] = freq[0]/8;
	freq[22] = freq[1]/8;
	freq[23] = freq[2]/8;
	freq[24] = freq[3]/8;
	freq[25] = freq[4]/8;
	freq[26] = freq[5]/8;
#endif
*/

//#if GRAIN_VOICES_MAX == 27
	if(part!=2)
	{
	freq[0] = bases[0];//FREQ_C5;
	freq[1] = bases[1];//freq[0]*MAJOR_THIRD;
	freq[2] = bases[2];//freq[0]*PERFECT_FIFTH;
	freq[3] = freq[0]*0.99*2;
	freq[4] = freq[1]*0.99*2;
	freq[5] = freq[2]*0.99*2;
	freq[6] = freq[0]*1.01/2;
	freq[7] = freq[1]*1.01/2;
	freq[8] = freq[2]*1.01/2;
	freq[9] = freq[0]*0.98/4;
	freq[10] = freq[1]*0.98/4;
	freq[11] = freq[2]*0.98/4;
	freq[12] = freq[0]*1.02/8;
	freq[13] = freq[1]*1.02/8;
	}
	if(part!=1)
	{
	freq[14] = freq[2]*1.02/8;
	freq[15] = freq[0]*0.995*4;
	freq[16] = freq[1]*0.995*4;
	freq[17] = freq[2]*0.995*4;
	freq[18] = freq[0]*1.005*8;
	freq[19] = freq[1]*1.005*8;
	freq[20] = freq[2]*1.005*8;
	freq[21] = freq[0]*1.012*16;
	freq[22] = freq[1]*1.012*16;
	freq[23] = freq[2]*1.012*16;
	if(voices_used>24)
	{
		freq[24] = freq[0]*0.987/16;
		freq[25] = freq[1]*0.987/16;
		freq[26] = freq[2]*0.987/16;
	}
	}
//#endif
	}
}
