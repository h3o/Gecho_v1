/*
 * codec.h
 *
 *  Created on: Apr 6, 2016
 *      Author: mayo
 *
 * Based on "EXAMPLE OF USING CS42L22 on STM32F4-Discovery" by A.Finkelmeyer
 * http://www.mind-dump.net/configuring-the-stm32f4-discovery-for-audio
 *
 *  This file is part of the Gecho Loopsynth Firmware Development Framework.
 *  It can be used within the terms of CC-BY-NC-SA license.
 *  It must not be distributed separately.
 *
 *  Find more information at: http://gechologic.com/gechologists/
 */

#include <board_config.h>
#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_i2c.h"
#include "stm32f4xx_spi.h"

#ifndef __CODEC_H
#define __CODEC_H

#define GPIO_SPEED_CODEC_RESET	GPIO_Speed_25MHz
#define GPIO_SPEED_CODEC_I2C	GPIO_Speed_25MHz
#define GPIO_SPEED_CODEC_I2S	GPIO_Speed_25MHz

//#define I2S_AUDIOFREQ		48000	//Dekrispator
#define I2S_AUDIOFREQ_HIGH	44100	//for some other DSP modes
//#define I2S_AUDIOFREQ		32000
#define I2S_AUDIOFREQ		22050	//default sampling rate for IIR filter modes
//#define I2S_AUDIOFREQ		16000
//#define I2S_AUDIOFREQ		I2S_AudioFreq_8k 	//chiptunes only

#define AUDIOFREQ_DIV_CORRECTION 4

//#define CODEC_WARM_UP_SAMPLES I2S_AUDIOFREQ / 2 //250ms
//#define CODEC_WARM_UP_SAMPLES I2S_AUDIOFREQ //500ms
#define CODEC_WARM_UP_SAMPLES (I2S_AUDIOFREQ * 2/3 + 1)//333ms with correction to not swap L and R channel

//timing definitions for default sampling rate of 22050Hz
#define TIMING_BY_SAMPLE_ONE_SECOND_W_CORRECTION sampleCounter==2*I2S_AUDIOFREQ-AUDIOFREQ_DIV_CORRECTION

#define TIMING_BY_SAMPLE_EVERY_1_MS	sampleCounter%(2*I2S_AUDIOFREQ/1000)	//1000Hz
#define TIMING_BY_SAMPLE_EVERY_10_MS	sampleCounter%(2*I2S_AUDIOFREQ/100)	//100Hz
#define TIMING_BY_SAMPLE_EVERY_20_MS	sampleCounter%(2*I2S_AUDIOFREQ/50)	//50Hz
#define TIMING_BY_SAMPLE_EVERY_25_MS	sampleCounter%(2*I2S_AUDIOFREQ/40)	//40Hz
#define TIMING_BY_SAMPLE_EVERY_40_MS	sampleCounter%(2*I2S_AUDIOFREQ/25)	//25Hz
#define TIMING_BY_SAMPLE_EVERY_50_MS	sampleCounter%(2*I2S_AUDIOFREQ/20)	//20Hz
#define TIMING_BY_SAMPLE_EVERY_83_MS	sampleCounter%(2*I2S_AUDIOFREQ/12)	//12Hz
#define TIMING_BY_SAMPLE_EVERY_100_MS	sampleCounter%(2*I2S_AUDIOFREQ/10)	//10Hz
#define TIMING_BY_SAMPLE_EVERY_125_MS	sampleCounter%(2*I2S_AUDIOFREQ/8)	//8Hz
#define TIMING_BY_SAMPLE_EVERY_166_MS	sampleCounter%(2*I2S_AUDIOFREQ/6)	//6Hz
#define TIMING_BY_SAMPLE_EVERY_200_MS	sampleCounter%(2*I2S_AUDIOFREQ/5)	//5Hz
#define TIMING_BY_SAMPLE_EVERY_250_MS	sampleCounter%(2*I2S_AUDIOFREQ/4)	//4Hz
#define TIMING_BY_SAMPLE_EVERY_500_MS	sampleCounter%(2*I2S_AUDIOFREQ/2)	//2Hz

//codec signal pins
#define I2S3_WS_PIN 	GPIO_Pin_4   //port A (WS = LRCK)
#define I2S3_MCLK_PIN 	GPIO_Pin_7   //port C
#define I2S3_SCLK_PIN 	GPIO_Pin_10  //port C
#define I2S3_SD_PIN 	GPIO_Pin_12  //port C

//STM32F4-DISC1 board
//#define CODEC_RESET_PIN GPIO_Pin_4  //port D
//#define CODEC_RESET_PORT GPIOD  //port D

//STM32F4-Proto1 board
//#define CODEC_RESET_PIN GPIO_Pin_5  //port A
//#define CODEC_RESET_PORT GPIOA  //port A

//Gecho v0.01 - PC11
#define CODEC_RESET_PIN GPIO_Pin_11  //port C
#define CODEC_RESET_PORT GPIOC  //port C

//STM32F4-DISC1 board - PB6
//#define I2C_SCL_PIN		GPIO_Pin_6  //port B
//#define I2C_SDA_PIN		GPIO_Pin_9  //port B

//Gecho v0.01 - PB8
#define I2C_SCL_PIN		GPIO_Pin_8  //port B
#define I2C_SDA_PIN		GPIO_Pin_9  //port B

#define CODEC_I2C I2C1
#define CODEC_I2S SPI3
#define CODEC_I2SEXT I2S3ext

#define CORE_I2C_ADDRESS 0x33

#ifdef CODEC_TLV

#define CODEC_I2C_ADDRESS 0x30 //TLV320AIC3104

#else

#ifdef CODEC_I2C_ADDRESS_NORMAL_AD0_LOW
	#define CODEC_I2C_ADDRESS 0x94 //normal CS43L22
#endif

#ifdef CODEC_I2C_ADDRESS_ALTERNATE_AD0_HIGH
	#define CODEC_I2C_ADDRESS 0x96 //the problematic one!
#endif

#endif

#define CODEC_MAPBYTE_INC 0x80

//register map bytes for CS42L22 (see page 35 of https://d3uzseaevmutz1.cloudfront.net/pubs/proDatasheet/CS43L22_F2.pdf)
#define CODEC_MAP_CHIP_ID				0x01
#define CODEC_MAP_PWR_CTRL1				0x02
#define CODEC_MAP_PWR_CTRL2				0x04
#define CODEC_MAP_CLK_CTRL				0x05
#define CODEC_MAP_IF_CTRL1				0x06
#define CODEC_MAP_IF_CTRL2				0x07
#define CODEC_MAP_PASSTHROUGH_A_SELECT	0x08
#define CODEC_MAP_PASSTHROUGH_B_SELECT	0x09
#define CODEC_MAP_ANALOG_SET			0x0A
#define CODEC_MAP_PASSTHROUGH_GANG_CTRL 0x0C
#define CODEC_MAP_PLAYBACK_CTRL1		0x0D
#define CODEC_MAP_MISC_CTRL				0x0E
#define CODEC_MAP_PLAYBACK_CTRL2		0x0F
#define CODEC_MAP_PASSTHROUGH_A_VOL 	0x14
#define CODEC_MAP_PASSTHROUGH_B_VOL 	0x15
#define CODEC_MAP_PCMA_VOL				0x1A
#define CODEC_MAP_PCMB_VOL				0x1B
#define CODEC_MAP_BEEP_FREQ_ONTIME		0x1C
#define CODEC_MAP_BEEP_VOL_OFFTIME		0x1D
#define CODEC_MAP_BEEP_TONE_CFG			0x1E
#define CODEC_MAP_TONE_CTRL				0x1F
#define CODEC_MAP_MASTER_A_VOL			0x20
#define CODEC_MAP_MASTER_B_VOL			0x21
#define CODEC_MAP_HP_A_VOL				0x22
#define CODEC_MAP_HP_B_VOL				0x23
#define CODEC_MAP_SPEAK_A_VOL			0x24
#define CODEC_MAP_SPEAK_B_VOL			0x25
#define CODEC_MAP_CH_MIX_SWAP			0x26
#define CODEC_MAP_LIMIT_CTRL1			0x27
#define CODEC_MAP_LIMIT_CTRL2			0x28
#define CODEC_MAP_LIMIT_ATTACK			0x29
#define CODEC_MAP_OVFL_CLK_STATUS		0x2E
#define CODEC_MAP_BATT_COMP				0x2F
#define CODEC_MAP_VP_BATT_LEVEL			0x30
#define CODEC_MAP_SPEAK_STATUS			0x31
#define CODEC_MAP_CHARGE_PUMP_FREQ		0x34

//#define HP_VOLUME_DEFAULT			0xf0 //-8db(?)
#define HP_VOLUME_DEFAULT			0xe8 //-12db(?)
//#define HP_VOLUME_DEFAULT			0xe0 //-16db(?)
//#define HP_VOLUME_DEFAULT			0xd8 //-20db(?)
//#define HP_VOLUME_DEFAULT			0xd0 //-24db(?)
#define HP_VOLUME_MAX				 256 //wraps back to 0 -> 0db //recording -- max value
#define HP_VOLUME_MIN				 128 //-64dB in 0.5db steps (int val for comparison)

//#define CODEC_VOLUME_DEFAULT			0x10 //+8db
#define CODEC_VOLUME_DEFAULT			0x08 //+4db //headphones level
//#define CODEC_VOLUME_DEFAULT			0x00 //0db(?)
//#define CODEC_VOLUME_DEFAULT			0xf8 //-4db(?)
//#define CODEC_VOLUME_DEFAULT			  -8 //-4db (int val for comparison)
//#define CODEC_VOLUME_DEFAULT			0xf0 //-8db(?)
//#define CODEC_VOLUME_DEFAULT			0xe0 //-16db(?)
#define CODEC_VOLUME_MAX				0x18 //+12db //recording -- max value
#define CODEC_VOLUME_MIN				 -32 //-16db (int val for comparison)

//#define CODEC_TREBLE_DEFAULT			0x08
#define CODEC_TREBLE_DEFAULT			0x0e //adjusted for bit worse common headphones
#define CODEC_TREBLE_MIN				0x0f //meaning of the value is reversed
#define CODEC_TREBLE_MAX				0x00

#define CODEC_BASS_DEFAULT				0x08
#define CODEC_BASS_MIN					0x0f //meaning of the value is reversed
#define CODEC_BASS_MAX					0x00

//#define VOLUME_CONTROL_BY_MST
#define VOLUME_CONTROL_BY_HP

#define FEED_DAC_WITH_SILENCE	while(!SPI_I2S_GetFlagStatus(CODEC_I2S,SPI_I2S_FLAG_TXE));SPI_I2S_SendData(CODEC_I2S,0)

extern volatile uint32_t sampleCounter;
extern int codec_rate;

extern uint8_t CodecCommandBuffer[5];
extern int hp_volume, codec_volume, codec_treble, codec_bass;


extern int tempo_bpm, TEMPO_BY_SAMPLE, MELODY_BY_SAMPLE;
#define TEMPO_BPM_DEFAULT 120
extern volatile int tempoCounter, melodyCounter;

//#define TEMPO_BPM_MIN 10
//#define TEMPO_BPM_MAX 320
//#define TEMPO_BPM_STEP 10

//range & indicator v1:
//#define TEMPO_BPM_MIN 15
//#define TEMPO_BPM_MAX 290
//#define TEMPO_BPM_STEP 5

//range & indicator v2:
//#define TEMPO_BPM_MIN 15
//#define TEMPO_BPM_MAX 310
//#define TEMPO_BPM_STEP 5

//range & indicator v3:
#define TEMPO_BPM_MIN 10
#define TEMPO_BPM_MAX 330
#define TEMPO_BPM_STEP 5

#ifdef __cplusplus
 extern "C" {
#endif

void codec_init();
void codec_reset();
void codec_restart();
void codec_comm_blink_test(int phase);
void codec_ctrl_init();
void codec_ctrl_init_TLV();
void send_codec_ctrl(uint8_t controlBytes[], uint8_t numBytes);
uint8_t read_codec_register(uint8_t mapByte);

int queue_codec_ctrl(uint8_t controlBytes[], uint8_t numBytes);
void queue_codec_ctrl_process();
int queue_codec_ctrl_remaining();
//void mute_master_playback(int state);

int get_tempo_by_BPM(int bpm);

#ifdef __cplusplus
}
#endif

#endif /* __CODEC_H */
