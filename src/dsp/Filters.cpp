/*
 * Filters.cpp
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

#include <Channels.h>
#include "Filters.h"
#include "freqs.h"
#include "songs.h"
#include "interface.h"

Filters::Filters(int selected_song)
{
	fp.resonance = 0.99;

	chord = new MusicBox(selected_song);

	if(selected_song>0)
	{
		int notes_parsed = parse_notes(chord->base_notes, chord->bases_parsed, chord->led_indicators);

		if(notes_parsed != chord->total_chords * NOTES_PER_CHORD) //integrity check
		{
			//we don't have correct amount of notes
			if(notes_parsed < chord->total_chords * NOTES_PER_CHORD) //some notes missing
			{
				do //let's add zeros, so it won't break the playback
				{
					chord->bases_parsed[notes_parsed++] = 0;
				}
				while(notes_parsed < chord->total_chords * NOTES_PER_CHORD);
			}
		}
		chord->generate(CHORD_MAX_VOICES);
	}

	fp.melody_filter_pair = -1; //none by default
	fp.arpeggiator_filter_pair = -1; //none by default
}

Filters::~Filters(void)
{
	delete(iir2);
	delete(chord);
}

void Filters::setup() //with no parameters will set up to default mode (Low-pass, 4th order)
{
	this->setup(DEFAULT_FILTERS_TYPE_AND_ORDER);
}

void Filters::setup(int filters_type_and_order)
{
	if(iir2!=NULL) //in case we are resetting all filters
	{
		free(iir2);
	}

	if(filters_type_and_order == FILTERS_TYPE_LOW_PASS + FILTERS_ORDER_4)
	{
		//iir2 = new IIR_Filter[FILTERS];
		iir2 = new IIR_Filter_LOW_PASS_4TH_ORDER[FILTERS];
	}
	else if(filters_type_and_order == FILTERS_TYPE_HIGH_PASS + FILTERS_ORDER_4)
	{
		//iir2 = new IIR_Filter[FILTERS];
		iir2 = new IIR_Filter_HIGH_PASS_4TH_ORDER[FILTERS];
	}

	/*
	int test = 0;
	test += iir2->BoundaryFn001();
	test += iir2->BoundaryFn002();
	*/

	for(int i=0;i<FILTERS;i++)
	{
		iir2[i].setResonance(fp.resonance);
		iir2[i].setCutoffAndLimits((float)freqs[i%(FILTERS/2)] * FILTERS_FREQ_CORRECTION / (float)I2S_AUDIOFREQ * 2 * 3);

		if(i == fp.melody_filter_pair || (fp.melody_filter_pair >= 0 && i == fp.melody_filter_pair + CHORD_MAX_VOICES))
		{
			fp.mixing_volumes[i] = MELODY_MIXING_VOLUME;
		}
		else
		{
			fp.mixing_volumes[i] = 1.0f;
		}
	}

	update_filters_loop = 0;
}

/*
void filters::start_update_filters(int f1, int f2, int f3, int f4, float freq1, float freq2, float freq3, float freq4)
{
	update_filters_f[0] = f1;
	update_filters_f[1] = f2;
	update_filters_f[2] = f3;
	update_filters_f[3] = f4;
	update_filters_freq[0] = freq1;
	update_filters_freq[1] = freq2;
	update_filters_freq[2] = freq3;
	update_filters_freq[3] = freq4;

	update_filters_loop = 4;
}
*/

void Filters::start_update_filters_pairs(int *filter_n, float *freq, int filters_to_update)
{
	for(int i=0;i<filters_to_update;i++)
	{
		update_filters_f[i] = filter_n[i];
		update_filters_freq[i] = freq[i];
		update_filters_f[i+filters_to_update] = filter_n[i] + FILTERS/2; //second filter of the pair starts at FILTERS/2 position
		update_filters_freq[i+filters_to_update] = freq[i];
	}
	update_filters_loop = filters_to_update * 2;
}

void Filters::start_next_chord()
{
	if(update_filters_loop!=0)
	{
		//problem - has not finished with previous update (possibly a melody)
		update_filters_loop = update_filters_loop;
	}
	for(int i=0;i<CHORD_MAX_VOICES;i++)
	{
		if(i == fp.melody_filter_pair)
		{
			//nothing to update, skip this voice
		}
		else
		{
			update_filters_f[update_filters_loop] = i;
			update_filters_freq[update_filters_loop] = chord->chords[chord->current_chord].freqs[i];
			update_filters_loop++;

			update_filters_f[update_filters_loop] = CHORD_MAX_VOICES + i;
			update_filters_freq[update_filters_loop] = chord->chords[chord->current_chord].freqs[i];
			update_filters_loop++;
		}
	}

	//update_filters_loop = CHORD_MAX_VOICES * 2;

	chord->current_chord++;
	if(chord->current_chord >= chord->total_chords)
	{
		chord->current_chord = 0;
	}
}

void Filters::start_next_melody_note()
{
	int i = fp.melody_filter_pair;

	int test_freq = 0;

	if(chord->melody_freqs_parsed[chord->current_melody_note] != 0)
	{
		update_filters_f[update_filters_loop] = i;
		update_filters_freq[update_filters_loop] = chord->melody_freqs_parsed[chord->current_melody_note];
		update_filters_loop++;

		update_filters_f[update_filters_loop] = CHORD_MAX_VOICES + i;
		update_filters_freq[update_filters_loop] = chord->melody_freqs_parsed[chord->current_melody_note];

		test_freq = update_filters_freq[update_filters_loop];
		update_filters_loop++;
	}

	chord->current_melody_note++;
	if(chord->current_melody_note >= chord->total_melody_notes)
	{
		chord->current_melody_note = 0;
	}

	test_freq += 0;
}

void Filters::progress_update_filters(Filters *fil, bool reset_buffers)
{
	if(!update_filters_loop)
	{
		return;
	}

	update_filters_loop--;
	int filter_n = update_filters_f[update_filters_loop];

	if(filter_n >= 0) //something to update
	{
		if(fp.melody_filter_pair >= 0 && (filter_n == fp.melody_filter_pair || filter_n == CHORD_MAX_VOICES + fp.melody_filter_pair)) //melody note
		{
			//fil->iir2[filter_n].setCutoffAndLimits_reso2(update_filters_freq[update_filters_loop] * FILTERS_FREQ_CORRECTION / (float)I2S_AUDIOFREQ * 2 * 3);
			fil->iir2[filter_n].setResonanceKeepFeedback(0.9998); //higher reso for melody voice
			fil->iir2[filter_n].setCutoffAndLimits(update_filters_freq[update_filters_loop] * FILTERS_FREQ_CORRECTION / (float)I2S_AUDIOFREQ * 2 * 3);
			fil->iir2[filter_n].disturbFilterBuffers();
		}
		else if(fp.arpeggiator_filter_pair >= 0 && (filter_n == fp.arpeggiator_filter_pair || filter_n == CHORD_MAX_VOICES + fp.arpeggiator_filter_pair)) //arpeggiator
		{
			fil->iir2[filter_n].setResonanceKeepFeedback(0.9998); //higher reso for arpeggiated voice
			fil->iir2[filter_n].setCutoffAndLimits(update_filters_freq[update_filters_loop] * FILTERS_FREQ_CORRECTION / (float)I2S_AUDIOFREQ * 2 * 3);
			fil->iir2[filter_n].disturbFilterBuffers();
			//fil->iir2[filter_n].resetFilterBuffers(); //causes more ticking
			//fil->iir2[filter_n].rampFilter(); //mask the initial ticking
		}
		else //background chord
		{
			fil->iir2[filter_n].setCutoffAndLimits(update_filters_freq[update_filters_loop] * FILTERS_FREQ_CORRECTION / (float)I2S_AUDIOFREQ * 2 * 3);
		}
	}

	/*
	if(reset_buffers && update_filters_loop == 0)
	{
		for(int f=0;f<CHORD_MAX_VOICES*2;f++)
		{
			fil->iir2[update_filters_f[f]].resetFilterBuffers();
		}
	}
	*/
}

void Filters::start_nth_chord(int chord_n)
{
	chord_n = chord_n % chord->total_chords; //wrap around, to be extra safe

	for(int i=0;i<CHORD_MAX_VOICES;i++)
	{
		update_filters_f[i] = i;
		update_filters_freq[i] = chord->chords[chord_n].freqs[i];

		update_filters_f[CHORD_MAX_VOICES + i] = CHORD_MAX_VOICES + i;
		update_filters_freq[CHORD_MAX_VOICES + i] = chord->chords[chord_n].freqs[i];
	}

	update_filters_loop = CHORD_MAX_VOICES * 2;
}

void Filters::set_melody_voice(int filter_pair, int melody_id)
{
	fp.melody_filter_pair = filter_pair;

	if(melody_id == 112 && melody_str != NULL)
	{
		chord->use_melody = melody_str;
	}
	else
	{
		chord->use_melody = (char*)MusicBox_SONGS[melody_id * 2 - 1];
	}

	if(chord->use_melody != NULL)
	{
		chord->total_melody_notes = chord->get_song_total_melody_notes(chord->use_melody);

		chord->melody_freqs_parsed = (float*)malloc(chord->total_melody_notes * sizeof(float));
		chord->melody_indicator_leds = (int*)malloc(chord->total_melody_notes * sizeof(int));
		parse_notes(chord->use_melody, chord->melody_freqs_parsed, chord->melody_indicator_leds);
	}
}

void Filters::add_to_update_filters_pairs(int filter_n, float freq)
{
	if(update_filters_loop>0)
	{
		//some filters haven't finished updating
		//update_filters_loop = 0;
	}

	update_filters_f[update_filters_loop] = filter_n;
	update_filters_freq[update_filters_loop] = freq;
	update_filters_loop++;

	update_filters_f[update_filters_loop] = filter_n + FILTERS/2; //second filter of the pair starts at FILTERS/2 position
	update_filters_freq[update_filters_loop] = freq;
	update_filters_loop++;
}
