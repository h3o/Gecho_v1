/*
 * codec.c
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

#include <hw/codec.h>
#include <hw/gpio.h>
#include <hw/eeprom.h>
#include <string.h>

volatile uint32_t sampleCounter = 0;
int codec_rate = 0; //default rate

uint8_t CodecCommandBuffer[5];
int hp_volume, codec_volume, codec_treble, codec_bass;
int tempo_bpm = 120;
int TEMPO_BY_SAMPLE, MELODY_BY_SAMPLE;

volatile int tempoCounter, melodyCounter;

void codec_init()
{
	GPIO_InitTypeDef PinInitStruct;
	GPIO_StructInit(&PinInitStruct);

	I2S_InitTypeDef I2S_InitType;
	I2C_InitTypeDef I2C_InitType;

	//Codec Reset pin as GPIO
	PinInitStruct.GPIO_Pin = CODEC_RESET_PIN;
	PinInitStruct.GPIO_Mode = GPIO_Mode_OUT;
	PinInitStruct.GPIO_PuPd = GPIO_PuPd_UP;
	PinInitStruct.GPIO_OType = GPIO_OType_PP;
	//PinInitStruct.GPIO_OType = GPIO_OType_OD; //boards that have external pullup and reset button linked to codec
	PinInitStruct.GPIO_Speed = GPIO_SPEED_CODEC_RESET;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOD | RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOC, ENABLE);

	GPIO_Init(CODEC_RESET_PORT, &PinInitStruct);

	// I2C pins
	PinInitStruct.GPIO_Mode = GPIO_Mode_AF; //GPIO_Mode_OUT; - Osc test
	PinInitStruct.GPIO_OType = GPIO_OType_OD;
	PinInitStruct.GPIO_Pin = I2C_SCL_PIN | I2C_SDA_PIN;
	PinInitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
	PinInitStruct.GPIO_Speed = GPIO_SPEED_CODEC_I2C;
	GPIO_Init(GPIOB, &PinInitStruct);

	/*
	//test SCL,SDA with oscilloscope
	int test_c = 0;
	while(1) {
		if(test_c%25000==0)GPIOB->BSRRL = I2C_SCL_PIN;
		if(test_c%50000==0)GPIOB->BSRRH = I2C_SCL_PIN;
		if(test_c%75000==0)GPIOB->BSRRL = I2C_SDA_PIN;
		if(test_c%100000==0)GPIOB->BSRRH = I2C_SDA_PIN;
		test_c++;
	}
	*/

	//STM32F4-DISC1 board - PB6,PB9
	//GPIO_PinAFConfig(GPIOB, GPIO_PinSource6, GPIO_AF_I2C1);
	//GPIO_PinAFConfig(GPIOB, GPIO_PinSource9, GPIO_AF_I2C1);

	//Gecho v0.01 - PB8,PB9
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource8, GPIO_AF_I2C1);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource9, GPIO_AF_I2C1);

	//enable I2S and I2C clocks
	//RCC_I2SCLKConfig(RCC_I2S2CLKSource_PLLI2S);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1 | RCC_APB1Periph_SPI3, ENABLE);
	RCC_PLLI2SCmd(ENABLE);

	// I2S pins
	PinInitStruct.GPIO_OType = GPIO_OType_PP;
	PinInitStruct.GPIO_Pin = I2S3_SCLK_PIN | I2S3_SD_PIN | I2S3_MCLK_PIN;
	PinInitStruct.GPIO_Speed = GPIO_SPEED_CODEC_I2S;
	GPIO_Init(GPIOC, &PinInitStruct);

	PinInitStruct.GPIO_Pin = I2S3_WS_PIN;
	GPIO_Init(GPIOA, &PinInitStruct);

	//prepare output ports for alternate function
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource4, GPIO_AF_SPI3);
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource7, GPIO_AF_SPI3);
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource10, GPIO_AF_SPI3);
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource12, GPIO_AF_SPI3);

	#ifdef CODEC_TLV
	//GPIO_PinAFConfig(GPIOB, GPIO_PinSource4, (uint8_t)0x07); //EXTSD -> AF7, as in datasheet (DM00037051 page 62)
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource4, GPIO_AF_I2S3ext); //EXTSD -> AF7 actually, as in stm32f4xx_gpio.h
	#endif

	//keep Codec off for now
	GPIO_ResetBits(CODEC_RESET_PORT, CODEC_RESET_PIN);

	// configure I2S port
	SPI_I2S_DeInit(CODEC_I2S);

	I2S_InitType.I2S_AudioFreq = codec_rate?I2S_AUDIOFREQ_HIGH:I2S_AUDIOFREQ;
	//rate can be 0 or 1 (default or max)

	I2S_InitType.I2S_MCLKOutput = I2S_MCLKOutput_Enable;
	I2S_InitType.I2S_DataFormat = I2S_DataFormat_16b;
	I2S_InitType.I2S_Mode = I2S_Mode_MasterTx;
	I2S_InitType.I2S_Standard = I2S_Standard_Phillips;
	I2S_InitType.I2S_CPOL = I2S_CPOL_Low;

	I2S_Init(CODEC_I2S, &I2S_InitType);
	//I2S_Cmd(CODEC_I2S, ENABLE); //do this later - in main.cpp

	#ifdef CODEC_TLV
	/* Configure the I2Sx_ext (the second instance) in Slave Receiver Mode */
	SPI_I2S_DeInit(CODEC_I2SEXT);
	I2S_FullDuplexConfig(CODEC_I2SEXT, &I2S_InitType);
	//I2S_Cmd(CODEC_I2SEXT, ENABLE); //do this later - in main.cpp

	//SPI_RxFIFOThresholdConfig (CODEC_I2SEXT, SPI_RxFIFOThreshold_QF);
	#endif

	// configure I2C port
	I2C_DeInit(CODEC_I2C);
	I2C_InitType.I2C_ClockSpeed = 100000;
	I2C_InitType.I2C_Mode = I2C_Mode_I2C;
	I2C_InitType.I2C_OwnAddress1 = CORE_I2C_ADDRESS;
	I2C_InitType.I2C_Ack = I2C_Ack_Enable;
	I2C_InitType.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
	I2C_InitType.I2C_DutyCycle = I2C_DutyCycle_2;

	I2C_Cmd(CODEC_I2C, ENABLE);
	I2C_Init(CODEC_I2C, &I2C_InitType);
}

void codec_reset()
{
	//take the reset line LOW, keeping Codec off
	GPIO_ResetBits(CODEC_RESET_PORT, CODEC_RESET_PIN);
}

void codec_restart()
{
	codec_init(); //audio codec - init but keep off (RESET -> LOW)
	codec_ctrl_init();
	I2S_Cmd(CODEC_I2S, ENABLE);
}

void codec_comm_blink_test(int phase)
{
	if(phase==1)
	{
		for(int i=0;i<50;i++)
		{
			Delay(20);
			LED_RDY_OFF;
			Delay(20);
			LED_RDY_ON;
		}
	}
	else if(phase==2)
	{
		for(int i=0;i<5;i++)
		{
			Delay(300);
			LED_RDY_ON;
			Delay(300);
			LED_RDY_OFF;
		}
	}
}

void codec_ctrl_init()
{
	uint32_t delaycount;
	uint8_t regValue = 0xFF;

	GPIO_SetBits(CODEC_RESET_PORT, CODEC_RESET_PIN); //release the /RESET
	delaycount = 1000000;
	while (delaycount > 0)
	{
		delaycount--;
	}

	CodecCommandBuffer[0] = CODEC_MAP_PLAYBACK_CTRL1; //CS43L22 datasheet page 43: 7.10 Playback Control 1 (Address 0Dh)
	//[7..5] HPGAIN - Headphone/Line Gain Setting (G) -> 000 = 0.3959
	//[4]    PLYBCKB=A - Single Volume Control for all Playback Channels -> 0 = Disabled
	//[3..2] INV_PCMB, INV_PCMA - PCM Signal Polarity -> 0 = Not Inverted
	//[1..0] MSTBMUTE, MSTAMUTE - Master Mute -> 1 = Muted
	CodecCommandBuffer[1] = 0x03;
	send_codec_ctrl(CodecCommandBuffer, 2);

	//begin initialization sequence (CS43L22 datasheet page 32: 4.11 - Required Initialization Settings)
	CodecCommandBuffer[0] = 0x00; //1. Write 0x99 to register 0x00
	CodecCommandBuffer[1] = 0x99;
	send_codec_ctrl(CodecCommandBuffer, 2);

	CodecCommandBuffer[0] = 0x47; //2. Write 0x80 to register 0x47
	CodecCommandBuffer[1] = 0x80;
	send_codec_ctrl(CodecCommandBuffer, 2);

	regValue = read_codec_register(0x32); //3. Write '1'b to bit 7 in register 0x32
	CodecCommandBuffer[0] = 0x32;
	CodecCommandBuffer[1] = regValue | 0x80;
	send_codec_ctrl(CodecCommandBuffer, 2);

	regValue = read_codec_register(0x32); //4. Write '0'b to bit 7 in register 0x32
	CodecCommandBuffer[0] = 0x32;
	CodecCommandBuffer[1] = regValue & (~0x80);
	send_codec_ctrl(CodecCommandBuffer, 2);

	CodecCommandBuffer[0] = 0x00; //5. Write 0x00 to register 0x00
	CodecCommandBuffer[1] = 0x00;
	send_codec_ctrl(CodecCommandBuffer, 2);
	//end of initialization sequence

	CodecCommandBuffer[0] = CODEC_MAP_PWR_CTRL2; //CS43L22 datasheet page 38: 7.3 Power Control 2 (Address 04h)
	//Headphone Power Control - Configures how the SPK/HP_SW pin, 6, controls the power for the headphone amplifier
	//[7..4] PDN_HPB1, PDN_HPB0, PDN_HPA1, PDN_HPA0 -> 10,10 = Headphone channel is always ON
	//Speaker Power Control - Configures how the SPK/HP_SW pin, 6, controls the power for the speaker amplifier
	//[3..0] PDN_SPKB1, PDN_SPKB0, PDN_SPKA1, PDN_SPKA0 -> 11,11 = Speaker channel is always OFF

	CodecCommandBuffer[1] = 0xAF; //enable headphones, disable speakers
	//CodecCommandBuffer[1] = 0xFA; //enable speakers, disable headphones

	send_codec_ctrl(CodecCommandBuffer, 2);

	CodecCommandBuffer[0] = CODEC_MAP_CLK_CTRL; //CS43L22 datasheet page 38: 7.4 Clocking Control (Address 05h)
	//Auto-Detect - Configures the auto-detect circuitry for detecting the speed mode of the CS43L22 when operating as a slave
	//[7] AUTO - Auto-detection of Speed Mode -> 1 = Enabled
	//[6..1] SPEED1, SPEED0, 32k_GROUP, VIDEOCLK, RATIO1, RATIO0
	//MCLK Divide By 2 - Divides the input MCLK by 2 prior to all internal circuitry
	//[0] MCLKDIV2 - MCLK signal into DAC -> 1 = Divided by 2
	CodecCommandBuffer[1] = 0x81; //auto detect the clock
	send_codec_ctrl(CodecCommandBuffer, 2);

	CodecCommandBuffer[0] = CODEC_MAP_IF_CTRL1; //CS43L22 datasheet page 40: 7.5 Interface Control 1 (Address 06h)
	//[7..4] M/S, INV_SCLK, Reserved, DSP -> all default values (slave, SCLK not inverted, DSP disabled)
	//DAC Interface Format - Configures the digital interface format for data on SDIN
	//[3..2] DACDIF1, DACDIF0 - DAC Interface Format -> 01 = I2S, up to 24-bit data
	//Audio Word Length - Configures the audio sample word length used for the data into SDIN
	//[1..0] AWL1, AWL0 - Audio Word Lengt -> 11 = 16-bit data
	CodecCommandBuffer[1] = 0x07;
	send_codec_ctrl(CodecCommandBuffer, 2);

	CodecCommandBuffer[0] = 0x0A; //CS43L22 datasheet page 42: 7.8 Analog ZC and SR Settings (Address 0Ah)
	CodecCommandBuffer[1] = 0x00; //no soft ramp, no zero crossing
	send_codec_ctrl(CodecCommandBuffer, 2);

	CodecCommandBuffer[0] = 0x27; //CS43L22 datasheet page 53: 7.23 Limiter Control 1, Min/Max Thresholds (Address 27h)
	CodecCommandBuffer[1] = 0x00; //all defaults, limiter off
	send_codec_ctrl(CodecCommandBuffer, 2);

	CodecCommandBuffer[0] = 0x1A | CODEC_MAPBYTE_INC;
	CodecCommandBuffer[1] = 0x0A;
	CodecCommandBuffer[2] = 0x0A;
	send_codec_ctrl(CodecCommandBuffer, 3);

	CodecCommandBuffer[0] = 0x1F; //CS43L22 datasheet page 50: 7.18 Tone Control (Address 1Fh)
	//Treble Gain - Sets the gain of the treble shelving filter
	//[7..4] TREB3..TREB0 - Gain Setting -> 0000 = +12.0 dB
	//Bass Gain - Sets the gain of the bass shelving filter
	//[3..0] BASS3..BASS0 - Gain Setting -> 1111 = -10.5 dB
	CodecCommandBuffer[1] = 0x0F; //treble +12dB, bass -10dB (why?)
	send_codec_ctrl(CodecCommandBuffer, 2);

	CodecCommandBuffer[0] = CODEC_MAP_PLAYBACK_CTRL1; //CS43L22 datasheet page 43: 7.10 Playback Control 1 (Address 0Dh)
	//[7..5] HPGAIN - Headphone/Line Gain Setting (G) -> 011 = 0.6047
	//[4]    PLYBCKB=A - Single Volume Control for all Playback Channels -> 1 = Enabled
	//[3..2] INV_PCMB, INV_PCMA - PCM Signal Polarity -> 0 = Not Inverted
	//[1..0] MSTBMUTE, MSTAMUTE - Master Mute -> 0 = Not Muted
	CodecCommandBuffer[1] = 0x70;
	send_codec_ctrl(CodecCommandBuffer, 2); //un-mute master playback, hp gain = 0.6047, PLYBCKB=A


	CodecCommandBuffer[0] = CODEC_MAP_MASTER_A_VOL; //0x20
	//CodecCommandBuffer[1] = 0x18; //+12db //recording
	//CodecCommandBuffer[1] = 0x10; //+8db
	//CodecCommandBuffer[1] = 0x08; //+4db //headphones level
	//CodecCommandBuffer[1] = CODEC_VOLUME_DEFAULT;
	codec_volume = (int8_t)(EEPROM_LoadSettings_B(SETTINGS_CODEC_VOLUME,0) + CODEC_VOLUME_DEFAULT);
	CodecCommandBuffer[1] = codec_volume;
	send_codec_ctrl(CodecCommandBuffer, 2); //master volume set - MSTA
	CodecCommandBuffer[0] = CODEC_MAP_MASTER_B_VOL; //0x21
	send_codec_ctrl(CodecCommandBuffer, 2); //master volume set - MSTB

	//Headphone Volume - HPA
	CodecCommandBuffer[0] = CODEC_MAP_HP_A_VOL; //0x22
	//CodecCommandBuffer[0] = CODEC_MAP_SPEAK_A_VOL; //0x24
	//CodecCommandBuffer[1] = 0x00; //+0db --max value
	//CodecCommandBuffer[1] = 0xe0; //-16db
	//CodecCommandBuffer[1] = 0xd0; //-24db
	//CodecCommandBuffer[1] = 0xc0; //-32db
	//CodecCommandBuffer[1] = HP_VOLUME_DEFAULT;
	hp_volume = (EEPROM_LoadSettings_B(SETTINGS_MAIN_VOLUME,0) + HP_VOLUME_DEFAULT) & 0xFF;
	if(hp_volume==0)
	{
		hp_volume=256;
	}
	CodecCommandBuffer[1] = hp_volume;

	send_codec_ctrl(CodecCommandBuffer, 2);
	//Headphone Volume - HPB
	CodecCommandBuffer[0] = CODEC_MAP_HP_B_VOL; //0x23
	//CodecCommandBuffer[0] = CODEC_MAP_SPEAK_B_VOL; //0x25
	send_codec_ctrl(CodecCommandBuffer, 2);

	/*
	//Miscellaneous Controls - enable de-emphasis
	CodecCommandBuffer[0] = CODEC_MAP_MISC_CTRL; //0x0e
	regValue = read_codec_register(CodecCommandBuffer[0]);
	CodecCommandBuffer[1] = regValue | 0x04; //bit #2 DEEMPH
	send_codec_ctrl(CodecCommandBuffer, 2);
	*/

	/*
	//beep frequency and on time
	CodecCommandBuffer[0] = CODEC_MAP_BEEP_FREQ_ONTIME; //0x1c
	//CodecCommandBuffer[1] = 0x60; //01100000 - 888.89 Hz, ~86ms
	CodecCommandBuffer[1] = 0xe0; //11100000 - 2000.00 Hz, ~86ms
	send_codec_ctrl(CodecCommandBuffer, 2);

	//beep volume and off time
	CodecCommandBuffer[0] = CODEC_MAP_BEEP_VOL_OFFTIME; //0x1d
	CodecCommandBuffer[1] = 0x08; //00001000
	send_codec_ctrl(CodecCommandBuffer, 2);
	*/

	//enable tone control - set TCEN bit [0] of CODEC_MAP_BEEP_TONE_CFG (0x1e)
	CodecCommandBuffer[0] = CODEC_MAP_BEEP_TONE_CFG; //0x1e
	//CodecCommandBuffer[1] = read_codec_register(CodecCommandBuffer[0]);
	//CodecCommandBuffer[1] |= 0x01; //bit[0]
	//CodecCommandBuffer[1] = 0x01; //bit[0] - tone control ON, corner freqs 50Hz/5kHz (default)
	CodecCommandBuffer[1] = 0x07; //bit[0] - tone control ON, corner freqs 250Hz/5kHz
	//CodecCommandBuffer[1] = 0x87; //10000111 tone control ON, corner freqs 250Hz/5kHz, beep multiple mode
	//CodecCommandBuffer[1] = 0xa7; //10100111 tone control ON, corner freqs 250Hz/5kHz, beep multiple mode w mix disable
	send_codec_ctrl(CodecCommandBuffer, 2);

	codec_treble = (int8_t)(EEPROM_LoadSettings_B(SETTINGS_EQ_TREBLE,0) + CODEC_TREBLE_DEFAULT);
	codec_bass = (int8_t)(EEPROM_LoadSettings_B(SETTINGS_EQ_BASS,0) + CODEC_BASS_DEFAULT);

	//tone control defaults
	CodecCommandBuffer[0] = CODEC_MAP_TONE_CTRL; //0x1f
	//CodecCommandBuffer[1] = 0x88; //all default
	CodecCommandBuffer[1] = codec_bass | (codec_treble << 4);
	send_codec_ctrl(CodecCommandBuffer, 2);

	#ifdef CODEC_OUTPUT_LIMITER
		//Limiter Control 1, Min/Max Thresholds (Address 27h)
		CodecCommandBuffer[0] = CODEC_MAP_LIMIT_CTRL1;
		//CodecCommandBuffer[1] = 0x00; //all default
		//CodecCommandBuffer[1] = 0xfa; //11111100 //-30db Threshold, -30db Cushion, DIGSFT, DIGZC - noticeable
		//CodecCommandBuffer[1] = 0xda; //11011100 //-24db Threshold, -30db Cushion, DIGSFT, DIGZC - okay
		//CodecCommandBuffer[1] = 0xb8; //10111000 //-18db Threshold, -24db Cushion, DIGSFT, DIGZC - little clipping
		//CodecCommandBuffer[1] = 0x94; //10010100 //-12db Threshold, -18db Cushion, DIGSFT, DIGZC - no clipping noticeable
		CodecCommandBuffer[1] = 0x70; //01110000 //-9db Threshold, -12db Cushion, DIGSFT, DIGZC - untested
		send_codec_ctrl(CodecCommandBuffer, 2);

		//Limiter Control 2, Release Rate (Address 28h)
		CodecCommandBuffer[0] = CODEC_MAP_LIMIT_CTRL2;
		CodecCommandBuffer[1] = 0xff; //11111111 //enabled,both channels,slowest release
		send_codec_ctrl(CodecCommandBuffer, 2);

		//Limiter Attack Rate (Address 29h)
		//CodecCommandBuffer[0] = CODEC_MAP_LIMIT_ATTACK;
		//CodecCommandBuffer[1] = 0x00; //all default, fastest attack
		//send_codec_ctrl(CodecCommandBuffer, 2);
	#endif

	//power up the codec
	CodecCommandBuffer[0] = CODEC_MAP_PWR_CTRL1; //Power Down - Configures the power state of the CS43L22.
	CodecCommandBuffer[1] = 0x9E; //1001 1110 Powered Up (1001 1111 Powered Down)
	send_codec_ctrl(CodecCommandBuffer, 2);
}

void codec_ctrl_init_TLV()
{
	uint32_t delaycount;
	uint8_t regValue = 0xFF;

	GPIO_SetBits(CODEC_RESET_PORT, CODEC_RESET_PIN); //release the /RESET
	delaycount = 1000000;
	while (delaycount > 0)
	{
		delaycount--;
	}

	//return;

	/*
	CodecCommandBuffer[0] = 0x00; //Page 0/Register 0: Page Select Register
	CodecCommandBuffer[1] = 0x01; //select page #1
	send_codec_ctrl(CodecCommandBuffer, 2);

	//re-read the value
	CodecCommandBuffer[0] = 0x00; //Page 0/Register 0: Page Select Register
	regValue = read_codec_register(CodecCommandBuffer[0]);
    */

	CodecCommandBuffer[0] = 0x00; //Page 0/Register 0: Page Select Register
	CodecCommandBuffer[1] = 0x00; //select page #0
	send_codec_ctrl(CodecCommandBuffer, 2);

	/*
	//re-read the value
	CodecCommandBuffer[0] = 0x00; //Page 0/Register 0: Page Select Register
	regValue = read_codec_register(CodecCommandBuffer[0]);
	*/

	CodecCommandBuffer[0] = 0x07; //Page 0/Register 7: Codec Data-Path Setup Register
	//D7->1: fS(ref) = 44.1 kHz
	//D4-D3->01: Left-DAC data path plays left-channel input data
	//D2-D1->01: Right-DAC data path plays right-channel input data
	CodecCommandBuffer[1] = 0x8a; //10001010
	send_codec_ctrl(CodecCommandBuffer, 2);

/*
	CodecCommandBuffer[0] = 0x02; //Page 0/Register 2: Codec Sample Rate Select Register
	//D7-D4: 0010: ADC fS = fS(ref)/2
	//D3-D0: 0010: DAC fS = fS(ref)/2
	CodecCommandBuffer[1] = 0x22; //00100010
	send_codec_ctrl(CodecCommandBuffer, 2);
*/

	CodecCommandBuffer[0] = 0x25; //Page 0/Register 37: DAC Power and Output Driver Control Register
	//D7: 1: Left DAC is powered up
	//D6: 1: Right DAC is powered up
	//D5-D4: 10: HPLCOM configured as independent single-ended output
	CodecCommandBuffer[1] = 0xe8; //11100000
	send_codec_ctrl(CodecCommandBuffer, 2);

	CodecCommandBuffer[0] = 0x26; //Page 0/Register 38: High-Power Output Driver Control Register
	//D5-D3: 010: HPRCOM configured as independent single-ended output
	//D2: 1: Short-circuit protection on all high-power output drivers is enabled
	//D1: 1: If short-circuit protection is enabled, it powers down the output driver automatically when a short is detected
	CodecCommandBuffer[1] = 0x16; //00010110
	send_codec_ctrl(CodecCommandBuffer, 2);

	CodecCommandBuffer[0] = 0x29; //Page 0/Register 41: DAC Output Switching Control Register
	//D7-D6: 00: Left-DAC output selects DAC_L1 path
	//D5-D4: 00: Right-DAC output selects DAC_R1 path
	//D1-D0: 10: Right-DAC volume follows the left-DAC digital volume control register
	CodecCommandBuffer[1] = 0x02; //00000010
	send_codec_ctrl(CodecCommandBuffer, 2);

	CodecCommandBuffer[0] = 0x2A; //Page 0/Register 42: Output Driver Pop Reduction Register
	//D7-D4: 1001: Driver power-on time = 800 ms
	//D7-D4: 0111: Driver power-on time = 200 ms
	//D3-D2: 10: Driver ramp-up step time = 2 ms
	//D1: 0: Weakly driven output common-mode voltage is generated from resistor divider off the AVDD supply
	//CodecCommandBuffer[1] = 0x98; //10011000 -> 800ms power on, 2ms step
	CodecCommandBuffer[1] = 0x78; //10011000 -> 200ms power on, 2ms step
	send_codec_ctrl(CodecCommandBuffer, 2);

	CodecCommandBuffer[0] = 0x2B; //Page 0/Register 43: Left-DAC Digital Volume Control Register
	//D7: 0: The left-DAC channel is not muted
	//D6-D0: 001 1000: Gain = -12 dB
	//CodecCommandBuffer[1] = 0x18; //00011000 -> -12dB
	CodecCommandBuffer[1] = CODEC_VOLUME_DEFAULT;
	send_codec_ctrl(CodecCommandBuffer, 2);

	codec_volume = CODEC_VOLUME_DEFAULT; //set default

	/*
	CodecCommandBuffer[0] = 0x2C; //Page 0/Register 44: Right-DAC Digital Volume Control Register
	//D7: 0: The right-DAC channel is not muted
	//D6-D0: 001 1000: Gain = -12 dB
	CodecCommandBuffer[1] = 0x18; //00011000
	send_codec_ctrl(CodecCommandBuffer, 2);
	*/

	CodecCommandBuffer[0] = 0x2F; //Page 0/Register 47: DAC_L1 to HPLOUT Volume Control Register
	//D7: 1: DAC_L1 is routed to HPLOUT
	//D6-D0: DAC_L1 to HPLOUT Analog Volume Control -> 000 1100: Gain = -6 dB
	CodecCommandBuffer[1] = 0x8C; //10001100
	send_codec_ctrl(CodecCommandBuffer, 2);

	/*
	CodecCommandBuffer[0] = 0x32; //Page 0/Register 50: DAC_R1 to HPLOUT Volume Control Register
	//D7: 1: DAC_R1 is routed to HPLOUT
	//D6-D0: DAC_R1 to HPLOUT Analog Volume Control -> 000 1100: Gain = -6 dB
	CodecCommandBuffer[1] = 0x8C; //10001100
	send_codec_ctrl(CodecCommandBuffer, 2);
	*/

	CodecCommandBuffer[0] = 0x33; //Page 0/Register 51: HPLOUT Output Level Control Register
	//D7-D4: 0000: Output level control = 0 dB
	//D3: 1: HPLOUT is not muted
	//D2: 0: HPLOUT is weakly driven to a common-mode when powered down
	//D1: 0: All programmed gains to HPLOUT have been applied
	//D0: 1: HPLOUT is fully powered up
	//CodecCommandBuffer[1] = 0x09; //00001001 -> 0dB output level
	CodecCommandBuffer[1] = 0x69; //01101001 -> 6dB output level

	send_codec_ctrl(CodecCommandBuffer, 2);

	CodecCommandBuffer[0] = 0x40; //Page 0/Register 64: DAC_R1 to HPROUT Volume Control Register
	//D7: 1: DAC_R1 is routed to HPROUT
	//D6-D0: DAC_R1 to HPROUT Analog Volume Control -> 000 1100: Gain = -6 dB
	CodecCommandBuffer[1] = 0x8C; //10001100
	send_codec_ctrl(CodecCommandBuffer, 2);

	CodecCommandBuffer[0] = 0x41; //Page 0/Register 65: HPROUT Output Level Control Register
	//D7-D4: 0000: Output level control = 0 dB
	//D3: 1: HPROUT is not muted
	//D2: 0: HPROUT is weakly driven to a common mode when powered down
	//D1: 0: All programmed gains to HPROUT have been applied
	//D0: 1: HPROUT is fully powered up
	//CodecCommandBuffer[1] = 0x09; //00001001 -> 0dB output level
	CodecCommandBuffer[1] = 0x69; //01101001 -> 6dB output level

	send_codec_ctrl(CodecCommandBuffer, 2);

	//verify if DAC and HP driver is powered up
	CodecCommandBuffer[0] = 0x00; //Page 0/Register 94: Module Power Status Register
	regValue = read_codec_register(CodecCommandBuffer[0]);

	//ADC setup -----------------------------------------------------------------------------------

	CodecCommandBuffer[0] = 0x0f; //Page 0/Register 15: Left-ADC PGA Gain Control Register
	//D7: 0: The left-ADC PGA is not muted
	//D6-D0: 000 1100: Gain = 6 dB
	//CodecCommandBuffer[1] = 0x0C; //00001100 -> 6dB
	CodecCommandBuffer[1] = 0x40; //01000000 -> 32dB
	send_codec_ctrl(CodecCommandBuffer, 2);

	CodecCommandBuffer[0] = 0x10; //Page 0/Register 16: Right-ADC PGA Gain Control Register
	//D7: 0: The right ADC PGA is not muted
	//D6-D0: 000 1100: Gain = 6 dB
	//CodecCommandBuffer[1] = 0x0C; //00001100 -> 6dB
	CodecCommandBuffer[1] = 0x40; //01000000 -> 32dB
	send_codec_ctrl(CodecCommandBuffer, 2);

	CodecCommandBuffer[0] = 0x13; //Page 0/Register 19: MIC1LP/LINE1LP to Left-ADC Control Register
	//D7: 1: MIC1LP/LINE1LP and MIC1LM/LINE1LM are configured in fully differential mode
	//D6-D3: 0010: Input level control gain = -3 dB
	//D2: 1: Left-ADC channel is powered up
	//D1-D0: 00: Left-ADC PGA soft-stepping at once per sample period
	CodecCommandBuffer[1] = 0x94; //10010100
	send_codec_ctrl(CodecCommandBuffer, 2);

	CodecCommandBuffer[0] = 0x16; //Page 0/Register 22: MIC1RP/LINE1RP to Right-ADC Control Register
	//D7: 1: MIC1RP/LINE1RP and MIC1RM/LINE1RM are configured in fully differential mode
	//D6-D3: 0010: Input level control gain = -3 dB
	//D2: 1: Right-ADC channel is powered up
	//D1-D0: 00: Right-ADC PGA soft-stepping at once per sample period
	CodecCommandBuffer[1] = 0x94; //10010100
	send_codec_ctrl(CodecCommandBuffer, 2);

	CodecCommandBuffer[0] = 0x19; //Page 0/Register 25: MICBIAS Control Register
	//D7-D6: 01: MICBIAS output is powered to 2 V
	CodecCommandBuffer[1] = 0x40; //01000000
	send_codec_ctrl(CodecCommandBuffer, 2);

	CodecCommandBuffer[0] = 0x6B; //Page 0/Register 107: New Programmable ADC Digital Path and I2C Bus Condition Register
	//D5-D4: 11: Left and right analog microphones are used
	CodecCommandBuffer[1] = 0x30; //00110000
	send_codec_ctrl(CodecCommandBuffer, 2);
}

void send_codec_ctrl(uint8_t controlBytes[], uint8_t numBytes)
{
	uint8_t bytesSent=0;

	while (I2C_GetFlagStatus(CODEC_I2C, I2C_FLAG_BUSY))
	{
		//just wait until no longer busy
	}

	I2C_GenerateSTART(CODEC_I2C, ENABLE);
	while (!I2C_GetFlagStatus(CODEC_I2C, I2C_FLAG_SB))
	{
		//wait for generation of start condition
	}
	I2C_Send7bitAddress(CODEC_I2C, CODEC_I2C_ADDRESS, I2C_Direction_Transmitter);
	while (!I2C_CheckEvent(CODEC_I2C, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
	{
		//wait for end of address transmission
	}
	while (bytesSent < numBytes)
	{
		I2C_SendData(CODEC_I2C, controlBytes[bytesSent]);
		bytesSent++;
		while (!I2C_CheckEvent(CODEC_I2C, I2C_EVENT_MASTER_BYTE_TRANSMITTING))
		{
			//wait for transmission of byte
		}
	}
	while(!I2C_GetFlagStatus(CODEC_I2C, I2C_FLAG_BTF))
	{
	    //wait until it's finished sending before creating STOP
	}
	I2C_GenerateSTOP(CODEC_I2C, ENABLE);
}

uint8_t read_codec_register(uint8_t mapbyte)
{
	uint8_t receivedByte = 0;

	while (I2C_GetFlagStatus(CODEC_I2C, I2C_FLAG_BUSY))
	{
		//just wait until no longer busy
	}

	I2C_GenerateSTART(CODEC_I2C, ENABLE);
	while (!I2C_GetFlagStatus(CODEC_I2C, I2C_FLAG_SB))
	{
		//wait for generation of start condition
	}

	I2C_Send7bitAddress(CODEC_I2C, CODEC_I2C_ADDRESS, I2C_Direction_Transmitter);
	while (!I2C_CheckEvent(CODEC_I2C, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
	{
		//wait for end of address transmission
	}

	I2C_SendData(CODEC_I2C, mapbyte); //sets the transmitter address
	while (!I2C_CheckEvent(CODEC_I2C, I2C_EVENT_MASTER_BYTE_TRANSMITTING))
	{
		//wait for transmission of byte
	}

	I2C_GenerateSTOP(CODEC_I2C, ENABLE);

	while (I2C_GetFlagStatus(CODEC_I2C, I2C_FLAG_BUSY))
	{
		//just wait until no longer busy
	}

	I2C_AcknowledgeConfig(CODEC_I2C, DISABLE);

	I2C_GenerateSTART(CODEC_I2C, ENABLE);
	while (!I2C_GetFlagStatus(CODEC_I2C, I2C_FLAG_SB))
	{
		//wait for generation of start condition
	}

	I2C_Send7bitAddress(CODEC_I2C, CODEC_I2C_ADDRESS, I2C_Direction_Receiver);
	while (!I2C_CheckEvent(CODEC_I2C, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED))
	{
		//wait for end of address transmission
	}

	while (!I2C_CheckEvent(CODEC_I2C, I2C_EVENT_MASTER_BYTE_RECEIVED))
	{
		//wait until byte arrived
	}
	receivedByte = I2C_ReceiveData(CODEC_I2C);

	I2C_GenerateSTOP(CODEC_I2C, ENABLE);

	return receivedByte;
}

int queue_codec_ctrl_length = 0;
int queue_codec_ctrl_progress = 0;
int queue_codec_ctrl_bytes_sent = 0;
uint8_t I2C_CommandBuffer[5];

int queue_codec_ctrl(uint8_t controlBytes[], uint8_t numBytes)
{
	if(queue_codec_ctrl_length) //if previous tx sequence not finished
	{
		return 0;
	}

	queue_codec_ctrl_length = numBytes + 3; //amount of bytes to tx plus 3 control states
	queue_codec_ctrl_progress = 0;
	queue_codec_ctrl_bytes_sent = 0;

	memcpy(I2C_CommandBuffer, controlBytes, numBytes);

	return queue_codec_ctrl_length; //return # of accepted bytes + states to process
}

void queue_codec_ctrl_process()
{
	if(!queue_codec_ctrl_length) //if tx sequence finished
	{
		return;
	}

	switch (queue_codec_ctrl_progress) {
		case 0:

			//check if no longer busy
			if(!I2C_GetFlagStatus(CODEC_I2C, I2C_FLAG_BUSY))
			{
				I2C_GenerateSTART(CODEC_I2C, ENABLE);
				queue_codec_ctrl_progress++;
				queue_codec_ctrl_length--;
			}
			return;

		case 1: //start condition request initialized

			//check if start condition generated
			if(I2C_GetFlagStatus(CODEC_I2C, I2C_FLAG_SB))
			{
				I2C_Send7bitAddress(CODEC_I2C, CODEC_I2C_ADDRESS, I2C_Direction_Transmitter);
				queue_codec_ctrl_progress++;
				queue_codec_ctrl_length--;
			}
			return;

		case 2: //start condition generated succesfully

			if(I2C_CheckEvent(CODEC_I2C, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
			{
				//send first byte
				I2C_SendData(CODEC_I2C, I2C_CommandBuffer[queue_codec_ctrl_bytes_sent]);
				queue_codec_ctrl_progress++;
				queue_codec_ctrl_length--;
				queue_codec_ctrl_bytes_sent++;
			}
			return;

	} //end case statement

	//if initial conditions plus first byte done and more bytes remain to transmit
	if(queue_codec_ctrl_progress>2 && queue_codec_ctrl_length>1)
	{
		//check if transmission of byte is done
		if(I2C_CheckEvent(CODEC_I2C, I2C_EVENT_MASTER_BYTE_TRANSMITTING))
		{
			//send next byte
			I2C_SendData(CODEC_I2C, I2C_CommandBuffer[queue_codec_ctrl_bytes_sent]);
			queue_codec_ctrl_progress++;
			queue_codec_ctrl_length--;
			queue_codec_ctrl_bytes_sent++;
		}
		return;
	}

	//if all sent, need to transmit stop condition
	if(queue_codec_ctrl_length==1)
	{
		//check if it's finished sending before creating STOP
		if(I2C_GetFlagStatus(CODEC_I2C, I2C_FLAG_BTF))
		{
			I2C_GenerateSTOP(CODEC_I2C, ENABLE);
			queue_codec_ctrl_progress++;
			queue_codec_ctrl_length--;
		}
		return;
	}
}

int queue_codec_ctrl_remaining()
{
	return queue_codec_ctrl_length;
}

/*
void mute_master_playback(int state)
{
	if(state)
	{
		//mute master playback
		CodecCommandBuffer[0] = CODEC_MAP_PLAYBACK_CTRL1;
		CodecCommandBuffer[1] = 0x13;
		send_codec_ctrl(CodecCommandBuffer, 2);

		//mute headphones
		CodecCommandBuffer[0] = CODEC_MAP_PLAYBACK_CTRL2;
		CodecCommandBuffer[1] = 0xf0;
		send_codec_ctrl(CodecCommandBuffer, 2);
	}
	else
	{
		//un-mute master playback, hp gain = 0.6047, PLYBCKB=A
		CodecCommandBuffer[0] = CODEC_MAP_PLAYBACK_CTRL1;
		CodecCommandBuffer[1] = 0x70;
		send_codec_ctrl(CodecCommandBuffer, 2);

		//un-mute headphones
		CodecCommandBuffer[0] = CODEC_MAP_PLAYBACK_CTRL2;
		CodecCommandBuffer[1] = 0x00;
		send_codec_ctrl(CodecCommandBuffer, 2);
	}
}
*/

int get_tempo_by_BPM(int bpm)
{
	int b = 2*I2S_AUDIOFREQ-AUDIOFREQ_DIV_CORRECTION; //120BPM (16s / loop);

	if(bpm==120)
	{
		return b;
	}

	b = ((int)((2*(double)I2S_AUDIOFREQ*120.0f/(double)bpm)/4))*4; //must be divisible by 4 and rounded down
	return b;


	//int TEMPO_BY_SAMPLE = (2*I2S_AUDIOFREQ-AUDIOFREQ_DIV_CORRECTION) * 3 / 2; //80 BPM (24s / loop)
	//int TEMPO_BY_SAMPLE = ((2*I2S_AUDIOFREQ-AUDIOFREQ_DIV_CORRECTION) * 3) / 4; //160 BPM (12s / loop) //33072
	//int TEMPO_BY_SAMPLE = 29400;//((2*I2S_AUDIOFREQ-AUDIOFREQ_DIV_CORRECTION) * 2) / 3; //180 BPM (10.6s / loop) //29397
}
