/*
 * Gecho Loopsynth official firmware
 *
 *  Created on: Apr 10, 2016
 *      Author: mario (http://gechologic.com/contact)
 *
 *  This file is part of the Gecho Loopsynth Firmware Development Framework.
 *  It can be used within the terms of CC-BY-NC-SA license.
 *
 *  Find more information at: http://gechologic.com/gechologists/
 */

#include "main.h"

//#define DIRECT_BUTTONS_LEDS_TEST //useful when assembling master DIY kit

int main(void)
{
	SystemInit();
	FlashAcceleratorInit();

	SysTick_Config(168000000/1000); //each 1000ms

	EEPROM_Init();
	//EEPROM_LoadSongAndMelody(&progression_str, &melody_str);
	EEPROM_Test();

	#ifdef CAN_BLOCK_SWD_DEBUG
		GPIO_Init_all(true);
	#else
		GPIO_Init_all(false);
	#endif

//------------------------------------------------------------

	//check_battery_level(); //TODO: add an option to disable this

	while(1) //select channel loop
	{
		GPIO_LEDs_Buttons_Reset();
		program_settings_reset();


		#ifdef DIRECT_BUTTONS_LEDS_TEST
		direct_buttons_LEDs_test();
		#endif

		codec_init(); //audio codec - init but keep off (RESET -> LOW)

		#ifndef CODEC_COMM_BLINK_TEST

		#ifdef DIRECT_PROGRAM_START
			channel = DIRECT_PROGRAM_START;
			LED_RDY_ON; //set RDY so the sensors will work later
		#else
			channel = select_channel();
		#endif

		#endif

		#ifdef CODEC_COMM_BLINK_TEST
			codec_comm_blink_test(1);
		#endif

		#ifdef CODEC_TLV
			codec_ctrl_init_TLV(); //new v2 boards - TLV320AIC3104
		#else
			codec_ctrl_init(); //audio codec - release reset, init controller
		#endif

		#ifdef CODEC_COMM_BLINK_TEST
			codec_comm_blink_test(2);
			while(1);
		#endif

		I2S_Cmd(CODEC_I2S, ENABLE);

		#ifdef CODEC_TLV
			/* Enable the I2Sx_ext peripheral for Full Duplex mode */
			I2S_Cmd(CODEC_I2SEXT, ENABLE);
		#endif

		//if(channel==11111)
		//{
			//legacy_reference_low_pass();
		//}

		custom_program_init(channel);
		filters_and_signals_init();
		custom_effect_init(channel);

		//float smx0=-1000000, smx1=1000000;
		//uint16_t si0=0, si1=0xffff;

		if(loop_type > 0)
		{
			audio_loop_no_filters();
		}
		else
		{
			audio_loop_filters();
		}

		//release some allocated memory
		if(fil != NULL)
		{
			delete(fil);
			fil = NULL;
		}

	} //select program loop
}
