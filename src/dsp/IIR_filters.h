/*
 * IIR_filters.h
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

#ifndef IIR_FILTERS_H_
#define IIR_FILTERS_H_

#include <hw/signals.h>

class IIR_Filter
{
public:
	enum FilterMode {
		FILTER_MODE_LOWPASS = 0,
		FILTER_MODE_HIGHPASS,
		FILTER_MODE_BANDPASS,
		kNumFilterModes
	};

	int BoundaryFn001(void);

	IIR_Filter(void);
	//virtual float process(float inputValue);
	virtual float process(float inputValue) { return 0.0f; };
	//float process(float inputValue);
	//float process_HIGH_PASS_4TH_ORDER(float inputValue);
	//float process_LOW_PASS_4TH_ORDER(float inputValue);

	//inline void setCutoff(float newCutoff) { cutoff = newCutoff; calculateFeedbackAmount(); };
	inline void setCutoff(float newCutoff) { cutoff = newCutoff; feedbackAmount = resonance + resonance/(1.0 - cutoff); };
	inline void setCutoffKeepFeedback(float newCutoff) { cutoff = newCutoff; };

	inline void setCutoffAndLimits(float newCutoff) {
		if(newCutoff < 0.001)
		{
			newCutoff = 0.247836739 / 2;
		}
		cutoff = newCutoff;
		cutoff_min = cutoff / 2;
		cutoff_max = cutoff * 2;
		calculateFeedbackAmount();
	};

	inline void driftCutoff(float drift) {
		cutoff += drift;
		if(cutoff<cutoff_min)
		{
			cutoff = cutoff_min;
		}
		else if(cutoff>cutoff_max)
		{
			cutoff = cutoff_max;
		}
		calculateFeedbackAmount();
	};

	inline void setResonance(float newResonance) { resonance = newResonance; calculateFeedbackAmount(); };
	inline void setResonanceKeepFeedback(float newResonance) { resonance = newResonance; };
	inline void setResonanceAndFeedback(float newResonance, float newFeedback) { resonance = newResonance; feedbackAmount = newFeedback; };

	inline void resetFilterBuffers() { buf0 = 0; buf1 = 0; buf2 = 0; buf3 = 0; };
	inline void disturbFilterBuffers() { buf0 /= 2; buf1 = 0; };

	static float iir_filter_multi_sum(float input, IIR_Filter *iir_array, int total_filters, float *mixing_volumes);
	static float iir_filter_multi_sum_w_noise(float input, IIR_Filter *iir_array, int total_filters, float *mixing_volumes, uint16_t noise, float noise_volume);

	int BoundaryFn002(void);

protected:
//private:

	float cutoff, resonance, feedbackAmount;
    float buf0,buf1,buf2,buf3;
    float cutoff_min, cutoff_max;
    inline void calculateFeedbackAmount() { feedbackAmount = resonance + resonance/(1.0 - cutoff); }
};

class IIR_Filter_LOW_PASS_4TH_ORDER : public IIR_Filter
{
public:
	float process(float inputValue);
};

class IIR_Filter_HIGH_PASS_4TH_ORDER : public IIR_Filter
{
public:
	float process(float inputValue);
};

#endif /* IIR_FILTERS_H_ */
