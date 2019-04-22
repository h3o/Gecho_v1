/*
 * eeprom.h
 *
 *  Created on: Jul 14, 2016
 *      Author: mayo
 */

#ifndef EEPROM_H_
#define EEPROM_H_

#include "stm32f4xx.h"

#ifdef __cplusplus
 extern "C" {
#endif

#define SETTINGS_BASE  (BKPSRAM_BASE + 4000)
#define SETTINGS_RESERVED_LENGTH 64

#define BKPSRAM_TEST_BASE (SETTINGS_BASE + SETTINGS_RESERVED_LENGTH) //test space at the very end, after settings space
#define BKPSRAM_TEST_LENGTH 32

#define SONG_STORED_ID		(char *) (BKPSRAM_BASE+128)
#define SONG_STORED_DATA	(char *) (BKPSRAM_BASE+128+16)
#define MELODY_STORED_ID	(char *) (BKPSRAM_BASE+2048)
#define MELODY_STORED_DATA	(char *) (BKPSRAM_BASE+2048+16)

#define SETTINGS_MAIN_VOLUME	(SETTINGS_BASE + 8) //after the "SETTINGS" string
#define SETTINGS_DELAY_LENGTH	(SETTINGS_BASE + 9)
#define SETTINGS_INPUT_SELECT	(SETTINGS_BASE + 10)
#define SETTINGS_EQ_BASS		(SETTINGS_BASE + 11)
//#define SETTINGS_EQ_TREBLE		(SETTINGS_BASE + 12)
#define SETTINGS_EQ_TREBLE		(SETTINGS_BASE + 13)
#define SETTINGS_INPUT_GAIN		(SETTINGS_BASE + 14) //length = 4Bytes
#define SETTINGS_CODEC_VOLUME	(SETTINGS_BASE + 18) //length = 2Bytes

#define SETTINGS_WAVETABLE_SAMPLE (SETTINGS_BASE + 20) //length = 1Byte
#define SETTINGS_TEMPO_BPM		(SETTINGS_BASE + 22) //length = 2Bytes

void EEPROM_Init();
void EEPROM_Test();
void EEPROM_LoadSongAndMelody(char **song_buffer, char **melody_buffer);
void EEPROM_StoreSongAndMelody(char *song_buffer, char *melody_buffer);

void EEPROM_StoreSettings_B(uint32_t settings_position,uint8_t value);
void EEPROM_StoreSettings_W(uint32_t settings_position,uint16_t value);
void EEPROM_StoreSettings_DW(uint32_t settings_position,uint32_t value);
uint8_t EEPROM_LoadSettings_B(uint32_t settings_position,uint8_t default_value);
uint16_t EEPROM_LoadSettings_W(uint32_t settings_position,uint16_t default_value);
uint32_t EEPROM_LoadSettings_DW(uint32_t settings_position);
void EEPROM_ClearSettings();

#ifdef __cplusplus
}
#endif

#endif /* EEPROM_H_ */
