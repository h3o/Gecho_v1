/*
 * Granular.cpp
 *
 *  Created on: 26 Jan 2018
 *      Author: mario
 *
 *  As explained in the coding tutorial: http://gechologic.com/granular-sampler
 *
 *  This file is part of the Gecho Loopsynth Firmware Development Framework.
 *  It can be used within the terms of CC-BY-NC-SA license.
 *
 *  Find more information at: http://gechologic.com/gechologists/
 */

#include "Sequencer.h"
#include "Drums.h"

void drum_sequencer()
{
	//--------------------------------------------------------------------------
	//defines, constants and variables

	#define BAR_RESOLUTION 256 //how many events to record per bar (which is usually 0.5s)

	int sequence_length; //user-defined sequence length in bars, one bar usually corresponds to half a second
	int sequence_bars, sequence_verses;
	int verse = 0, bar = -1; //bar needs to be at position 0 at the first increment
	char *events;
	int events_n, event_position;

	//this selector is recycled from custom_song_programming_mode()
	//we have no chords here, but this needs to works the same way
	sequence_length = set_number_of_chords(&sequence_bars, &sequence_verses);

	events_n = sequence_length * BAR_RESOLUTION;
	events = (char*)malloc(events_n); //allocate memory for events
	memset(events,0,events_n); //clear the events array (set all elements to 0)

	float drum_kit_mixing_sample; //variable to mix all drum samples
	int metronome_ptr = -1; //"metronome" sample pointer, stopped initially

	int metronome_enabled = 1;
	int btn_held = 0;

	//mapping coefficient for determining event position by sampleCounter
	float sample_to_events = (float)BAR_RESOLUTION / (float)I2S_AUDIOFREQ;

	KEY_LED_all_off(); //turn off all LEDs
	while(ANY_USER_BUTTON_ON); //wait till all buttons released (some may still be pressed from selecting a channel)
	ADC_configure_SENSORS(ADCConvertedValues); //configure IR proximity sensors

	//init and configure audio codec
	codec_init();
	codec_ctrl_init();
	I2S_Cmd(CODEC_I2S, ENABLE);

	drum_kit_process(); //when run for the first time, this will init the drum kit

	while(1)
	{
		sampleCounter++;

		if (TIMING_BY_SAMPLE_EVERY_500_MS==0) //2Hz periodically, at 0ms
		{
			bar++;
			if(bar==sequence_bars)
			{
				bar = 0;
				verse++;
				if(verse==sequence_verses)
				{
					verse = 0;
				}
			}

			//light up a Red LED according to current bar
			LED_R8_all_OFF();
			LED_R8_set(bar, 1);

			//light up an Orange LED according to current verse
			LED_O4_all_OFF();
			LED_O4_set(verse, 1);
		}

		sample_mix = 0; //mix samples for all voices (left or right channel, based on sampleCounter)

		event_position = (sampleCounter % (I2S_AUDIOFREQ*sequence_length)) * sample_to_events;

		//process the drums
    	for (int i=0;i<DRUM_CHANNELS_MAX;i++)
    	{
			if(events[event_position] & 2<<i)
			{
				drum_trigger[i] = 1;		//flip the trigger
				drum_samples_ptr[i] = 0;	//set pointer to the beginning of the sample

			}
			else if ((ADC_last_result[i] > DRUM_SENSOR_THRESHOLD(i)) && !drum_trigger[i]) //if not playing and IR Sensor threshold detected, start playing
			{
				drum_trigger[i] = 1;		//flip the trigger
				drum_samples_ptr[i] = 0;	//set pointer to the beginning of the sample

				//store the event
				events[event_position] += (2<<i);
			}
    		if ((ADC_last_result[i] < DRUM_SENSOR_THRESHOLD(i)) && drum_trigger[i])	//if playing and IR Sensor reports finger moved away, allow restart
    		{
    			drum_trigger[i] = 0;		//release trigger
    		}
    	}

    	if (sampleCounter & 0x00000001) //left stereo channel
		{
    		drum_kit_mixing_sample = 0; //clear the previous value

    		for (int i=0;i<DRUM_CHANNELS_MAX;i++)
    		{
    			if (drum_samples_ptr[i] >= 0) //if playing
    			{
    				//translate from 16-bit binary format to float
    				drum_kit_mixing_sample += ((float)((int16_t*)(drum_bases[i]))[drum_samples_ptr[i]]) * DRUM_SAMPLE_VOLUME;

    				drum_samples_ptr[i]++;						//move on to the next sample
    				if (drum_samples_ptr[i]==drum_lengths[i])	//if reached the end of the sample
    				{
    					drum_samples_ptr[i] = -1;				//reset the pointer to "stopped" state
    				}
    			}
    		}
    	}

    	//add metronome at 120bpm
		#define METRONOME_SAMPLE 1
		#define METRONOME_SAMPLE_VOLUME (DRUM_SAMPLE_VOLUME/2)

    	if (TIMING_BY_SAMPLE_EVERY_500_MS == 0)
    	{
    		if(metronome_enabled)
    		{
    			metronome_ptr = 0; //start playing
    		}
    	}
    	if(metronome_ptr != -1)
    	{
    		drum_kit_mixing_sample += ((float)((int16_t*)(drum_bases[METRONOME_SAMPLE]))[metronome_ptr]) * METRONOME_SAMPLE_VOLUME;
    		//metronome_ptr += 4; //play at two octaves higher
    		metronome_ptr += 8; //play at three octaves higher

			if (metronome_ptr >= drum_lengths[METRONOME_SAMPLE]) //if reached the end of the sample
			{
				metronome_ptr = -1; //reset the pointer to "stopped" state
			}
    	}

    	sample_mix += (int16_t)(drum_kit_mixing_sample * FLASH_SAMPLES_MIXING_VOLUME); //return sample

		//send data to codec
		while (!SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE));
        SPI_I2S_SendData(CODEC_I2S, (int16_t)(sample_mix));

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

			if(BUTTON_U4_ON)
			{
				if(!btn_held)
				{
					metronome_enabled = metronome_enabled?0:1;
					btn_held = 1;
				}
			}
			else
			{
				btn_held = 0;
			}
		}

		//process any I2C commands in queue (e.g. volume change by buttons)
		queue_codec_ctrl_process();
	}
}
