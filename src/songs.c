/*
 * songs.c
 *
 *  Created on: May 30, 2016
 *      Author: mayo
 *
 *  This file is part of the Gecho Loopsynth Firmware Development Framework.
 *  It can be used within the terms of CC-BY-NC-SA license.
 *  It must not be distributed separately.
 *
 *  Find more information at: http://gechologic.com/gechologists/
 */

#include "songs.h"
#include <hw/signals.h>
//#include <hw/eeprom.h>
//#include <string.h>
//#include <stdlib.h>
//#include <ctype.h>
//#include <math.h>

//returns selected song and melody encoded as one int value (song + 1000 * melody)
int channel_to_song_and_melody(int channel)
{
	int selected_song = 0, selected_melody = 0;

	//look up if channel overridden in FLASH user-content sector


	//if not found in FLASH user-content sector, fall back to hard-coded mapping
	if (channel <= 4)
	{
		selected_song = channel;
		selected_melody = 0;

		if (selected_song == 1) //song #1 with melody and hi-pass filters
		{
			selected_song = 1;
			selected_melody = 1;
		}
		if (selected_song == 2) //song #2 with melody (epic ad)
		{
			selected_song = 2;
			selected_melody = 2;
		}
		if (selected_song == 3) //pick random one out of basic songs and play it with low-pass filters
		{
			new_random_value();

			if(random_value % 5 == 0)
			{
				selected_song = 11;
			}
			else if(random_value % 5 == 1)
			{
				selected_song = 12;
			}
			else if(random_value % 5 == 2)
			{
				selected_song = 13;
			}
			else if(random_value % 5 == 3)
			{
				selected_song = 22;
			}
			else if(random_value % 5 == 4)
			{
				selected_song = 23;
			}
		}
		if (selected_song == 4) //override - song #21 (GITS)
		{
			selected_song = 21;
		}
	}
	if (channel >= 4111 && channel <= 4144)
	{
		selected_song = channel % 100; //songs 11-44
		selected_melody = 0;

		if(selected_song == 14 || selected_song == 21)
		{
			selected_melody = selected_song;
		}
	}
	/*
	if(channel==111 || channel==112)
	{
		selected_song = 111;
		selected_melody = 112;
	}
	*/
	return selected_song + 1000 * selected_melody;
}
