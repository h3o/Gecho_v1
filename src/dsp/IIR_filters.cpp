/*
 * IIR_filters.cpp
 *
 *  Created on: Apr 18, 2016
 *      Author: mayo
 *
 *  Based on code by Paul Kellett
 *  Source: http://www.musicdsp.org/showone.php?id=29
 *
 *  This file is part of the Gecho Loopsynth Firmware Development Framework.
 *  It can be used within the terms of CC-BY-NC-SA license.
 *  It must not be distributed separately.
 *
 *  Find more information at: http://gechologic.com/gechologists/
 */

#include "IIR_filters.h"

int IIR_Filter::BoundaryFn001(void)
{
	int c;
	c = 0x1111;
	c = 0x2222;
	c = 0x3333;
	c = 0x4444;
	c++;
	return c;
}

IIR_Filter::IIR_Filter(void)
{
	cutoff = 0.99;
	resonance = 0.0;
	buf0 = 0.0;
	buf1 = 0.0;
	buf2 = 0.0;
	buf3 = 0.0;
	calculateFeedbackAmount();
}

//virtual float IIR_Filter::process(float inputValue) { };
/*
float IIR_Filter::process(float inputValue) {
    //buf0 += cutoff * (inputValue - buf0);
    buf0 += cutoff * (inputValue - buf0 + feedbackAmount * (buf0 - buf1)); //with resonance
    buf1 += cutoff * (buf0 - buf1);

    //2nd order outputs:

    //return buf1;					//low-pass
    //return inputValue - buf0;		//high-pass
    //return buf0 - buf1;			//band-pass

    buf2 += cutoff * (buf1 - buf2);
    buf3 += cutoff * (buf2 - buf3);

    //4th order outputs:

    return buf3; 					//low-pass (default)
    //return inputValue - buf3;		//high-pass
    //return buf0 - buf3;			//band-pass
};
*/

float IIR_Filter_LOW_PASS_4TH_ORDER::process(float inputValue) {
    //buf0 += cutoff * (inputValue - buf0);
    buf0 += cutoff * (inputValue - buf0 + feedbackAmount * (buf0 - buf1)); //with resonance
    buf1 += cutoff * (buf0 - buf1);

    //2nd order outputs:

    //return buf1;					//low-pass
    //return inputValue - buf0;		//high-pass
    //return buf0 - buf1;			//band-pass

    buf2 += cutoff * (buf1 - buf2);
    buf3 += cutoff * (buf2 - buf3);

    //4th order outputs:

    return buf3; 					//low-pass (default)
    //return inputValue - buf3;		//high-pass
    //return buf0 - buf3;			//band-pass
}

float IIR_Filter_HIGH_PASS_4TH_ORDER::process(float inputValue) {
    buf0 += cutoff * (inputValue - buf0 + feedbackAmount * (buf0 - buf1)); //with resonance
    buf1 += cutoff * (buf0 - buf1);
    buf2 += cutoff * (buf1 - buf2);
    buf3 += cutoff * (buf2 - buf3);
    return inputValue - buf3;		//high-pass
}

float IIR_Filter::iir_filter_multi_sum(float input, IIR_Filter *iir_array, int total_filters, float *mixing_volumes)
{
	float output = 0.0;

	for(int f=0;f<total_filters;f++) {
		output += iir_array[f].process(input)*mixing_volumes[f];
	}

	return output/(float)total_filters;
}


float IIR_Filter::iir_filter_multi_sum_w_noise(float input, IIR_Filter *iir_array, int total_filters, float *mixing_volumes, uint16_t noise, float noise_volume)
{
	float output = 0.0;
	input += (float)(32768 - noise) / 32768.0f * noise_volume;

	/*
	if(total_filters > 100) //trap
	{
		total_filters++;
	}
	*/

	for(int f=0;f<total_filters;f++) {
		output += iir_array[f].process(input) * mixing_volumes[f];
		//noise <<= 1;
		//noise |= f;
	}

	return output/(float)total_filters;
}

int IIR_Filter::BoundaryFn002(void)
{
	int c;
	c = 0x4444;
	c = 0x3333;
	c = 0x2222;
	c = 0x1111;
	c++;
	return c;
}
