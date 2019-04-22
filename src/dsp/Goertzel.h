/*
 * Goertzel.h
 *
 *  Created on: Jun 29, 2016
 *      Author: mayo
 *
 *  Based on code by Kevin Banks
 *  Source: http://www.embedded.com/design/configurable-systems/4024443/The-Goertzel-Algorithm
 *
 */

#ifndef GOERTZEL_H_
#define GOERTZEL_H_

#include "stm32f4xx.h"

#define SAMPLING_RATE		22050.0	//22kHz
//#define TARGET_FREQUENCY	941.0	//941 Hz

class Goertzel {

	float coeff;
	float Q1;
	float Q2;
	float sine;
	float cosine;

public:
	Goertzel();
	virtual ~Goertzel();
	void reset();
	void init(float TARGET_FREQUENCY, int N);	//N=block size
	void process_sample(uint16_t sample);
	void get_real_imag(float *realPart, float *imagPart);
	float get_magnitude_squared(void);
};

#endif /* GOERTZEL_H_ */
