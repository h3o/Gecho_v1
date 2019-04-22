/*
 * Reverb.h
 *
 *  Created on: 17 July 2018
 *      Author: mayo
 *
 *  This file is part of the Gecho Loopsynth Firmware Development Framework.
 *  It can be used within the terms of CC-BY-NC-SA license.
 *
 *  Find more information at: http://gechologic.com/gechologists/
 */

#ifndef EXTENSIONS_REVERB_H_
#define EXTENSIONS_REVERB_H_

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

void reverb_with_echo(int mode);
int16_t add_reverb(int16_t sample);

#ifdef __cplusplus
 }
#endif

#endif /* EXTENSIONS_REVERB_H_ */
