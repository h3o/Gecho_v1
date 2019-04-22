/*
 * Samples.c
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

#include "Samples.h"
#include <hw/controls.h>
#include <hw/signals.h>

//toy box sample
#define SAMPLE_BASE_ADDR1 0x808C960			//base address where we uploaded samples
#define SAMPLE_LENGTH1 88480				//length of first sample in bytes

#define FLASH_SAMPLES 1						//we uploaded 1 sample to FLASH memory

int flash_samples_ptr[FLASH_SAMPLES]; 		//pointers for currently playing sample
int fhash_sample_trigger[FLASH_SAMPLES];	//triggers for execution of each sample
int flash_sample_lengths[FLASH_SAMPLES];	//lengths of samples in bytes
int flash_sample_bases[FLASH_SAMPLES];		//base addresses of samples
int flash_sample_init = 0;					//has the sample playback been initialized?
float flash_sample_return_value = 0;		//variable to mix all samples into one value
float flash_sample_note_freq = 0;

#define FLASH_SAMPLE_VOLUME 0.04f;			//general adjustment volume for samples we used

extern volatile int16_t sample_i16;
extern float sample_mix;

uint16_t flash_sample_process(float playback_note_frequency)	//returns 16-bit sample
{
	int sample_ptr;

	if (!flash_sample_init)
	{
		flash_sample_init = 1;
		for (int i=0;i<FLASH_SAMPLES;i++)	//init all values
		{
			flash_samples_ptr[i] = -1;		//reset to "stopped" state
			fhash_sample_trigger[i] = 0;	//reset to "released" state
		}

		flash_sample_lengths[0] = SAMPLE_LENGTH1 / 2;	//each sample is 2 bytes
		flash_sample_bases[0] = SAMPLE_BASE_ADDR1;		//copy the values to variables for easy access in for loops
	}

	for (int i=0;i<FLASH_SAMPLES;i++)
	{
		if (playback_note_frequency>1)		//some frequency
		{
			flash_sample_note_freq = playback_note_frequency;	//store the new frequency
			fhash_sample_trigger[i] = 1;	//flip the trigger
			flash_samples_ptr[i] = 0;		//set pointer to the beginning of the sample
		}
	}

	if (sampleCounter%2==0)	//calculate the value only on each "left" sample
	{
		flash_sample_return_value = 0; //clear the previous value

		for (int i=0;i<FLASH_SAMPLES;i++)
		{
			if (flash_samples_ptr[i] >= 0) //if playing
			{
				//translate from 16-bit binary format to float
				sample_ptr = flash_samples_ptr[i] * flash_sample_note_freq / 523.0f * 2;

				if (sample_ptr < flash_sample_lengths[i]) //do not overrun real sample length
				{
					flash_sample_return_value += ((float)((int16_t*)(flash_sample_bases[i]))[sample_ptr]) * FLASH_SAMPLE_VOLUME;
				}

				flash_samples_ptr[i]++;								//move on to the next sample
				if (flash_samples_ptr[i]==flash_sample_lengths[i])	//if reached the end of the sample
				{
					flash_samples_ptr[i] = -1;						//reset the pointer to "stopped" state
				}
			}
		}
	}
	else //return previous value for the other channel
	{
		//nothing needs to be done
	}
	return (int16_t)(flash_sample_return_value * FLASH_SAMPLES_MIXING_VOLUME); //return sample
}

void play_buffer(uint16_t *buffer, int buffer_length, int channels)
{
	int buffer_ptr = 0;

	codec_ctrl_init();
	I2S_Cmd(CODEC_I2S, ENABLE);

	codec_volume = CODEC_VOLUME_MIN;

	CodecCommandBuffer[0] = CODEC_MAP_MASTER_A_VOL; //0x20
	CodecCommandBuffer[1] = codec_volume;
	send_codec_ctrl(CodecCommandBuffer, 2);
	CodecCommandBuffer[0] = CODEC_MAP_MASTER_B_VOL; //0x21
	CodecCommandBuffer[1] = codec_volume;
	send_codec_ctrl(CodecCommandBuffer, 2);

	for(int delay=0;delay<22050;delay++)
	{
		while (!SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE));
		SPI_I2S_SendData(CODEC_I2S, 0);
	}

	buffer_ptr = 0;

	for(int i=0;i<buffer_length/2;i++) //buffer length is in bytes, sample is word
	{
		while (!SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE));
		SPI_I2S_SendData(CODEC_I2S, buffer[buffer_ptr]);// * PLAYBACK_VOLUME);
		if(channels==2)
		{
			buffer_ptr++;
		}
		while (!SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE));
		SPI_I2S_SendData(CODEC_I2S, buffer[buffer_ptr]);// * PLAYBACK_VOLUME);
		buffer_ptr++;
	}

	codec_reset();
}

/*
void mix_in_buffer(uint16_t *buffer, int buffer_length, int channels, int volume_sample_rshift, unsigned int mask)
{
	int buffer_ptr = 0;

	for(int i=0;i<buffer_length/2;i++) //buffer length is in bytes, sample is word
	{
		sample_mix = (float)((int16_t)buffer[buffer_ptr]) / 10.0f;
		sample_i16 = (int16_t)(sample_mix * FLASH_SAMPLES_MIXING_VOLUME);

		while (!SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE));
		//SPI_I2S_SendData(CODEC_I2S, (buffer[buffer_ptr] >> volume_sample_rshift) & mask);// * PLAYBACK_VOLUME);
		SPI_I2S_SendData(CODEC_I2S, sample_i16);
		//if(channels==2)
		//{
		//	buffer_ptr++;
		//}
		while (!SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE));
		//SPI_I2S_SendData(CODEC_I2S, (buffer[buffer_ptr] >> volume_sample_rshift) & mask);// * PLAYBACK_VOLUME);
		SPI_I2S_SendData(CODEC_I2S, sample_i16);// * PLAYBACK_VOLUME);
		buffer_ptr++;
	}
}
*/

