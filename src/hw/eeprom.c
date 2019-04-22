/*
 * eeprom.c
 *
 *  Created on: Jul 14, 2016
 *      Author: mayo
 */

#include "eeprom.h"
#include "gpio.h"
#include "leds.h"
#include "stm32f4xx_pwr.h"
#include <string.h>
#include <stdlib.h>

void EEPROM_Init()
{
	//Enable the PWR clock
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
	//Enable access to the backup domain
	PWR_BackupAccessCmd(ENABLE);
	//Enable backup SRAM Clock
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_BKPSRAM, ENABLE);
	//Enable the Backup SRAM low power Regulator to retain it's content in VBAT mode
	PWR_BackupRegulatorCmd(ENABLE);

	// Wait until the Backup SRAM low power Regulator is ready
	while(PWR_GetFlagStatus(PWR_FLAG_BRR) == RESET) {};
}

void EEPROM_Test()
{
	//Enable access to the backup domain
	PWR_BackupAccessCmd(ENABLE);

	//and you can write/read datas to sram (these codes from BKP_Domain codes
	//in STM32F4xx_DSP_StdPeriph_Lib) (in my mcu stm32f417 BKPSRAM_BASE=0x40024000)

	// Write to Backup SRAM with 32-Bit Data
	int i, errorCount = 0;

	for (i = BKPSRAM_TEST_BASE; i < BKPSRAM_TEST_BASE + BKPSRAM_TEST_LENGTH; i += 4) {
		*(__IO uint32_t *) (i) = i;
	}

	// Check the written Data
	for (i = BKPSRAM_TEST_BASE; i < BKPSRAM_TEST_BASE + BKPSRAM_TEST_LENGTH; i += 4) {
		if ((*(__IO uint32_t *) (i)) != i){
			errorCount++;
		}
	}

	if(errorCount)
	{
		//indicate Backup SRAM memory problem with blinking Orange LEDs
		for(i=0;i<16;i++)
		{
			LED_O4_set_byte(0x0f);
			Delay(32);
			LED_O4_all_OFF();
			Delay(32);
		}
	}

	//check if settings space initialized
	if(!strncmp((char*)SETTINGS_BASE,"SETTINGS",8)) //check if the string stored there
	{
		//all ok
	}
	else //initialize settings space with all zeroes
	{
		memset((char*)SETTINGS_BASE,0,SETTINGS_RESERVED_LENGTH);
		strcpy((char*)SETTINGS_BASE,"SETTINGS");
	}

	//Disable access to the backup domain
	PWR_BackupAccessCmd(DISABLE);
}

void EEPROM_LoadSongAndMelody(char **song_buffer, char **melody_buffer)
{
	//Enable access to the backup domain
	//PWR_BackupAccessCmd(ENABLE);
	EEPROM_Init();

	//char test[100];
	//strncpy(test,SONG_STORED_ID,100);
	//strncpy(test,MELODY_STORED_ID,100);
	//test[0] = test[0];

	if(!strncmp(SONG_STORED_ID,"SONG_STORED",11)) //check if someting really stored there
	{
		if(song_buffer[0]!=0)
		{
			free(song_buffer[0]);
		}
		song_buffer[0] = (char*)malloc(strlen(SONG_STORED_DATA));
		strcpy(song_buffer[0],SONG_STORED_DATA);
	}
	else
	{
		song_buffer[0] = NULL;
	}
	if(!strncmp(MELODY_STORED_ID,"MELODY_STORED",13)) //check if someting really stored there
	{
		if(melody_buffer[0]!=0)
		{
			free(melody_buffer[0]);
		}
		melody_buffer[0] = (char*)malloc(strlen(MELODY_STORED_DATA));
		strcpy(melody_buffer[0],MELODY_STORED_DATA);
	}
	else
	{
		melody_buffer[0] = NULL;
	}

	//strncpy(test,SONG_STORED_ID,100);
	//strncpy(test,MELODY_STORED_ID,100);
	//test[0] = test[0];

	//Disable access to the backup domain
	PWR_BackupAccessCmd(DISABLE);
}

void EEPROM_StoreSongAndMelody(char *song_buffer, char *melody_buffer)
{
	//Enable access to the backup domain
	//PWR_BackupAccessCmd(ENABLE);
	EEPROM_Init();

	if(song_buffer!=0)
	{
		if(!strncmp(song_buffer,"CLEAR",5))
		{
			strcpy(SONG_STORED_ID,""); //clear current content
			strcpy(SONG_STORED_DATA,"");
		}
		else
		{
			strcpy(SONG_STORED_ID,"SONG_STORED"); //write this string so we know a song was stored
			strcpy(SONG_STORED_DATA,song_buffer);
		}
	}

	if(melody_buffer!=0)
	{
		if(!strncmp(melody_buffer,"CLEAR",5))
		{
			strcpy(MELODY_STORED_ID,""); //clear current content
			strcpy(MELODY_STORED_DATA,"");
		}
		else
		{
			strcpy(MELODY_STORED_ID,"MELODY_STORED"); //write this string so we know a melody was stored
			strcpy(MELODY_STORED_DATA,melody_buffer);
		}
	}

	//char test[100];
	//strncpy(test,SONG_STORED_ID,100);
	//strncpy(test,MELODY_STORED_ID,100);
	//test[0] = test[0];

	//Disable access to the backup domain
	PWR_BackupAccessCmd(DISABLE);
}

void EEPROM_StoreSettings_B(uint32_t settings_position,uint8_t value)
{
	//Enable access to the backup domain
	//PWR_BackupAccessCmd(ENABLE);
	EEPROM_Init();

	//memcpy(settings_position,&value,1); //write one byte
	((uint8_t*)settings_position)[0] = value;

	//Disable access to the backup domain
	PWR_BackupAccessCmd(DISABLE);
}

void EEPROM_StoreSettings_W(uint32_t settings_position,uint16_t value)
{
	//Enable access to the backup domain
	//PWR_BackupAccessCmd(ENABLE);
	EEPROM_Init();

	//memcpy(settings_position,&value,2); //write one byte
	((uint16_t*)settings_position)[0] = value;

	//Disable access to the backup domain
	PWR_BackupAccessCmd(DISABLE);
}

void EEPROM_StoreSettings_DW(uint32_t settings_position,uint32_t value)
{
	//Enable access to the backup domain
	//PWR_BackupAccessCmd(ENABLE);
	EEPROM_Init();

	((uint32_t*)settings_position)[0] = value; //write four bytes

	//Disable access to the backup domain
	PWR_BackupAccessCmd(DISABLE);
}

uint8_t EEPROM_LoadSettings_B(uint32_t settings_position,uint8_t default_value)
{
	uint8_t value;

	//Enable access to the backup domain
	//PWR_BackupAccessCmd(ENABLE);
	EEPROM_Init();

	memcpy(&value, (char*)settings_position, 1); //load one byte

	//Disable access to the backup domain
	PWR_BackupAccessCmd(DISABLE);

	if(value==0)
	{
		return default_value;
	}
	return value;
}

uint16_t EEPROM_LoadSettings_W(uint32_t settings_position,uint16_t default_value)
{
	uint16_t value;

	//Enable access to the backup domain
	//PWR_BackupAccessCmd(ENABLE);
	EEPROM_Init();

	memcpy(&value, (char*)settings_position, 2); //load two bytes

	//Disable access to the backup domain
	PWR_BackupAccessCmd(DISABLE);

	if(value==0)
	{
		return default_value;
	}
	return value;
}

uint32_t EEPROM_LoadSettings_DW(uint32_t settings_position)
{
	uint32_t value;

	//Enable access to the backup domain
	//PWR_BackupAccessCmd(ENABLE);
	EEPROM_Init();

	memcpy(&value, (char*)settings_position, 4); //load four bytes

	//Disable access to the backup domain
	PWR_BackupAccessCmd(DISABLE);

	return value;
}

void EEPROM_ClearSettings()
{
	EEPROM_Init();
	memset((char*)SETTINGS_BASE,0,SETTINGS_RESERVED_LENGTH);
	strcpy((char*)SETTINGS_BASE,"SETTINGS");
	//Disable access to the backup domain
	PWR_BackupAccessCmd(DISABLE);
}
