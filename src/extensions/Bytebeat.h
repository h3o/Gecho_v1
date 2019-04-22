/*
 * Bytebeat.h
 *
 *  Created on: 19 July 2018
 *      Author: mario (http://gechologic.com/contact)
 *
 *  Used bytebeat math formulas from: https://youtu.be/GtQdIYUtAHg
 *
 *  This file is part of the Gecho Loopsynth Firmware Development Framework.
 *  It can be used within the terms of CC-BY-NC-SA license.
 *
 *  Find more information at: http://gechologic.com/gechologists/
 */

#ifndef EXTENSIONS_BYTEBEAT_H_
#define EXTENSIONS_BYTEBEAT_H_

#include <stdlib.h>
#include <string.h>
#include "hw/gpio.h"
#include "hw/leds.h"
//#include "hw/sensors.h"
#include "hw/signals.h"
#include <Interface.h>

#ifdef __cplusplus
 extern "C" {
#endif

void bytebeat(int mode);

#ifdef __cplusplus
 }
#endif

#endif /* EXTENSIONS_BYTEBEAT_H_ */
