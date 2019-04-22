/*
 * Filters.h
 *
 *  Created on: Apr 27, 2016
 *      Author: mayo
 *
 *  This file is part of the Gecho Loopsynth Firmware Development Framework.
 *  It can be used within the terms of CC-BY-NC-SA license.
 *  It must not be distributed separately.
 *
 *  Find more information at: http://gechologic.com/gechologists/
 */

#ifndef FILTERS_H_
#define FILTERS_H_

#include "IIR_filters.h"
#include <hw/codec.h>
#include <hw/controls.h>
#include <MusicBox.h>

#define FILTERS (FILTER_PAIRS*2)
#define ACCORD MINOR

#define CHORD_MAX_VOICES (FILTERS/2)	//two channels per voice
#define FILTERS_FREQ_CORRECTION 1.035
#define MELODY_MIXING_VOLUME 2.5f

typedef struct {

	float volume_coef,volume_f,resonance;
	float mixing_volumes[FILTERS];

	int melody_filter_pair; //between 0 and CHORD_MAX_VOICES
	int arpeggiator_filter_pair; //between 0 and CHORD_MAX_VOICES

} FILTERS_PARAMS;

class Filters
{
	public:

		IIR_Filter *iir2 = NULL;
		FILTERS_PARAMS fp;
		MusicBox *chord;

		Filters(int selected_song);
		~Filters(void);

		void setup();
		void setup(int filters_type_and_order);

		//void start_update_filters(int f1, int f2, int f3, int f4, float freq1, float freq2, float freq3, float freq4);
		void start_update_filters_pairs(int *filter_n, float *freq, int filters_to_update);
		void start_next_chord();
		void start_next_melody_note();
		void progress_update_filters(Filters *fil, bool reset_buffers);
		void start_nth_chord(int chord_n);
		void set_melody_voice(int filter_pair, int melody_id);
		void add_to_update_filters_pairs(int filter_n, float freq);

	private:

		int update_filters_f[CHORD_MAX_VOICES*2 + 2]; //also two spaces for updating melody
		float update_filters_freq[CHORD_MAX_VOICES*2 + 2]; //also two spaces for updating melody
		int update_filters_loop;
};

#endif /* FILTERS_H_ */
