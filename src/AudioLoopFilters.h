/*
 * AudioLoopFilters.h
 *
 *  Created on: Apr 10, 2016
 *      Author: mayo
 *
 *  This file is part of the Gecho Loopsynth Firmware Development Framework.
 *  It can be used within the terms of CC-BY-NC-SA license.
 *  It must not be distributed separately.
 *
 *  Find more information at: http://gechologic.com/gechologists/
 */

#ifndef AUDIO_LOOP_FILTERS_H_
#define AUDIO_LOOP_FILTERS_H_

//time-critical stuff
#define SHIFT_MELODY_NOTE_TIMING_BY_SAMPLE 3
#define SHIFT_CHORD_TIMING_BY_SAMPLE 5
#define SHIFT_ARP_TIMING_BY_SAMPLE 2

void audio_loop_filters();

#endif



