/*
 * Chord.h
 *
 *  Created on: May 30, 2016
 *      Author: mayo
 */

#ifndef MUSICBOX_H_
#define MUSICBOX_H_

#include <stdlib.h>
#include <math.h>

typedef struct {
	int tones;
	float *freqs = NULL;
} CHORD;

class MusicBox {
public:
	MusicBox(int song);
	virtual ~MusicBox();
	void generate(int max_voices);
	int get_song_total_chords(char *base_notes);
	int* get_current_chord_LED();
	int get_current_melody_note();
	float get_current_melody_freq();
	static int get_song_total_melody_notes(char *melody);

	CHORD *chords = NULL;

#define MAX_VOICES_PER_CHORD 9 //only 8 needed for 16 filters by default
#define BASE_FREQS_PER_CHORD 3

	static const int expand_multipliers[MAX_VOICES_PER_CHORD][2];

	char *base_notes = NULL;
	float *bases_parsed = NULL;
	int *led_indicators = NULL;

	float *melody_freqs_parsed = NULL;
	int *melody_indicator_leds = NULL;

	char *use_song;
	char *use_melody = NULL;

	//for sequential playback (test)
	int total_chords; //number of chors in whole song (default 8)
	int current_chord; //the chord that currently plays
	int total_melody_notes; //number of notes in melody string
	int current_melody_note; //the note that currently plays

	static const char *SONGS[];
	char *XAOS_MELODY = NULL; //for dynamically created melody
};

#endif /* MUSICBOX_H_ */
