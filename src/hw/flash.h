/*
 * flash.h
 *
 *  Created on: Oct 7, 2017
 *      Author: mayo
 */

#ifndef FLASH_H_
#define FLASH_H_

#include "stm32f4xx.h"
#include <stddef.h>

#ifdef __cplusplus
 extern "C" {
#endif

#define COMPILE_PRODUCTION

#ifdef COMPILE_PRODUCTION
//production, when code <128k
#define SECTOR_BASE				0x08020000
#define SECTOR_LENGTH			0x20000 //128kB
#define SECTOR_ID				FLASH_Sector_5
#define FLASH_VOLTAGE_RANGE		VoltageRange_3

#else

//debug, when code >128k
#define SECTOR_BASE				0x08040000
#define SECTOR_LENGTH			0x20000 //128kB
#define SECTOR_ID				FLASH_Sector_6
#define FLASH_VOLTAGE_RANGE		VoltageRange_3

#endif

#define FLASH_CONTENT_MAP_SIZE	8192 //reserved space - should be enough for all user custom songs in overriddable channels

#define ACTIVATION_LOCK_ADDRESS	(SECTOR_BASE+1337*16)
#define ACTIVATION_LOCK_DWORD	((uint64_t)0xdeadbeefc0de10cc)

#define OVERRIDABLE_CHANNELS 85
static const int overridable_channels[OVERRIDABLE_CHANNELS] = {
	1,
	2,
	3,
	4,
	111,
	3411,
	3412,
	3413,
	3414,
	3421,
	3422,
	3423,
	3424,
	3431,
	3432,
	3433,
	3434,
	3441,
	3442,
	3443,
	3444,
	4111,
	4112,
	4113,
	4114,
	4121,
	4122,
	4123,
	4124,
	4131,
	4132,
	4133,
	4134,
	4141,
	4142,
	4143,
	4144,
	4211,
	4212,
	4213,
	4214,
	4221,
	4222,
	4223,
	4224,
	4231,
	4232,
	4233,
	4234,
	4241,
	4242,
	4243,
	4244,
	4311,
	4312,
	4313,
	4314,
	4321,
	4322,
	4323,
	4324,
	4331,
	4332,
	4333,
	4334,
	4341,
	4342,
	4343,
	4344,
	4411,
	4412,
	4413,
	4414,
	4421,
	4422,
	4423,
	4424,
	4431,
	4432,
	4433,
	4434,
	4441,
	4442,
	4443,
	4444
};

extern char **overridable_memory_map;

int FLASH_channel_to_position(int channel);
int FLASH_position_to_channel(int position);
void FLASH_load_memory_map();
void FLASH_save_memory_map();
void FLASH_delete_memory_map();

void FLASH_save_to_overridable_channel(char *command);
char *FLASH_load_from_overridable_channel(char *command);

int FLASH_all_empty();

void FLASH_LoadChannelMap(char **return_buffer);
void FLASH_LoadOverriddenChannel(int channel, char **song, char **melody, char **settings);
int FLASH_IsOverriddenChannel(int channel);

void FLASH_erase_all_custom_data();
void FLASH_set_activation_lock();
int FLASH_check_activation_lock();

#ifdef __cplusplus
}
#endif

#endif /* FLASH_H_ */
