/*
 * Detectors.cpp
 *
 *  Created on: Jun 30, 2016
 *      Author: mayo
 */

#include <dsp/Detectors.h>
#include <string.h>
#include <stdlib.h>

Detectors::Detectors(char *goertzel_octaves)
{
    gor = new Goertzel[GORS];
    gors_freqs = (float*)malloc(GORS * sizeof(float));

    gors_freq_notes = (char*)malloc(strlen(goertzel_octaves) * sizeof(char));
	memcpy(gors_freq_notes, goertzel_octaves, strlen(goertzel_octaves) * sizeof(char));
}

Detectors::~Detectors(void)
{
	free(gors_freqs);
	free(gors_freq_notes);
}

void Detectors::detectors_setup(int buffer_length)
{
    for(int i=0;i<GORS;i++)
  	{
  		gor[i].init(gors_freqs[i], buffer_length);
  	}
}

void Detectors::process_sample(float sample)
{
	for(int g=0;g<GORS;g++)
	{
		gor[g].process_sample(sample);
	}
}

int Detectors::find_max_freq()
{
	float mag, max = 0, limit = 300.0f;
	int max_index = -1;
	int mags[GORS];

	for(int i=0;i<GORS;i++)
	{
		mag = gor[i].get_magnitude_squared();
		if(mag > limit && mag > max)
		{
			max = mag;
			max_index = i;
		}
		mags[i] = mag;
	}

	mag = mags[0];

	return max_index;
}

void Detectors::reset_all()
{
	for(int i=0;i<GORS;i++)
  	{
  		gor[i].reset();
  	}
}

/*
void test_Goertzel_detectors()
{
	//test Goertzel

	//get 1/2s one channel sample into buffer with real and imaginary parts
	float *fft_test_buf;
	int fft_samples = 512;//256; //I2S_AUDIOFREQ/2;
	fft_test_buf = (float*)malloc(fft_samples * sizeof(float));
	float result1,result2;
	int timing;

	int led_blue_state=0;

	detectors *det = new detectors();
	det->detectors_setup(fft_samples);

	while(1)
	{
		led_blue_state++;
		if(led_blue_state%2)
		{
			LED_BLUE_ON;
		}
		else
		{
			LED_BLUE_OFF;
		}
		timing = get_millis();
		for(int i=0;i<fft_samples;i++)
		{
			fft_test_buf[i] = (float)(4096/2 - (int16_t)ADC2_read()) / 4096.0f * PREAMP_BOOST;

			//timing by Codec
			while (!SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE));
			SPI_I2S_SendData(CODEC_I2S, 0);
			while (!SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE));
			SPI_I2S_SendData(CODEC_I2S, 0);
		}
		timing = get_millis() - timing;

		timing = get_millis();
		for(int i=0;i<fft_samples;i++)
		{
			det->process_sample(fft_test_buf[i]);
		}
		timing = get_millis() - timing;

		int max_freq = det->find_max_freq();

		det->reset_all();

		int result = max_freq;

		if(result>-1)
		{
			result ++;

			if(result==1)
			{
				LED_SB1_ON;
			}
			else if(result==2)
			{
				LED_SB2_ON;
			}
			else if(result==3)
			{
				LED_SB3_ON;
			}
			else if(result==4)
			{
				LED_SB4_ON;
			}
			else if(result==5)
			{
				LED_SR1_ON;
			}
			else if(result==6)
			{
				LED_SR2_ON;
			}
			else if(result==7)
			{
				LED_SR3_ON;
			}
			else if(result==8)
			{
				LED_SR4_ON;
			}
		}
		else
		{
			LED_SB1_OFF;
			LED_SB2_OFF;
			LED_SB3_OFF;
			LED_SB4_OFF;
			LED_SR1_OFF;
			LED_SR2_OFF;
			LED_SR3_OFF;
			LED_SR4_OFF;
		}
	}
}

*/
