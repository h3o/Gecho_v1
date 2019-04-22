/*
 * flash.c
 *
 *  Created on: Oct 7, 2017
 *      Author: mayo
 */

#include "flash.h"
#include "songs.h"
//#include "gpio.h"
//#include "leds.h"
//#include "stm32f4xx_pwr.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

char **overridable_memory_map = NULL;

void FLASH_save_to_overridable_channel(char *command)
{
	int channel;
	char *cmd_ptr = command;

	channel = atoi(command);
	while(cmd_ptr[0]!='&')
	{
		cmd_ptr++;
	}
	if(channel>0 && FLASH_channel_to_position(channel)>=0) //if valid and overridable channel
	{
		FLASH_load_memory_map();
		if(overridable_memory_map[FLASH_channel_to_position(channel)] != NULL)
		{
			free(overridable_memory_map[FLASH_channel_to_position(channel)]);
		}
		cmd_ptr++;
		overridable_memory_map[FLASH_channel_to_position(channel)] = (char*)malloc(strlen(cmd_ptr));
		strcpy(overridable_memory_map[FLASH_channel_to_position(channel)], cmd_ptr);
		FLASH_save_memory_map();
		FLASH_delete_memory_map();
	}
}

char *FLASH_load_from_overridable_channel(char *command)
{
	int channel = atoi(command);
	char *score = NULL;

	FLASH_load_memory_map();
	if((FLASH_channel_to_position(channel)>=0) && (overridable_memory_map[FLASH_channel_to_position(channel)] != NULL))
	{
		score = (char*)malloc(strlen(overridable_memory_map[FLASH_channel_to_position(channel)]));
		strcpy(score,overridable_memory_map[FLASH_channel_to_position(channel)]);
	}
	else
	{
		//load from pre-programmed songs
		int song_melody = channel_to_song_and_melody(channel);
		int song = song_melody % 1000;
		int melody = song_melody / 1000;
		char *song_str = (char*)MusicBox_SONGS[(song-1) * 2];
		char *melody_str = (char*)MusicBox_SONGS[(melody-1) * 2 + 1];

		score = (char*)malloc(strlen(song_str) + strlen(melody_str) + 30);
		score[0]=0;
		strcpy(score,"SONG=");
		if(song>0)
		{
			strcat(score,song_str);
		}
		strcat(score,"&MELODY=");
		if(melody>0)
		{
			strcat(score,melody_str);
		}
		strcat(score,";\n");
	}
	FLASH_delete_memory_map();
	return score;
}

int FLASH_channel_to_position(int channel)
{
	for(int i=0;i<OVERRIDABLE_CHANNELS;i++)
	{
		if(overridable_channels[i]==channel)
		{
			return i;
		}
	}
	return -1;
}

int FLASH_position_to_channel(int position)
{
	if(position<OVERRIDABLE_CHANNELS)
	{
		return overridable_channels[position];
	}
	return 0;
}

int FLASH_all_empty()
{
	int data_count = 0;
	for(char *ptr = (char*)SECTOR_BASE; ptr<(char*)(SECTOR_BASE + SECTOR_LENGTH); ptr++)
	{
		if(ptr[0]!=0xff)
		{
			data_count++;
		}
	}
	if(data_count>0)
	{
		return 0;
	}
	return 1;
}

void FLASH_load_memory_map()
{
	if(overridable_memory_map!=NULL)
	{
		FLASH_delete_memory_map();
	}
	overridable_memory_map = (char**)malloc(OVERRIDABLE_CHANNELS * sizeof(char*));
	for(int i=0;i<OVERRIDABLE_CHANNELS;i++)
	{
		overridable_memory_map[i] = NULL;
	}

	//parse content of the sector and load found channels into memory
	char *f_ptr = (char*)SECTOR_BASE, *f_end_chan_data;
	int channel, map_position;

	while((f_ptr = strstr(f_ptr,"CHAN=")))
	{
		f_ptr += 5;
		channel = atoi(f_ptr);
		f_ptr = strstr(f_ptr,"&SONG="); //find begin channel data delimiter
		if(f_ptr)
		{
			f_ptr ++;
			f_end_chan_data = strstr(f_ptr,";"); //find end channel data delimiter

			//add data to memory map
			map_position = FLASH_channel_to_position(channel);
			overridable_memory_map[map_position] = malloc((int)f_end_chan_data - (int)f_ptr + 2);
			strncpy(overridable_memory_map[map_position], f_ptr, (int)f_end_chan_data - (int)f_ptr);
			overridable_memory_map[map_position][(int)f_end_chan_data - (int)f_ptr] = 0;
		}
	}
}

void FLASH_save_memory_map()
{
	int flash_status, i;

	//render map to one large string
	char *user_content = (char*)malloc(FLASH_CONTENT_MAP_SIZE);
	char buf[10];
	if(user_content==NULL)
	{
		return;
	}

	//collect available channels for faster access
	strcpy(user_content,"CHANNELS=");
	for(i=0;i<OVERRIDABLE_CHANNELS;i++)
	{
		if(overridable_memory_map[i] != NULL) //custom data present for this channel
		{
			sprintf(buf,"%d,",FLASH_position_to_channel(i));
			strcat(user_content,buf);
		}
	}
	if(user_content[strlen(user_content)-1]==',')
	{
		user_content[strlen(user_content)-1] = ';';
	}
	else
	{
		strcat(user_content,"none;");
	}

	for(i=0;i<OVERRIDABLE_CHANNELS;i++)
	{
		if(overridable_memory_map[i] != NULL) //custom data present for this channel
		{
			sprintf(buf,"CHAN=%d&",FLASH_position_to_channel(i));
			strcat(user_content,buf);
			strcat(user_content,overridable_memory_map[i]);
			if(user_content[strlen(user_content)-1]!=';')
			{
				strcat(user_content,";");
			}
		}
	}


	FLASH_Unlock();
	FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);
	flash_status = FLASH_EraseSector(SECTOR_ID, FLASH_VOLTAGE_RANGE);

	if (flash_status != FLASH_COMPLETE) {
		FLASH_Lock();
	    return;
	}

	uint32_t write_address = SECTOR_BASE;
	uint32_t write_data;

	//write string as sequence of words
	for(int i=0; i<strlen(user_content)/4+2; i++) //+2 to make sure the string ending \0 byte is written
	{
		write_data = (uint32_t)(((uint32_t*)(user_content+4*i))[0]);
		flash_status = FLASH_ProgramWord(write_address, write_data);
		write_address += 4;

		if (flash_status != FLASH_COMPLETE) {
			FLASH_Lock();
			return;
		}
	}
	FLASH_ProgramWord(write_address, 0x00000000);

	free(user_content);
}

void FLASH_delete_memory_map()
{
	if(overridable_memory_map==NULL)
	{
		return;
	}
	for(int i=0;i<OVERRIDABLE_CHANNELS;i++)
	{
		if(overridable_memory_map[i]!=NULL)
		{
			free(overridable_memory_map[i]);
			//overridable_memory_map[i]=NULL;
		}
	}
	free(overridable_memory_map);
	overridable_memory_map = NULL;
}

void FLASH_LoadChannelMap(char **return_buffer)
{
	return_buffer[0] = NULL;

	//parse content of the sector and load found channels into memory
	char *f_ptr = (char*)SECTOR_BASE;
	char *end = strstr(f_ptr,";");
	if(!end)
	{
		return; //no channels stored
	}
	int size = (int)end - (int)f_ptr;
	return_buffer[0] = malloc(size+2);
	strncpy(return_buffer[0], f_ptr, size);
	return_buffer[0][size] = '$';
	return_buffer[0][size+1] = 0;
}

void FLASH_LoadOverriddenChannel(int channel, char **song, char **melody, char **settings)
{
	//parse content of the sector and load the desired channel into variables
	char *f_ptr = (char*)SECTOR_BASE;
	char delimiter[30];
	sprintf(delimiter,"CHAN=%d&SONG=",channel);
	char *channel_data = strstr(f_ptr,delimiter);
	char *end;
	int size;
	if(channel_data) //song data found
	{
		end = strstr(channel_data,"&MELODY"); //song data ends here
		size = (int)end - (int)channel_data - strlen(delimiter);
		song[0] = malloc(size+2);
		strncpy(song[0],channel_data+strlen(delimiter),size);
		song[0][size] = 0;
	}

	sprintf(delimiter,"&MELODY=");
	channel_data = strstr(channel_data,delimiter);
	if(channel_data) //melody data found
	{
		end = strstr(channel_data+1,"&"); //melody data ends here if settings present
		if(!end) //if no settings, look for the other delimiter
		{
			end = strstr(channel_data+1,";"); //melody data ends here
		}
		size = (int)end - (int)channel_data - strlen(delimiter);
		if(size)
		{
		  melody[0] = malloc(size+2);
		  strncpy(melody[0],channel_data+strlen(delimiter),size);
		  melody[0][size] = 0;
		}

		if(end[0]=='&' && !strncmp(end,"&SET=",5))
		{
			//settings present
			settings[0] = malloc(strlen(end+5)+2);
			strcpy(settings[0],end+5);
		}
	}
}

int FLASH_IsOverriddenChannel(int channel)
{
	char *map;
	FLASH_LoadChannelMap(&map);
	char chan[20];
	int found = 0;
	sprintf(chan,"=%d,",channel);
	if(strstr(map,chan))
	{
		found++;
	}
	sprintf(chan,",%d,",channel);
	if(strstr(map,chan))
	{
		found++;
	}
	sprintf(chan,",%d$",channel);
	if(strstr(map,chan))
	{
		found++;
	}
	sprintf(chan,"=%d$",channel);
	if(strstr(map,chan))
	{
		found++;
	}
	free(map);
	return found;
}

void FLASH_erase_all_custom_data()
{
	FLASH_Unlock();
	FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);
	int flash_status = FLASH_EraseSector(SECTOR_ID, FLASH_VOLTAGE_RANGE);

	if (flash_status != FLASH_COMPLETE) {
		FLASH_Lock();
	    return;
	}
}

void FLASH_set_activation_lock()
{
	int flash_status;

	FLASH_erase_all_custom_data();

	for(int i=0; i<4; i++)
	{
		//flash_status = FLASH_ProgramDoubleWord(ACTIVATION_LOCK_ADDRESS+i*10000, ACTIVATION_LOCK_DWORD);
		//flash_status = FLASH_ProgramWord(ACTIVATION_LOCK_ADDRESS+i*8/*1024*16*/, (uint32_t)(ACTIVATION_LOCK_DWORD));

	//FLASH_Unlock();
	//FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);
		//flash_status = FLASH_ProgramWord(ACTIVATION_LOCK_ADDRESS, 0xdead);
		flash_status = FLASH_ProgramWord(ACTIVATION_LOCK_ADDRESS+i*1024*16, (uint32_t)(ACTIVATION_LOCK_DWORD));

		if (flash_status != FLASH_COMPLETE) {
			FLASH_Lock();
			return;
		}

		flash_status = FLASH_ProgramWord(ACTIVATION_LOCK_ADDRESS+i*1024*16+4, (uint32_t)(ACTIVATION_LOCK_DWORD>>32));

		if (flash_status != FLASH_COMPLETE) {
			FLASH_Lock();
			return;
		}
	}
	FLASH_Lock();
}

int FLASH_check_activation_lock()
{
	uint64_t dw;

	for(int i=0; i<4; i++)
	{
		dw = ((int64_t*)(ACTIVATION_LOCK_ADDRESS+i*1024*16))[0];
		if(dw==ACTIVATION_LOCK_DWORD)
		{
			return 1;
		}
	}
	return 0;
}
