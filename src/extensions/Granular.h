/*
 * Granular.h
 *
 *  Created on: 22 Jan 2018
 *      Author: mario (http://gechologic.com/contact)
 *
 *  As explained in the coding tutorial: http://gechologic.com/granular-sampler
 *
 *  This file is part of the Gecho Loopsynth Firmware Development Framework.
 *  It can be used within the terms of CC-BY-NC-SA license.
 *
 *  Find more information at: http://gechologic.com/gechologists/
 */

#ifndef EXTENSIONS_GRANULAR_H_
#define EXTENSIONS_GRANULAR_H_

#include <stdlib.h>
#include <string.h>
#include "hw/gpio.h"
#include "hw/leds.h"
#include "hw/sensors.h"
#include "hw/signals.h"
#include <Interface.h>

#ifdef __cplusplus
 extern "C" {
#endif

void granular_sampler(int mode);
int16_t add_echo(int16_t sample);
void update_grain_freqs(float *freq, float *bases, int voices_used, int part); //part: 0=all

#ifdef __cplusplus
 }
#endif

#endif /* EXTENSIONS_GRANULAR_H_ */
