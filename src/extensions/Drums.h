/*
 * Drums.h
 *
 *  Created on: Nov 26, 2016
 *      Author: mayo
 *
 *  This file is part of the Gecho Loopsynth Firmware Development Framework.
 *  It can be used within the terms of CC-BY-NC-SA license.
 *  It must not be distributed separately.
 *
 *  Find more information at: http://gechologic.com/gechologists/
 */

#ifndef DRUMS_H_
#define DRUMS_H_

#define DRUM_BASE_ADDR1 0x080D2420		//base address where we uploaded samples
#define DRUM_LENGTH1 17416				//length of first sample in bytes
#define DRUM_BASE_ADDR2 (DRUM_BASE_ADDR1 + DRUM_LENGTH1)
#define DRUM_LENGTH2 39104
#define DRUM_BASE_ADDR3 (DRUM_BASE_ADDR2 + DRUM_LENGTH2)
#define DRUM_LENGTH3 12416
#define DRUM_BASE_ADDR4 (DRUM_BASE_ADDR3 + DRUM_LENGTH3)
#define DRUM_LENGTH4 68310

#define DRUM_SAMPLES 4						//we uploaded 4 samples to FLASH memory
#define DRUM_CHANNELS_MAX 4 				//no real polyphony for now, just one of each samples at the time

#define DRUM_SENSOR_THRESHOLD(x) ((x==0||x==3)?600:300)	//how far from the sensor to flip trigger, inner sensors more sensitive
#define DRUM_SAMPLE_VOLUME 0.19f	//general adjustment volume for samples we used

extern int drum_samples_ptr[DRUM_CHANNELS_MAX]; 	//pointers for currently playing sample
extern int drum_trigger[DRUM_CHANNELS_MAX];		//triggers for execution of each drum
extern int drum_lengths[DRUM_SAMPLES];				//lengths of samples in bytes
extern int drum_bases[DRUM_SAMPLES];				//base addresses of samples


#include <hw/stm32f4xx_it.h> //for uint16_t type

#ifdef __cplusplus
 extern "C" {
#endif

uint16_t drum_kit_process();
uint16_t drum_kit_process_cv_trigger();

#ifdef __cplusplus
}
#endif

#endif /* DRUMS_H_ */
