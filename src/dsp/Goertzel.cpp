/*
 * Goertzel.cpp
 *
 *  Created on: Jun 29, 2016
 *      Author: mayo
 */

#include <dsp/Goertzel.h>
#include <stdio.h>
#include <math.h>

Goertzel::Goertzel() {
	// TODO Auto-generated constructor stub

}

Goertzel::~Goertzel() {
	// TODO Auto-generated destructor stub
}

/* Call this routine before every "block" (size=N) of samples. */
void Goertzel::reset()
{
	Q2 = 0;
	Q1 = 0;
}

/* Call this once, to precompute the constants. */
void Goertzel::init(float TARGET_FREQUENCY, int N) //N=block size
{
	int	k;
	float floatN;
	float omega;

	floatN = (float) N;
	k = (int) (0.5 + ((floatN * TARGET_FREQUENCY) / SAMPLING_RATE));
	omega = (2.0 * M_PI * k) / floatN;
	sine = sin(omega);
	cosine = cos(omega);
	coeff = 2.0 * cosine;

	//printf("For SAMPLING_RATE = %f", SAMPLING_RATE);
	//printf(" N = %d", N);
	//printf(" and FREQUENCY = %f,\n", TARGET_FREQUENCY);
	//printf("k = %d and coeff = %f\n\n", k, coeff);

	reset();
}

/* Call this routine for every sample. */
void Goertzel::process_sample(uint16_t sample)
{
  float Q0;
  Q0 = coeff * Q1 - Q2 + (float) sample;
  Q2 = Q1;
  Q1 = Q0;
}

/* Basic Goertzel */
/* Call this routine after every block to get the complex result. */
void Goertzel::get_real_imag(float *realPart, float *imagPart)
{
  *realPart = (Q1 - Q2 * cosine);
  *imagPart = (Q2 * sine);
}

/* Optimized Goertzel */
/* Call this after every block to get the RELATIVE magnitude squared. */
float Goertzel::get_magnitude_squared(void)
{
  float result;

  result = Q1 * Q1 + Q2 * Q2 - Q1 * Q2 * coeff;
  return result;
}
