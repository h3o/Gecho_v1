/*
 * notes.c
 *
 *  Created on: Oct 31, 2016
 *      Author: mayo
 *
 *  This file is part of the Gecho Loopsynth Firmware Development Framework.
 *  It can be used within the terms of CC-BY-NC-SA license.
 *  It must not be distributed separately.
 *
 *  Find more information at: http://gechologic.com/gechologists/
 */

#include "notes.h"
#include <hw/eeprom.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

int *set_chords_map = NULL;
int (*temp_song)[NOTES_PER_CHORD] = NULL;

const int halftone_lookup[8] = {
	0,	//a
	2,	//b
	3,	//c
	5,	//d
	7,	//e
	8,	//f
	10,	//g
	2	//h
};

const float notes_freqs[13] = {
	261.6, //c4
	277.2, //c#4
	293.7, //d4
	311.1, //d#4
	329.6, //e4
	349.2, //f4
	370.0, //f#4
	392.0, //g4
	415.3, //g#4
	440.0, //a4
	466.2, //a#4
	493.9, //b4
	523.3  //c5
};

const int key_numbers_to_note_codes[13] = { //not tested
		1,
		11,
		2,
		12,
		3,
		4,
		13,
		5,
		14,
		6,
		15,
		7,
		8
};

const int note_to_key[8] = {
	10,	//a
	12,	//b
	1,	//c
	3,	//d
	5,	//e
	6,	//f
	8,	//g
	12	//h
};

int encode_temp_song_to_notes(int (*song)[NOTES_PER_CHORD], int chords, char **dest_buffer)
{
	char *tmp_buffer = (char*)malloc(chords * 10); //allocate maximum possible memory for chords (if all had sharp symbol)
	int tmp_buf_ptr = 0;

	for(int i=0;i<chords;i++)
	{
		for(int n=0;n<NOTES_PER_CHORD;n++)
		{
			tmp_buf_ptr += tmp_song_find_note(song[i][n],tmp_buffer+tmp_buf_ptr);
		}
		tmp_buffer[tmp_buf_ptr] = ',';
		tmp_buf_ptr++;
	}

	if(dest_buffer[0] != NULL)
	{
		free(dest_buffer[0]);
	}
	dest_buffer[0] = (char*)malloc(tmp_buf_ptr);
    memcpy(dest_buffer[0],tmp_buffer,tmp_buf_ptr);
    free(tmp_buffer);
    dest_buffer[0][--tmp_buf_ptr] = 0; //remove last comma and terminate the string
	return tmp_buf_ptr;
}

int tmp_song_find_note(int note, char *buffer)
{
	int chars = 2;
	int octave = note / 100;// + 1;
	note %= 100;

	if(note==1 || note==2)
	{
		buffer[0] = 'c';
	}
	if(note==3 || note==4)
	{
		buffer[0] = 'd';
	}
	if(note==5)
	{
		buffer[0] = 'e';
	}
	if(note==6 || note==7)
	{
		buffer[0] = 'f';
	}
	if(note==8 || note==9)
	{
		buffer[0] = 'g';
	}
	if(note==10 || note==11)
	{
		buffer[0] = 'a';
	}
	if(note==12)
	{
		buffer[0] = 'b';
	}
	if(note==13)
	{
		buffer[0] = 'c';
		octave++;
	}

	if(note==2 || note==4 || note==7 || note==9 || note==11) //if sharp
	{
		buffer++;
		buffer[0] = '#';
		chars++;
	}
	buffer++;
	buffer[0] = '0' + octave;

	return chars;
}


int translate_intervals_to_notes(char *buffer, char chord_code, char *fragment)
{
	//base note will be 'A3'
	char key = tolower(chord_code);

	int dist0 = key - 'a'; //wrong!

	if(key=='a') { dist0 = 0; }
	if(key=='b') { dist0 = 2; }
	if(key=='c') { dist0 = 3; }
	if(key=='d') { dist0 = 5; }
	if(key=='e') { dist0 = 7; }
	if(key=='f') { dist0 = 8; }
	if(key=='g') { dist0 = 10; }
	if(key=='j') { dist0 = 4; }
	if(key=='k') { dist0 = 6; }
	if(key=='l') { dist0 = 9; }
	if(key=='m') { dist0 = 11; }
	if(key=='n') { dist0 = 1; }

	int dist;
	int buffer_ptr = 0;

	for(unsigned int i=0;i<strlen(fragment);i++)
	{
		if(fragment[i]=='.') //pause
		{
			buffer[buffer_ptr] = '.';
			buffer_ptr++;
		}
		else
		{
			dist = dist0 + fragment[i] - '0';
			buffer_ptr += interval_to_note(buffer+buffer_ptr,dist);
		}
	}
	return buffer_ptr;
}

const char *notes_by_distance[] = {"a3","a#3","b3","c4","c#4","d4","d#4","e4","f4","f#4","g4","g#4",
		                         "a4","a#4","b4","c5","c#5","d5","d#5","e5","f5","f#5","g5","g#5",
		                         "a5","a#5","b5","c6","c#6","d6","d#6","e6","f6","f#6","g6","g#6"};

int interval_to_note(char *buffer, int distance)
{
	char *note = (char *)notes_by_distance[distance+12]; //shift one octave up
	strcpy(buffer,note);
	return strlen(note);
}

void set_progression_str(char *chord_progression)
{
    if(progression_str != NULL)
    {
    	free(progression_str);
    	progression_str = NULL;
    }

    progression_str = (char*)malloc(strlen(chord_progression)+1); //plus one byte for string-terminating zero
	strcpy(progression_str, chord_progression);
	EEPROM_StoreSongAndMelody(progression_str, "CLEAR"); //store song and clear melody
}

void set_melody_str(char *melody)
{
    if(melody_str != NULL)
    {
    	free(melody_str);
    	melody_str = NULL;
    }

    melody_str = (char*)malloc(strlen(melody)+1); //plus one byte for string-terminating zero
	strcpy(melody_str, melody);
	EEPROM_StoreSongAndMelody(0, melody_str); //store melody but do not clear the song
}

//translate from note number (1=c,2=c#,3=d,...,11=a#,12=b) to LED number (0-7=whites,10-14=blues)
void notes_to_LEDs(int *notes, int *leds, int notes_per_chord)
{
	const int translate_table[14] = {-1,0,10,1,11,2,3,12,4,13,5,14,6,7};
	for(int i=0;i<notes_per_chord;i++)
	{
		leds[i] = translate_table[notes[i]%100];
	}
}

int parse_notes(char *base_notes_buf, float *bases_parsed_buf, int *led_indicators_buf) {

	int ptr,len=strlen(base_notes_buf),halftones,octave,/*parsed_chord=0,*/parsed_note=0,octave_shift,LED_indicator;
	double note_freq;
	double halftone_step_coef = HALFTONE_STEP_COEF;
	char note;

	for(ptr=0;ptr<len;ptr++)
	{
		base_notes_buf[ptr] = tolower(base_notes_buf[ptr]);
	}

	for(ptr=0;ptr<len;ptr++)
	{
		note = base_notes_buf[ptr];

		if(note == ' ') //skip spaces
		{
			//ptr++; //creates interesting effect by skipping some
			continue;
		}

		if(note >= 'a' && note <= 'h')
		{
			//get the note
			halftones = halftone_lookup[note - 'a'];

			if(base_notes_buf[ptr+1]=='#')
			{
				halftones++;
				ptr++;

				//Blue LEDs:
				if(note == 'c') LED_indicator = 10;
				else if(note == 'd') LED_indicator = 11;
				else if(note == 'f') LED_indicator = 12;
				else if(note == 'g') LED_indicator = 13;
				else if(note == 'a') LED_indicator = 14;
				else LED_indicator = -1; //no note
			}
			else
			{
				//White LEDs:
				LED_indicator = (6 + note - 'a') % 8; //c->0, a->6
			}
			ptr++;

			//get the octave
			if(base_notes_buf[ptr] >= '1' && base_notes_buf[ptr] <= '7')
			{
				octave = base_notes_buf[ptr] - '1' + 1;
			}

			if(base_notes_buf[ptr+1]==',')
			{
				ptr++;
			}

			//add halftones... 12 halftones is plus one octave -> frequency * 2
			note_freq = NOTE_FREQ_A4 * pow(halftone_step_coef, halftones);

			//shift according to the octave
			octave_shift = octave - 4; //ref note is A4 - fourth octave
			if(note>='c')
			{
				octave_shift--;
			}

			if(octave_shift > 0) //shift up
			{
				do
				{
					note_freq *= 2;
					octave_shift--;
				}
				while(octave_shift!=0);
			}
			else if(octave_shift < 0) //shift down
			{
				do
				{
					note_freq /= 2;
					octave_shift++;
				}
				while(octave_shift!=0);
			}

			bases_parsed_buf[/*3*parsed_chord + */parsed_note] = (float)note_freq;
			if(led_indicators_buf != NULL)
			{
				led_indicators_buf[parsed_note] = LED_indicator;
			}

			parsed_note++;
			/*
			if(parsed_note == BASE_FREQS_PER_CHORD)
			{
				parsed_note = 0;
				parsed_chord++;
			}
			*/
		}
		else if(note == '.') //no note, skip to next frame
		{
			bases_parsed_buf[parsed_note] = 0; //no note
			if(led_indicators_buf != NULL)
			{
				led_indicators_buf[parsed_note] = -1; //no LED
			}

			parsed_note++;
		}
	}
	return parsed_note;
}

char *read_next_note(char *str, int *return_buffer)
{
  int note;
  //301; //c3
  //305; //e3
  //308; //g3

  if(str[0]==',')
  {
	  str++;
  }

  if(str[0]>='a' && str[0]<='h')
  {
	  note = note_to_key[str[0]-'a'];
  }
  str++;
  if(str[0]=='#')
  {
	  note++;
	  str++;
  }
  if(str[0]>='1' && str[0]<='9')
  {
	  note += 100 * atoi(str);
	  str++;
  }
  return_buffer[0] = note;
  return str;
}

int spread_notes(float *notes, int length, int multiplier)
{
	if(multiplier > 0)
	{
		for(int i=length*multiplier-1;i>=0;i--)
		{
			if(i%multiplier==0)
			{
				notes[i] = notes[i/multiplier];
			}
			else
			{
				notes[i] = 0;
			}

		}
		return length * multiplier;
	}
	return 0;
}
