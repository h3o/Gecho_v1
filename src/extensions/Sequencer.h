/*
 * Sequencer.h
 *
 *  Created on: 26 Jan 2018
 *      Author: mayo
 *
 *  As explained in the coding tutorial: http://gechologic.com/granular-sampler
 *
 *  This file is part of the Gecho Loopsynth Firmware Development Framework.
 *  It can be used within the terms of CC-BY-NC-SA license.
 *
 *  Find more information at: http://gechologic.com/gechologists/
 */

#ifndef EXTENSIONS_SEQUENCER_H_
#define EXTENSIONS_SEQUENCER_H_

#include <string.h>
#include "hw/gpio.h"
#include "hw/leds.h"
#include "hw/sensors.h"
#include <Interface.h>

#ifdef __cplusplus
 extern "C" {
#endif

void drum_sequencer();

#ifdef __cplusplus
 }
#endif

#endif /* EXTENSIONS_SEQUENCER_H_ */
