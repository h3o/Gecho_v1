/*
 * Reverb.cpp
 *
 *  Created on: 17 July 2018
 *      Author: mario
 *
 *  This file is part of the Gecho Loopsynth Firmware Development Framework.
 *  It can be used within the terms of CC-BY-NC-SA license.
 *
 *  Find more information at: http://gechologic.com/gechologists/
 */

#include <extensions/Reverb.h>
#include <extensions/Granular.h> //for add_echo()
//#include <MusicBox.h>
//#include <songs.h>
#include <hw/eeprom.h>

void reverb_with_echo(int mode)
{
	//there is an external sampleCounter variable already, it will be used to timing and counting seconds
	sampleCounter = 0;
	seconds = 0;

	//set input gain
	const float op_gain = PREAMP_BOOST * UNFILTERED_SIGNAL_VOLUME * 2;

	if(mode)
	{
	}
	else
	{
	}

	//-------------------------------------------------------------------

	KEY_LED_all_off(); //turn off all LEDs
	while(ANY_USER_BUTTON_ON); //wait till all buttons released (some may still be pressed from selecting a channel)
	ADC_configure_SENSORS(ADCConvertedValues); //configure IR proximity sensors

	//init and configure audio codec
	codec_init();
	codec_ctrl_init();
	I2S_Cmd(CODEC_I2S, ENABLE);

	//initialize echo without using init_echo_buffer() which allocates too much memory
	#define REVERB_ECHO_BUFFER_SIZE (I2S_AUDIOFREQ*2) //one second (buffer is shared for both stereo channels)
	echo_dynamic_loop_length = REVERB_ECHO_BUFFER_SIZE; //set default value (can be changed dynamically)
	echo_buffer = (int16_t*)malloc(echo_dynamic_loop_length*sizeof(int16_t)); //allocate memory
	memset(echo_buffer,0,echo_dynamic_loop_length*sizeof(int16_t)); //clear memory
	echo_buffer_ptr0 = 0; //reset pointer

    ECHO_MIXING_GAIN_MUL = 9; //amount of signal to feed back to echo loop, expressed as a fragment
    ECHO_MIXING_GAIN_DIV = 10; //e.g. if MUL=2 and DIV=3, it means 2/3 of signal is mixed in

	program_settings_reset();

	int BIT_CRUSHER_REVERB_MIN, BIT_CRUSHER_REVERB_MAX;

	if(mode==1)
	{
		//v1: short reverb (higher freqs)
		BIT_CRUSHER_REVERB_MIN = (REVERB_BUFFER_LENGTH/50);
		BIT_CRUSHER_REVERB_MAX = (REVERB_BUFFER_LENGTH/4);
	}
	else if(mode==0)
	{
		//v2: longer reverb (bass)
		BIT_CRUSHER_REVERB_MIN = (REVERB_BUFFER_LENGTH/5);
		BIT_CRUSHER_REVERB_MAX = REVERB_BUFFER_LENGTH;
		//BIT_CRUSHER_REVERB_DEFAULT = (BIT_CRUSHER_REVERB_MIN*10);
		//BIT_CRUSHER_REVERB_DEFAULT = (BIT_CRUSHER_REVERB_MIN*20);
	}
	else if(mode==2)
	{
		BIT_CRUSHER_REVERB_MIN = (REVERB_BUFFER_LENGTH/50);
		BIT_CRUSHER_REVERB_MAX = REVERB_BUFFER_LENGTH;
	}

	int BIT_CRUSHER_REVERB_DEFAULT = BIT_CRUSHER_REVERB_MIN;

	int sensors_controls = 0;
	int bit_crusher_reverb_direction = 1;

	if(mode==2)
	{
		BIT_CRUSHER_REVERB_DEFAULT = BIT_CRUSHER_REVERB_MIN + (BIT_CRUSHER_REVERB_MAX-BIT_CRUSHER_REVERB_MIN)/2;
		sensors_controls = 1;
		bit_crusher_reverb_direction = 0;
	}

	int bit_crusher_reverb = BIT_CRUSHER_REVERB_DEFAULT;

    REVERB_MIXING_GAIN_MUL = 9; //amount of signal to feed back to reverb loop, expressed as a fragment
    REVERB_MIXING_GAIN_DIV = 10; //e.g. if MUL=2 and DIV=3, it means 2/3 of signal is mixed in

    reverb_dynamic_loop_length = I2S_AUDIOFREQ / BIT_CRUSHER_REVERB_DEFAULT; //set default value (can be changed dynamically)
	//reverb_buffer = (int16_t*)malloc(reverb_dynamic_loop_length*sizeof(int16_t)); //allocate memory
	memset(reverb_buffer,0,REVERB_BUFFER_LENGTH*sizeof(int16_t)); //clear memory
	reverb_buffer_ptr0 = 0; //reset pointer

	while(1) //function loop
	{
		sampleCounter++;
		if (TIMING_BY_SAMPLE_ONE_SECOND_W_CORRECTION) //one full second passed
		{
			seconds++;
			sampleCounter = 0;
		}

		if (TIMING_BY_SAMPLE_EVERY_250_MS == 24) //4Hz
		{
			bit_crusher_reverb += bit_crusher_reverb_direction;

			if(bit_crusher_reverb>=BIT_CRUSHER_REVERB_MAX || bit_crusher_reverb<=BIT_CRUSHER_REVERB_MIN)
			{
				//bit_crusher_reverb = BIT_CRUSHER_REVERB_MAX;
				//bit_crusher_reverb = BIT_CRUSHER_REVERB_MIN;
				bit_crusher_reverb_direction = -bit_crusher_reverb_direction;
			}
			reverb_dynamic_loop_length = bit_crusher_reverb;
		}

		sample_mix = (int16_t)(4096/2 - (int16_t)ADC1_read()) / 4096.0f * op_gain;

		//send data to codec, with added reverb and echo
        while (!SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE));
        SPI_I2S_SendData(CODEC_I2S, add_echo(add_reverb((int16_t)(sample_mix))));

		#define sensor_values ADC_last_result
        if (TIMING_BY_SAMPLE_EVERY_25_MS == 40) //40Hz periodically, at 40ms
		{
			if(sensors_controls)
			{
				if (SENSOR_THRESHOLD_RED_8)
				{
					bit_crusher_reverb +=5;
				}
				else if (SENSOR_THRESHOLD_RED_6)
				{
					bit_crusher_reverb +=3;
				}
				else if (SENSOR_THRESHOLD_RED_4)
				{
					bit_crusher_reverb ++;
				}
				else if (SENSOR_THRESHOLD_RED_2)
				{
					bit_crusher_reverb_direction = 0;
				}

				if (SENSOR_THRESHOLD_WHITE_1)
				{
					bit_crusher_reverb -=5;
				}
				else if (SENSOR_THRESHOLD_WHITE_3)
				{
					bit_crusher_reverb -=3;
				}
				else if (SENSOR_THRESHOLD_WHITE_5)
				{
					bit_crusher_reverb --;
				}
				else if (SENSOR_THRESHOLD_WHITE_7)
				{
					bit_crusher_reverb_direction = 0;
				}

				if(bit_crusher_reverb>BIT_CRUSHER_REVERB_MAX)
				{
					bit_crusher_reverb = BIT_CRUSHER_REVERB_MAX;
				}

				if(bit_crusher_reverb<BIT_CRUSHER_REVERB_MIN)
				{
					bit_crusher_reverb = BIT_CRUSHER_REVERB_MIN;
				}

				if (SENSOR_THRESHOLD_ORANGE_2)
				{
					bit_crusher_reverb_direction = 1;
				}

				if (SENSOR_THRESHOLD_BLUE_2)
				{
					bit_crusher_reverb_direction = -1;
				}
			}
		}

		/*
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
		*/

        sample_mix = (int16_t)(4096/2 - (int16_t)ADC2_read()) / 4096.0f * op_gain;

		//send data to codec, with added reverb and echo
        while (!SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE));
        SPI_I2S_SendData(CODEC_I2S, add_echo(add_reverb((int16_t)(sample_mix))));

        /*
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
		*/

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
			if(echo_dynamic_loop_length > REVERB_ECHO_BUFFER_SIZE)
			{
				echo_dynamic_loop_length = REVERB_ECHO_BUFFER_SIZE; //set default value
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

int16_t add_reverb(int16_t sample)
{
	//wrap the reverb loop
	reverb_buffer_ptr0++;
	if (reverb_buffer_ptr0 >= reverb_dynamic_loop_length)
	{
		reverb_buffer_ptr0 = 0;
	}

	reverb_buffer_ptr = reverb_buffer_ptr0 + 1;
	if (reverb_buffer_ptr >= reverb_dynamic_loop_length)
	{
		reverb_buffer_ptr = 0;
	}

	//add reverb from the loop
	reverb_mix_f = float(sample) + float(reverb_buffer[reverb_buffer_ptr]) * REVERB_MIXING_GAIN_MUL / REVERB_MIXING_GAIN_DIV;

	if (reverb_mix_f > COMPUTED_SAMPLE_MIXING_LIMIT_UPPER)
	{
		reverb_mix_f = COMPUTED_SAMPLE_MIXING_LIMIT_UPPER;
	}

	if (reverb_mix_f < COMPUTED_SAMPLE_MIXING_LIMIT_LOWER)
	{
		reverb_mix_f = COMPUTED_SAMPLE_MIXING_LIMIT_LOWER;
	}

	sample = (int16_t)reverb_mix_f;

	//store result to reverb, the amount defined by a fragment
	//reverb_mix_f = ((float)sample_i16 * REVERB_MIXING_GAIN_MUL / REVERB_MIXING_GAIN_DIV);
	//reverb_mix_f *= REVERB_MIXING_GAIN_MUL / REVERB_MIXING_GAIN_DIV;
	//reverb_buffer[reverb_buffer_ptr0] = (int16_t)reverb_mix_f;
	reverb_buffer[reverb_buffer_ptr0] = sample;

	return sample;
}

