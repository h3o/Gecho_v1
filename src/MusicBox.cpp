/*
 * Chord.cpp
 *
 *  Created on: May 30, 2016
 *      Author: mayo
 */

#include "MusicBox.h"
#include <Channels.h>
#include <Interface.h>
#include "notes.h"
#include "songs.h"
#include <ctype.h>

const int MusicBox::expand_multipliers[MAX_VOICES_PER_CHORD][2] = {
	/*
	//v1. - more trebles, less basses
	{0, 1},		//source 0, no change
	{1, 1},		//source 1, no change
	{2, 1}, 	//source 2, no change
	{0, -2},	//source 0, freq / 2
	{0, 2},		//source 0, freq * 2
	{1, 2},		//source 1, freq * 2
	{2, 2},		//source 2, freq * 2
	{1, -2},	//source 1, freq / 2
	{2, -2}		//source 2, freq / 2
	*/

	//v2. - less trebles, more basses
	{0, 1},		//source 0, no change
	{1, 1},		//source 1, no change
	{2, 1}, 	//source 2, no change
	{0, 2},		//source 0, freq * 2 //only powers of 2 make sense to shift octaves up and down (2,4,8)
	{0, -2},	//source 0, freq / 2
	{0, 4},		//source 0, freq * 4
	{0, -4},	//source 0, freq / 4
	{1, 2},		//source 1, freq * 2
	{2, 2}		//source 2, freq * 2 //no need if only 8 voices per chord
};

MusicBox::MusicBox(int song) {
	// TODO Auto-generated constructor stub

	if(song>0 && song!=CHANNEL_111_RECENTLY_GENERATED_SONG && song!=CHANNEL_112_RECENTLY_GENERATED_SONG_W_MELODY)
	{
		use_melody = NULL;
		use_song = (char*)MusicBox_SONGS[(song-1) * 2];
	}
	else //if(song==CHANNEL_111_RECENTLY_GENERATED_SONG)
	{
		//translate to required notation
		//total_chords = temp_total_chords;
		/*int encoded_length =*/ //encode_temp_song_to_notes(temp_song, total_chords, &use_song); //memory will be allocated for use_song
	//}
	//else if(song==CHANNEL_112_RECENTLY_GENERATED_SONG_W_MELODY)
	//{
		//the song is prepared in progression_str variable
		use_song = progression_str;
		if(melody_str && strlen(melody_str))//song==CHANNEL_112_RECENTLY_GENERATED_SONG_W_MELODY)
		{
			use_melody = melody_str;
		}
	}

	base_notes = (char*)malloc(strlen(use_song) * sizeof(char) + 1);
	memcpy(base_notes, use_song, strlen(use_song) * sizeof(char));
	base_notes[strlen(use_song) * sizeof(char)] = 0;

	if(use_song && (song==CHANNEL_111_RECENTLY_GENERATED_SONG || song==CHANNEL_112_RECENTLY_GENERATED_SONG_W_MELODY))
	{
		free(use_song); //free memory that was allocated by encode_temp_song_to_notes()
	}

	total_chords = get_song_total_chords(base_notes);

	bases_parsed = (float*)malloc(total_chords * BASE_FREQS_PER_CHORD * sizeof(float)); //default = 8 chords, 3 base freqs for each chord
	led_indicators = (int*)malloc(total_chords * 3 * sizeof(int)); //default = 8 chords, 3 LED lights per chord

	chords = (CHORD*) malloc(total_chords * sizeof(CHORD));

	current_chord = 0;
	current_melody_note = 0;

	melody_freqs_parsed = NULL;
	melody_indicator_leds = NULL;
}

MusicBox::~MusicBox() {
	// TODO Auto-generated destructor stub

	if(chords != NULL) {

		//todo: first release all freqs arrays within chord structure: chords[c].freqs
		for(int c=0;c<total_chords;c++)
		{
			if(chords[c].freqs != NULL)
			{
				free(chords[c].freqs);
				chords[c].freqs = NULL;
			}
		}

		free(chords); chords = NULL;
	}

	if(melody_freqs_parsed != NULL) { free(melody_freqs_parsed); melody_freqs_parsed = NULL; }
	if(melody_indicator_leds != NULL) { free(melody_indicator_leds); melody_indicator_leds = NULL; }
	if(base_notes != NULL) { free(base_notes); base_notes = NULL; }
	if(bases_parsed != NULL) { free(bases_parsed); bases_parsed = NULL; }
	if(led_indicators != NULL) { free(led_indicators); led_indicators = NULL; }
}

void MusicBox::generate(int max_voices) {

	int c,t;
	//float test;
	for(c=0;c<total_chords;c++)
	{
		chords[c].tones = max_voices;
		chords[c].freqs = (float *) malloc(max_voices * sizeof(float));

		for(t=0;t<chords[c].tones;t++)
		{
			if(expand_multipliers[t][1] > 0)
			{
				chords[c].freqs[t] = bases_parsed[3*c + expand_multipliers[t][0]] * (float)expand_multipliers[t][1];
				//test = chords[c].freqs[t];
				//test++;

				//if(chords[c].freqs[t] > 2000.0f)
				//{
				//	chords[c].freqs[t] /= 2.0f;
				//}
			}
			else
			{
				chords[c].freqs[t] = bases_parsed[3*c + expand_multipliers[t][0]] / (float)(-expand_multipliers[t][1]);
				//test = chords[c].freqs[t];
				//test++;
			}
		}
	}
}

 int MusicBox::get_song_total_chords(char *base_notes)
{
	if(strlen(base_notes)==0)
	{
		return 0;
	}

	int chords = 1;

	for(unsigned int i=0;i<strlen(base_notes);i++)
	{
	    if(base_notes[i]==',')
	    {
	    	chords++;
	    }
	}
	return chords;
}

int* MusicBox::get_current_chord_LED()
{
	return led_indicators + 3 * current_chord;
}

int MusicBox::get_current_melody_note()
{
	return melody_indicator_leds[current_melody_note];
}

float MusicBox::get_current_melody_freq()
{
	return melody_freqs_parsed[current_melody_note];
}


int MusicBox::get_song_total_melody_notes(char *melody)
{
	if(strlen(melody)==0)
	{
		return 0;
	}

	int notes = 0;

	for(unsigned int i=0;i<strlen(melody);i++)
	{
	    if(melody[i]=='.' || (melody[i]>='a' && melody[i]<='h'))
	    {
	    	notes++;
	    }
	}
	return notes;
}

/*
void Chord::clear_old_song_and_melody()
{
	if(use_song!=NULL)
	{
		free(use_song); //free memory that was allocated by encode_temp_song_to_notes()
		use_song = NULL;
	}
	if(use_melody!=NULL)
	{
		free(use_melody);
		use_melody = NULL;
	}
}
*/
