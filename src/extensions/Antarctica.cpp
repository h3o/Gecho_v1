/*
 * Antarctica.cpp
 *
 *  Created on: Sep 24, 2016
 *      Author: mayo
 *
 *  This file is part of the Gecho Loopsynth Firmware Development Framework.
 *  It can be used within the terms of CC-BY-NC-SA license.
 *  It must not be distributed separately.
 *
 *  Find more information at: http://gechologic.com/gechologists/
 */

#include <dsp/IIR_filters.h>
#include <hw/codec.h>
#include <hw/gpio.h>
#include <hw/signals.h>
#include <hw/controls.h>
#include "Antarctica.h"
#include <string.h>

volatile uint32_t a_sampleCounter = 0;
volatile int16_t a_sample_i16 = 0;

//#define WIND_FILTERS 2
//int wind_freqs[] = {880};
//float cutoff[WIND_FILTERS] = {0.159637183,0.159637183};
//float cutoff_limit[2] = {0.079818594};

#define WIND_FILTERS 4
int wind_freqs[] = {880,880};
float cutoff[WIND_FILTERS] = {0.159637183,0.159637183,0.159637183,0.159637183};
float cutoff_limit[2] = {0.079818594,0.319274376};
//float cutoff[WIND_FILTERS] = {0.359637183,0.459637183,0.4159637183,0.3159637183};
//float cutoff_limit[2] = {0.279818594,0.5319274376};
//float cutoff[WIND_FILTERS] = {0.1759637183,0.18159637183,0.1578159637183,0.1983159637183};
//float cutoff_limit[2] = {0.15,0.20};

//#define WIND_FILTERS 8
//int wind_freqs[] = {880,880,880,880};
//float cutoff[WIND_FILTERS] = {0.159637183,0.159637183,0.159637183,0.159637183,0.159637183,0.159637183,0.159637183,0.159637183};
//float cutoff_limit[2] = {0.079818594,0.319274376};

/*
#define WIND_FILTERS 12
int wind_freqs[] = {880,880,880,880,880,880};
float cutoff[WIND_FILTERS] = {0.159637183,0.159637183,0.159637183,0.159637183,0.159637183,0.159637183,0.159637183,0.159637183,0.159637183,0.159637183,0.159637183,0.159637183};
float cutoff_limit[2] = {0.079818594,0.319274376};
*/

IIR_Filter *a_iir;

uint32_t a_random_value;

float a_sample[WIND_FILTERS/2],a_sample_mix;
unsigned long a_seconds;

float A_SAMPLE_VOLUME = 2.0f; //0.375f;

float resonance = 0.970;
float feedback;
float resonance_limit[2] = {0.950,0.995};
float feedback_limit[2];

float a_volume = 400.0f; //400.0f;

int i, cutoff_sweep = 0;

float mixing_volumes[WIND_FILTERS];
int mixing_deltas[WIND_FILTERS];

#define BUTTON_SET_ON	(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0)==1)	//SET button: PA0

void filter_setup02()
{
	a_iir = new IIR_Filter_LOW_PASS_4TH_ORDER[WIND_FILTERS];

	for(i=0;i<WIND_FILTERS;i++)
	{
		a_iir[i].setResonance(resonance);
		a_iir[i].setCutoff((float)wind_freqs[i%(WIND_FILTERS/2)] / (float)I2S_AUDIOFREQ * 2 * 2);
		mixing_volumes[i]=1.0f;

		mixing_deltas[i]=0;
	}

	//feedback boundaries
	feedback = 0.96;
	feedback_limit[0] = 0.5;
	feedback_limit[1] = 1.0;
}

void song_of_wind_and_ice()
{
	filter_setup02();

    a_seconds = 0;

    float signal_min = 100000, signal_max = -100000;
    float signal_max_limit = 1100;
    float signal_min_limit = -250;

	while(1)
    {
		if (!(a_sampleCounter & 0x00000001)) //right channel
		{
			float r = PseudoRNG1a_next_float();
			memcpy(&a_random_value, &r, sizeof(a_random_value));

    		a_sample[0] = (float)(32768 - (int16_t)a_random_value) / 32768.0f;
    		a_sample[1] = (float)(32768 - (int16_t)(a_random_value>>16)) / 32768.0f;
		}

    	while(!SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE));

		SPI_I2S_SendData(CODEC_I2S, a_sample_i16);

		pwr_button_shutdown();

		if (a_sampleCounter & 0x00000001) //left channel
		{
			a_sample_mix = IIR_Filter::iir_filter_multi_sum(a_sample[0], a_iir, WIND_FILTERS/2, mixing_volumes) * a_volume;
		}
		else
		{
			a_sample_mix = IIR_Filter::iir_filter_multi_sum(a_sample[1], a_iir+WIND_FILTERS/2, WIND_FILTERS/2, mixing_volumes+WIND_FILTERS/2) * a_volume;
		}

		if (a_sample_mix > signal_max)
		{
			signal_max = a_sample_mix;
		}
		if (a_sample_mix < signal_min)
		{
			signal_min = a_sample_mix;
		}

		if (a_sample_mix > signal_max_limit)
		{
			a_sample_mix = signal_max_limit;
		}
		if (a_sample_mix < signal_min_limit)
		{
			a_sample_mix = signal_min_limit;
		}

		a_sample_i16 = (int16_t)(a_sample_mix * A_SAMPLE_VOLUME);

		a_sampleCounter++;

    	if (a_sampleCounter==I2S_AUDIOFREQ)
    	{
    		//LED_BLUE_OFF;
    	}
    	else if (a_sampleCounter==2*I2S_AUDIOFREQ)
    	{
    		//LED_BLUE_ON;
    		a_sampleCounter = 0;
    		a_seconds++;

    		if (a_seconds % 3 == 0)
    		{
    			for (int d=0; d<WIND_FILTERS; d++)
    			{
    				mixing_deltas[d] = (a_random_value >> d) & 0x00000001;
    			}
    		}
    	}
        if (a_sampleCounter%(2*I2S_AUDIOFREQ/10)==0) //every 100ms
    	{
        	cutoff_sweep++;
        	cutoff_sweep %= WIND_FILTERS;

        	cutoff[cutoff_sweep] += 0.005 - (float)(0.01 * ((a_random_value) & 0x00000001));
        	if (cutoff[cutoff_sweep] < cutoff_limit[0])
        	{
        		cutoff[cutoff_sweep] = cutoff_limit[0];
        	}
        	else if (cutoff[cutoff_sweep] > cutoff_limit[1])
        	{
        		cutoff[cutoff_sweep] = cutoff_limit[1];
        	}

			mixing_volumes[cutoff_sweep] += mixing_deltas[cutoff_sweep] * 0.01f;
			if (mixing_volumes[cutoff_sweep] < 0)
			{
				mixing_volumes[cutoff_sweep] = 0;
			}
			else if (mixing_volumes[cutoff_sweep] > 1)
			{
				mixing_volumes[cutoff_sweep] = 1;
			}

			a_iir[cutoff_sweep].setCutoff(cutoff[cutoff_sweep]);
    	}

        queue_codec_ctrl_process();

        if (a_sampleCounter%(2*I2S_AUDIOFREQ/10)==3) //every 100ms - 10Hz periodically, at sample #3
		{
			check_for_reset();
		}
    	if (a_sampleCounter%(2*I2S_AUDIOFREQ/10)==2345) //every 100ms - 10Hz periodically, at sample #2345
    	{
    		buttons_controls_during_play();
    	}
    }
}
