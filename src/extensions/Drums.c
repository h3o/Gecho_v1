/*
 * Drums.c
 *
 *  Created on: Nov 26, 2016
 *      Author: mayo
 *
 *  This file is part of the Gecho Loopsynth Firmware Development Framework.
 *  It can be used within the terms of CC-BY-NC-SA license.
 *  It must not be distributed separately.
 *
 *  Find more information at: http://gechologic.com/gechologists/
 */

#include "Drums.h"
#include <hw/controls.h>
#include <hw/signals.h>
#include <hw/sensors.h>

int drum_samples_ptr[DRUM_CHANNELS_MAX]; 	//pointers for currently playing sample
int drum_trigger[DRUM_CHANNELS_MAX];		//triggers for execution of each drum
int drum_lengths[DRUM_SAMPLES];				//lengths of samples in bytes
int drum_bases[DRUM_SAMPLES];				//base addresses of samples
int drum_kit_init = 0;						//has drum kit been initialized?
float drum_machine_return_sample = 0;		//variable to mix all samples into one value

uint16_t drum_kit_process()	//returns 16-bit sample
{
	if (!drum_kit_init)
	{
		drum_kit_init = 1;
		for (int i=0;i<DRUM_CHANNELS_MAX;i++)	//init all values
		{
			drum_samples_ptr[i] = -1;			//reset to "stopped" state
			drum_trigger[i] = 0;				//reset to "released" state
		}

		drum_lengths[0] = DRUM_LENGTH1 / 2;		//each sample is 2 bytes
		drum_lengths[1] = DRUM_LENGTH2 / 2;
		drum_lengths[3] = DRUM_LENGTH3 / 2;		//swapped S3/S4
		drum_lengths[2] = DRUM_LENGTH4 / 2;
		drum_bases[0] = DRUM_BASE_ADDR1;		//copy the values to variables for easy access in for loops
		drum_bases[1] = DRUM_BASE_ADDR2;
		drum_bases[3] = DRUM_BASE_ADDR3;		//swapped S3/S4
		drum_bases[2] = DRUM_BASE_ADDR4;
	}

	for (int i=0;i<DRUM_CHANNELS_MAX;i++)
	{
		if (0)//i==1 && TEST_enable_V2_control_drum)
		{
			if ((ADC_measured_vals[i] > DRUM_SENSOR_THRESHOLD(i)) && !drum_trigger[i]) //if not playing and CV threshold detected, start playing
			{
				drum_trigger[i] = 1;		//flip the trigger
				drum_samples_ptr[i] = 0;	//set pointer to the beginning of the sample
			}
		}
		else
		{
			if ((ADC_last_result[i] > DRUM_SENSOR_THRESHOLD(i)) && !drum_trigger[i]) //if not playing and IR Sensor threshold detected, start playing
			{
				drum_trigger[i] = 1;		//flip the trigger
				drum_samples_ptr[i] = 0;	//set pointer to the beginning of the sample
			}
			if ((ADC_last_result[i] < DRUM_SENSOR_THRESHOLD(i)) && drum_trigger[i])	//if playing and IR Sensor reports finger moved away, allow restart
			{
				drum_trigger[i] = 0;		//release trigger
			}
		}
	}

	if (sampleCounter%2==0)	//calculate the value only on each "left" sample
	{
		drum_machine_return_sample = 0; //clear the previous value

		for (int i=0;i<DRUM_CHANNELS_MAX;i++)
		{
			if (drum_samples_ptr[i] >= 0) //if playing
			{
				//translate from 16-bit binary format to float
				drum_machine_return_sample += ((float)((int16_t*)(drum_bases[i]))[drum_samples_ptr[i]]) * DRUM_SAMPLE_VOLUME;

				drum_samples_ptr[i]++;						//move on to the next sample
				if (drum_samples_ptr[i]==drum_lengths[i])	//if reached the end of the sample
				{
					drum_samples_ptr[i] = -1;				//reset the pointer to "stopped" state
				}
			}
		}
	}
	else //return previous value for the other channel
	{
		//nothing needs to be done
	}
	return (int16_t)(drum_machine_return_sample * FLASH_SAMPLES_MIXING_VOLUME); //return sample
}

uint16_t drum_kit_process_cv_trigger()	//returns 16-bit sample
{
	if (!drum_kit_init)
	{
		drum_kit_init = 1;
		for (int i=0;i<DRUM_CHANNELS_MAX;i++)	//init all values
		{
			drum_samples_ptr[i] = -1;			//reset to "stopped" state
			drum_trigger[i] = 0;				//reset to "released" state
		}

		drum_lengths[0] = DRUM_LENGTH1 / 2;		//each sample is 2 bytes
		drum_lengths[1] = DRUM_LENGTH2 / 2;
		drum_lengths[3] = DRUM_LENGTH3 / 2;		//swapped S3/S4
		drum_lengths[2] = DRUM_LENGTH4 / 2;
		drum_bases[0] = DRUM_BASE_ADDR1;		//copy the values to variables for easy access in for loops
		drum_bases[1] = DRUM_BASE_ADDR2;
		drum_bases[3] = DRUM_BASE_ADDR3;		//swapped S3/S4
		drum_bases[2] = DRUM_BASE_ADDR4;
	}

	for (int i=0;i<DRUM_CHANNELS_MAX;i++)
	{
		if ((ADC_measured_vals[i] > DRUM_SENSOR_THRESHOLD(i)) && !drum_trigger[i]) //if not playing and IR Sensor threshold detected, start playing
		{
			drum_trigger[i] = 1;		//flip the trigger
			drum_samples_ptr[i] = 0;	//set pointer to the beginning of the sample
		}
		if ((ADC_measured_vals[i] < DRUM_SENSOR_THRESHOLD(i)) && drum_trigger[i])	//if playing and IR Sensor reports finger moved away, allow restart
		{
			drum_trigger[i] = 0;		//release trigger
		}
	}

	if (sampleCounter%2==0)	//calculate the value only on each "left" sample
	{
		drum_machine_return_sample = 0; //clear the previous value

		for (int i=0;i<DRUM_CHANNELS_MAX;i++)
		{
			if (drum_samples_ptr[i] >= 0) //if playing
			{
				//translate from 16-bit binary format to float
				drum_machine_return_sample += ((float)((int16_t*)(drum_bases[i]))[drum_samples_ptr[i]]) * DRUM_SAMPLE_VOLUME;

				drum_samples_ptr[i]++;						//move on to the next sample
				if (drum_samples_ptr[i]==drum_lengths[i])	//if reached the end of the sample
				{
					drum_samples_ptr[i] = -1;				//reset the pointer to "stopped" state
				}
			}
		}
	}
	else //return previous value for the other channel
	{
		//nothing needs to be done
	}
	return (int16_t)(drum_machine_return_sample * FLASH_SAMPLES_MIXING_VOLUME); //return sample
}
