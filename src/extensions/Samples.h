/*
 * Samples.h
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

#ifndef SAMPLES_H_
#define SAMPLES_H_

#include <hw/stm32f4xx_it.h> //for uint16_t type

#ifdef __cplusplus
 extern "C" {
#endif

uint16_t flash_sample_process(float playback_note_frequency);
void play_buffer(uint16_t *buffer, int buffer_length, int channels);
//void mix_in_buffer(uint16_t *buffer, int buffer_length, int channels, int volume_sample_rshift, unsigned int mask);

#ifdef __cplusplus
}
#endif

#endif /* SAMPLES_H_ */
