/*
 * controls.c
 *
 *  Created on: Sep 24, 2016
 *      Author: mayo
 *
 *  This file is part of the Gecho Loopsynth Firmware Development Framework.
 *  It can be used within the terms of CC-BY-NC-SA license.
 *  It must not be distributed separately.
 *
 *  Find more information at: http://gechologic.com/gechologists/
 */

#include "controls.h"
#include <board_config.h>
#include <hw/eeprom.h>
#include <hw/gpio.h>
#include <hw/leds.h>
#include <hw/sensors.h>
#include <hw/signals.h>
#include <hw/sha1.h>
#include <hw/flash.h>
#include "notes.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//===== BEGIN UID HASH BLOCK ===========================================
//const char *BINARY_ID = "[0x133700000000]\n\0\0"; //test
const char *BINARY_ID = "[0x000000000090]\n\0\0"; //dev
const char *BINARY_HASH = "1f16a6c117f12c1539c4947e234de78a"; //random
const char *UID_HASH = "2b164b06c0018d1ec0db6f1960c874ce10102353"; //#90: production board after #27 failed (tung simple logo SWD)
//===== END UID HASH BLOCK =============================================

const char *FW_VERSION = "[FW=0.243]";

int mode_set_flag = 0;

static int buttons_state[USER_BUTTONS_N] = {0,0,0,0}; //user buttons 1-4 //todo: clear array programmatically
static int button_PWR_state = 0;

uint64_t select_channel()
{
	int button;
	int binary_status_O4 = 0;
	int loop = 0;
	int press_order[PRESS_ORDER_MAX], press_order_ptr = 0;
	uint64_t result = 0; //2^64 = 18446744073709551616 -> max 20 digits (or 19 if can start with 4)

	#ifdef USART1_CONTROL_MODE
		//char usart_command[100];
		//int usart_command_ptr = 0;
		GPIO_Init_USART1(115200);//9600); //revert USART1 Rx to input (set both Rx and Tx pins to AF)
		//int usart_receiving = 0;
		//char usart_data;
		int usart_timeout = 0;
		#define USART_TIMEOUT 10
	#endif

	button_PWR_state = 0;

	while(1)
	{
		//new_random_value(); //shuffle the random value for better entropy

		#ifdef USART1_CONTROL_MODE

		if(usart_command_received)
		{
			if(0==strncmp(usart_command,"Hi Gecho!",9))
			{
				USART_puts(USART1, "Hi there!\n");
			}
			else if(0==strncmp(usart_command,"BTN=",4))
			{
				if(0==strncmp(usart_command+4,"SET",3))
				{
					result = get_user_buttons_sequence(&press_order_ptr, press_order);
					if(result>0)
					{
						usart_command_received = 0; //clear the flag, so new command can be received
						return result;
					}
				}
				else if(0==strncmp(usart_command+4,"RST",3))
				{
					NVIC_SystemReset();
				}
				else
				{
					int btn_number = usart_command[4] - '1';
					if(btn_number >=0 && btn_number <=3)
					{
						user_button_pressed(btn_number, &binary_status_O4, &press_order_ptr, press_order);
					}
				}
			}
			else if(0==strncmp(usart_command,"CHAN=",5))
			{
				uint64_t channel_nr = atoi(usart_command+5);
				if(channel_nr > 0)
				{
					usart_command_received = 0; //clear the flag, so new command can be received
					return channel_nr;
				}
			}
			else if(0==strncmp(usart_command,"SONG=",5))
			{
                set_progression_str(usart_command+5);
				usart_command_received = 0; //clear the flag, so new command can be received
				ack_by_signal_LED();
			}
			else if(0==strncmp(usart_command,"MELODY=",7))
			{
				set_melody_str(usart_command+7);
				usart_command_received = 0; //clear the flag, so new command can be received
				ack_by_signal_LED();
			}
			else if(0==strncmp(usart_command,"SONG?=",6))
			{
				usart_command_received = 0; //clear the flag, so new command can be received
				//ack_by_signal_LED();
				EEPROM_LoadSongAndMelody(&progression_str, &melody_str);
				if(melody_str!=NULL)
				{
					free(melody_str);
				}
				if(progression_str!=NULL)
				{
					USART_puts(USART1, progression_str);
					free(progression_str);
				}
				USART_puts(USART1, "\n");
				NVIC_SystemReset();
			}
			else if(0==strncmp(usart_command,"MELODY?=",8))
			{
				usart_command_received = 0; //clear the flag, so new command can be received
				//ack_by_signal_LED();
				EEPROM_LoadSongAndMelody(&progression_str, &melody_str);
				if(progression_str!=NULL)
				{
					free(progression_str);
				}
				if(melody_str!=NULL)
				{
					USART_puts(USART1, melody_str);
					free(melody_str);
				}
				USART_puts(USART1, "\n");
				NVIC_SystemReset();
			}
			else if(0==strncmp(usart_command,"SAVE=",5))
			{
				LED_SIG_ON; //this will take a while, indicate by glowing SIG LED
				FLASH_save_to_overridable_channel(usart_command+5);
				usart_command_received = 0; //clear the flag, so new command can be received
				LED_SIG_OFF;
				NVIC_SystemReset();
			}
			else if(0==strncmp(usart_command,"LOAD=",5))
			{
				LED_SIG_ON;
				char *buf = FLASH_load_from_overridable_channel(usart_command+5);
				USART_puts(USART1, buf);
				USART_puts(USART1, ";\n");
				free(buf);
				usart_command_received = 0; //clear the flag, so new command can be received
				LED_SIG_OFF;
				NVIC_SystemReset();
			}
			else if(0==strncmp(usart_command,"PREVIEW=",8))
			{
				LED_SIG_ON;

				int channel = 999;
				if(0==strncmp(usart_command+8,"HP",2))
				{
					channel = 9995;
				}

				int composition_good = preview_composition(usart_command+11);
				if(!composition_good)
				{
					NVIC_SystemReset();
				}
				usart_command_received = 0; //clear the flag, so new command can be received
				LED_SIG_OFF;
				return channel;
			}
			else if(0==strncmp(usart_command,"ERASE=ALL",9))
			{
				LED_SIG_ON;
				FLASH_erase_all_custom_data();
				usart_command_received = 0; //clear the flag, so new command can be received
				LED_SIG_OFF;
				NVIC_SystemReset();
			}
			else if(0==strncmp(usart_command,"MAP?",4))
			{
				usart_command_received = 0; //clear the flag, so new command can be received
				//ack_by_signal_LED();
				char *channel_map;
				FLASH_LoadChannelMap(&channel_map);
				if(channel_map!=NULL)
				{
					USART_puts(USART1, channel_map);
					USART_puts(USART1, "\n");
					free(channel_map);
				}
				else
				{
					USART_puts(USART1, "MAP_EMPTY\n");
				}
				NVIC_SystemReset();
			}
			else if(0==strncmp(usart_command,"FN=",3))
			{
				if(0==strncmp(usart_command+3,"UID",3))
				{
					uint32_t uid_w0 = ((uint32_t*)UID_BASE_ADDRESS)[0];
					uint32_t uid_w1 = ((uint32_t*)UID_BASE_ADDRESS)[1];
					uint32_t uid_w2 = ((uint32_t*)UID_BASE_ADDRESS)[2];

					char *buf = (char*)malloc(32);
					sprintf(buf, "[%x-%x-%x]\n", uid_w0, uid_w1, uid_w2);

					USART_puts(USART1, buf);
					free(buf);
				}
				else if(0==strncmp(usart_command+3,"BNO",3))
				{
					USART_puts(USART1, (char*)BINARY_ID);
				}
				else if(0==strncmp(usart_command+3,"BHS",3))
				{
					USART_puts(USART1, (char*)BINARY_HASH);
				}
				else if(0==strncmp(usart_command+3,"UHS",3))
				{
					USART_puts(USART1, (char*)UID_HASH);
				}
				else if(0==strncmp(usart_command+3,"VER",3))
				{
					USART_puts(USART1, (char*)FW_VERSION);
				}
			}

			usart_command_received = 0; //clear the flag, so new command can be received
			usart_timeout = 0; //reset the timeout
		}
		else if(usart_command_length) //if started receiving
		{
			usart_timeout++;
			if(usart_timeout==USART_TIMEOUT)
			{
				usart_command_length = 0;
				usart_timeout = 0;
			}
		}

		/*
		// Wait until data is received
		if(USART_GetFlagStatus(USART1, USART_FLAG_RXNE))
		{
			LED_Y1_ON; //signal LED on
			usart_receiving = 1;
			usart_timeout = 0;

			// Read received char
			usart_data = USART_ReceiveData(USART1);
			//USART_SendData(USART1,data); //echo back

			usart_command[usart_command_ptr++] = usart_data;

			if(usart_data=='\r')
			{
				LED_Y1_OFF; //signal LED off
				//parse and execute command
				usart_command[usart_command_ptr] = 0;

				if(0==strncmp(usart_command,"CMD BTN",7))
				{
					user_button_pressed(usart_command[7] - '1', &binary_status_O4, &press_order_ptr, press_order);
				}

				usart_command_ptr = 0;
				usart_receiving = 0;
			}
		}
		else
		{
			if(usart_receiving)
			{
				usart_timeout++;
				if(usart_timeout==USART_TIMEOUT)
				{
					LED_Y1_OFF; //signal LED off
					usart_command_ptr = 0;
					usart_receiving = 0;
				}
			}
			else
			{
			*/

		#endif

			if(loop % 100 == 0)
			{
				LED_RDY_ON;
			}
			if(loop % 200 == 100)
			{
				LED_RDY_OFF;
			}
			loop++;

			if((button = scan_buttons()) > -1) //some button was just pressed
			{
				if(button<4)
				{
					user_button_pressed(button, &binary_status_O4, &press_order_ptr, press_order);
				}
				else if(button==4) //PWR pressed for 100ms
				{
					result = get_user_buttons_sequence(&press_order_ptr, press_order);
					if(result>0)
					{
						//since the channel was selected manually and not via serial command, will block serial
						GPIO_Deinit_USART1();
						return result;
					}
				}
				else if(button==5) //PWR pressed for 1 second
				{
					if(press_order_ptr == 0) //if no program chosen, turn off
					{
						mcu_and_codec_shutdown();
					}
					else
					{
						result = get_user_buttons_sequence(&press_order_ptr, press_order);
						//since the channel was selected manually and not via serial command, will block serial
						GPIO_Deinit_USART1();
						return result * 10 + 5; //append 5 to the decimal expansion of the number
					}
				}
				else if(button==6) //PWR pressed for 3 seconds, power off anyway
				{
					mcu_and_codec_shutdown();
				}
			}
			//#ifndef USART1_CONTROL_MODE
			//if(!usart_receiving)
			//{
			Delay(SCAN_BUTTONS_LOOP_DELAY);

			new_random_value(); //get some entropy
			//}
			//#endif
		//}
	}
}

uint64_t get_user_buttons_sequence(int *press_order_ptr, int *press_order)
{
	uint64_t res = 0;
	if(press_order_ptr[0] > 0) //if some program chosen, run it!
	{
		LED_RDY_ON; //set RDY so the sensors will work later
		//return binary_status_O4;
		for(int i=0;i<press_order_ptr[0];i++)
		{
			res *=10;
			res += press_order[i]+1;
		}
	}
	return res;
}

void user_button_pressed(int button, int *binary_status_O4, int *press_order_ptr, int *press_order)
{
	binary_status_O4[0] ^= (0x01 << button);
	if(binary_status_O4[0] & (0x01 << button))
	{
		LED_O4_set(button, 1); //set n-th LED on
	}
	else
	{
		LED_O4_set(button, 0); //set n-th LED off
	}
	if(press_order_ptr[0]<PRESS_ORDER_MAX)
	{
		press_order_ptr[0]++;
	}
	press_order[press_order_ptr[0]-1] = button;
}

bool get_button_state(int button)
{
	if(button==0)
	{
		return BUTTON_U1_ON;
	}
	if(button==1)
	{
		return BUTTON_U2_ON;
	}
	if(button==2)
	{
		return BUTTON_U3_ON;
	}
	if(button==3)
	{
		return BUTTON_U4_ON;
	}
	if(button==4)
	{
		return BUTTON_SET_ON;
	}
	return false;
}

int scan_buttons()
{
	for(int b=0;b<4;b++)
	{
		if(get_button_state(b))
		{
			if(buttons_state[b] < USER_BUTTONS_DEBOUNCE)
			{
				buttons_state[b]++;
			}
			else if(buttons_state[b] == USER_BUTTONS_DEBOUNCE)
			{
				buttons_state[b]++;
				return b;
			}
		}
		else
		{
			buttons_state[b] = 0;
		}
	}

	//if(get_button_state(4)) //PWR button
	if(BUTTON_SET_ON) //simpler
	{
		button_PWR_state++;

		if(button_PWR_state >= PWR_BUTTON_THRESHOLD_2)
		{
			return 5;
		}
		else if(button_PWR_state == PWR_BUTTON_THRESHOLD_3)
		{
			return 6;
		}
	}
	else
	{
		if(button_PWR_state >= PWR_BUTTON_THRESHOLD_1)
		{
			return 4;
		}

		button_PWR_state = 0;
	}

	return -1;
}

int button_hold_counter = 0, button_hold_state = 0, button_set_release = 0, set_mode_counter = 0;
#define SET_MODE_TIMEOUT 35 //35 x 100ms (interval at which this fn is called), gives 3.5 seconds timeout

void buttons_controls_during_play()
{
	if(!mode_set_flag) //basic options, accessible directly without pressing SET first
	{
		LED_SIG_OFF;

		if(!BUTTON_U1_ON && !BUTTON_U2_ON && !BUTTON_U3_ON && !BUTTON_U4_ON)
		{
			button_hold_counter = 0;
			button_hold_state = 0;
		}
		else
		{
			button_hold_counter++;
		}

		if(BUTTON_U1_ON) //button 1 turns volume down
		{
			#ifdef VOLUME_CONTROL_BY_MST
				codec_volume--;
				if(codec_volume < CODEC_VOLUME_MIN)
				{
					codec_volume = CODEC_VOLUME_MIN;
				}
				else if(button_hold_counter%2)
				{
					LED_SIG_ON;
				}

				CodecCommandBuffer[0] = CODEC_MAP_MASTER_A_VOL; //0x20
				CodecCommandBuffer[1] = codec_volume;
				queue_codec_ctrl(CodecCommandBuffer, 2);
				CodecCommandBuffer[0] = CODEC_MAP_MASTER_B_VOL; //0x21
				queue_codec_ctrl(CodecCommandBuffer, 2);
			#endif

				#ifdef VOLUME_CONTROL_BY_HP
				hp_volume--;
				if(hp_volume < HP_VOLUME_MIN)
				{
					hp_volume = HP_VOLUME_MIN;
				}
				else if(button_hold_counter%2)
				{
					LED_SIG_ON;
				}
				CodecCommandBuffer[0] = CODEC_MAP_HP_A_VOL; //0x22
				//CodecCommandBuffer[0] = CODEC_MAP_SPEAK_A_VOL; //0x24
				CodecCommandBuffer[1] = hp_volume;
				queue_codec_ctrl(CodecCommandBuffer, 2);
				CodecCommandBuffer[0] = CODEC_MAP_HP_B_VOL; //0x23
				//CodecCommandBuffer[0] = CODEC_MAP_SPEAK_B_VOL; //0x25
				queue_codec_ctrl(CodecCommandBuffer, 2);
				EEPROM_StoreSettings_B(SETTINGS_MAIN_VOLUME,hp_volume-HP_VOLUME_DEFAULT);
			#endif

			//computed_sample_dynamic_limit -= 100;
		}
		else if(BUTTON_U2_ON) //button 2 turns volume up
		{
			#ifdef VOLUME_CONTROL_BY_MST
				codec_volume++;
				if(codec_volume > CODEC_VOLUME_MAX)
				{
					codec_volume = CODEC_VOLUME_MAX;
				}
				else if(button_hold_counter%2)
				{
					LED_SIG_ON;
				}
				CodecCommandBuffer[0] = CODEC_MAP_MASTER_A_VOL; //0x20
				CodecCommandBuffer[1] = codec_volume;
				queue_codec_ctrl(CodecCommandBuffer, 2);
				CodecCommandBuffer[0] = CODEC_MAP_MASTER_B_VOL; //0x21
				queue_codec_ctrl(CodecCommandBuffer, 2);
            #endif

			#ifdef VOLUME_CONTROL_BY_HP
				hp_volume++;
				if(hp_volume > HP_VOLUME_MAX)
				{
					hp_volume = HP_VOLUME_MAX;
				}
				else if(button_hold_counter%2)
				{
					LED_SIG_ON;
				}
				CodecCommandBuffer[0] = CODEC_MAP_HP_A_VOL; //0x22
				//CodecCommandBuffer[0] = CODEC_MAP_SPEAK_A_VOL; //0x24
				CodecCommandBuffer[1] = hp_volume;
				queue_codec_ctrl(CodecCommandBuffer, 2);
				CodecCommandBuffer[0] = CODEC_MAP_HP_B_VOL; //0x23
				//CodecCommandBuffer[0] = CODEC_MAP_SPEAK_B_VOL; //0x25
				queue_codec_ctrl(CodecCommandBuffer, 2);
				EEPROM_StoreSettings_B(SETTINGS_MAIN_VOLUME,hp_volume-HP_VOLUME_DEFAULT);
			#endif

			//computed_sample_dynamic_limit += 100;
		}
		else if(BUTTON_U3_ON) //button 3 cycles through various echo loop lengths
		{
			if(!button_hold_state)
			{
				echo_dynamic_loop_current_step++;
				if(echo_dynamic_loop_current_step==ECHO_DYNAMIC_LOOP_STEPS)
				{
					echo_dynamic_loop_current_step = 0;
				}

				//if low memory (e.g. the song is too long), there is shorter buffer allocated
				if(echo_buffer_low_memory)
				{
					if(echo_dynamic_loop_current_step == ECHO_BUFFER_LOW_MEM_SKIP)
					{
						echo_dynamic_loop_current_step++;
					}
				}

				echo_dynamic_loop_length = get_echo_length(echo_dynamic_loop_current_step);

				if(echo_dynamic_loop_length==0)
				{
					PROG_add_echo = false; //turn echo off altogether
				}
				else
				{
					PROG_add_echo = true; //turn echo back on
				}
				EEPROM_StoreSettings_B(SETTINGS_DELAY_LENGTH,echo_dynamic_loop_current_step);

				LED_SIG_ON;
				button_hold_state++;
			}
		}
		else if(BUTTON_U4_ON) //button 4 switches between inputs
		{
			if(!button_hold_state)
			{
				input_mux_current_step++;
				if(input_mux_current_step==INPUT_MUX_STEPS)
				{
					input_mux_current_step = 0;
				}

				ADC_set_input_multiplexer(input_mux_current_step);
				EEPROM_StoreSettings_B(SETTINGS_INPUT_SELECT,input_mux_current_step);

				LED_SIG_ON;
				button_hold_state++;
			}
		}

	} else { //if mode_set_flag

		if(BUTTON_U1_ON || BUTTON_U2_ON || BUTTON_U3_ON || BUTTON_U4_ON)
		{
			set_mode_counter = 0;

			if(mode_set_flag==1) //EQ controls
			{
				if(BUTTON_U1_ON) //treble down
				{
					codec_treble++; //meaning of the value is reversed

					if(codec_treble > CODEC_TREBLE_MIN)
					{
						codec_treble = CODEC_TREBLE_MIN;
					}
					display_volume_level_indicator_i(codec_treble, CODEC_TREBLE_MIN, CODEC_TREBLE_MAX);

					CodecCommandBuffer[0] = CODEC_MAP_TONE_CTRL; //0x1f
					CodecCommandBuffer[1] = codec_bass | (codec_treble << 4);
					queue_codec_ctrl(CodecCommandBuffer, 2);
					EEPROM_StoreSettings_B(SETTINGS_EQ_TREBLE,codec_treble-CODEC_TREBLE_DEFAULT);
				}
				else if(BUTTON_U2_ON) //treble up
				{
					codec_treble--; //meaning of the value is reversed

					if(codec_treble < CODEC_TREBLE_MAX)
					{
						codec_treble = CODEC_TREBLE_MAX;
					}
					display_volume_level_indicator_i(codec_treble, CODEC_TREBLE_MIN, CODEC_TREBLE_MAX);

					CodecCommandBuffer[0] = CODEC_MAP_TONE_CTRL; //0x1f
					CodecCommandBuffer[1] = codec_bass | (codec_treble << 4);
					queue_codec_ctrl(CodecCommandBuffer, 2);
					EEPROM_StoreSettings_B(SETTINGS_EQ_TREBLE,codec_treble-CODEC_TREBLE_DEFAULT);
				}
				else if(BUTTON_U3_ON) //bass down
				{
					codec_bass++; //meaning of the value is reversed

					if(codec_bass > CODEC_BASS_MIN)
					{
						codec_bass = CODEC_BASS_MIN;
					}
					display_volume_level_indicator_i(codec_bass, CODEC_BASS_MIN, CODEC_BASS_MAX);

					CodecCommandBuffer[0] = CODEC_MAP_TONE_CTRL; //0x1f
					CodecCommandBuffer[1] = codec_bass | (codec_treble << 4);
					queue_codec_ctrl(CodecCommandBuffer, 2);
					EEPROM_StoreSettings_B(SETTINGS_EQ_BASS,codec_bass-CODEC_BASS_DEFAULT);
				}
				else if(BUTTON_U4_ON) //bass up
				{
					codec_bass--; //meaning of the value is reversed

					if(codec_bass < CODEC_BASS_MAX)
					{
						codec_bass = CODEC_BASS_MAX;
					}
					display_volume_level_indicator_i(codec_bass, CODEC_BASS_MIN, CODEC_BASS_MAX);

					CodecCommandBuffer[0] = CODEC_MAP_TONE_CTRL; //0x1f
					CodecCommandBuffer[1] = codec_bass | (codec_treble << 4);
					queue_codec_ctrl(CodecCommandBuffer, 2);
					EEPROM_StoreSettings_B(SETTINGS_EQ_BASS,codec_bass-CODEC_BASS_DEFAULT);
				}
			}
			else if(mode_set_flag==2) //codec volume, mixing volume
			{
				if(BUTTON_U1_ON) //button 1 turns OpAmp mixing volume down
				{
					OpAmp_ADC12_signal_conversion_factor -= OPAMP_ADC12_CONVERSION_FACTOR_STEP;
					if(OpAmp_ADC12_signal_conversion_factor < OPAMP_ADC12_CONVERSION_FACTOR_MIN)
					{
						OpAmp_ADC12_signal_conversion_factor = OPAMP_ADC12_CONVERSION_FACTOR_MIN;
					}
					display_volume_level_indicator_f(OpAmp_ADC12_signal_conversion_factor, OPAMP_ADC12_CONVERSION_FACTOR_MIN, OPAMP_ADC12_CONVERSION_FACTOR_MAX);
					EEPROM_StoreSettings_DW(SETTINGS_INPUT_GAIN,((uint32_t*)(&OpAmp_ADC12_signal_conversion_factor))[0]);
				}
				else if(BUTTON_U2_ON) //button 2 turns OpAmp mixing volume up
				{
					OpAmp_ADC12_signal_conversion_factor += OPAMP_ADC12_CONVERSION_FACTOR_STEP;
					if(OpAmp_ADC12_signal_conversion_factor > OPAMP_ADC12_CONVERSION_FACTOR_MAX)
					{
						OpAmp_ADC12_signal_conversion_factor = OPAMP_ADC12_CONVERSION_FACTOR_MAX;
					}
					display_volume_level_indicator_f(OpAmp_ADC12_signal_conversion_factor, OPAMP_ADC12_CONVERSION_FACTOR_MIN, OPAMP_ADC12_CONVERSION_FACTOR_MAX);
					EEPROM_StoreSettings_DW(SETTINGS_INPUT_GAIN,((uint32_t*)(&OpAmp_ADC12_signal_conversion_factor))[0]);
				}
				else if(BUTTON_U3_ON) //button 3 turns codec volume down
				{
					codec_volume--;
					if((int8_t)codec_volume < (int8_t)CODEC_VOLUME_MIN)
					{
						codec_volume = CODEC_VOLUME_MIN;
					}
					display_volume_level_indicator_i(codec_volume, CODEC_VOLUME_MIN, CODEC_VOLUME_MAX);

					CodecCommandBuffer[0] = CODEC_MAP_MASTER_A_VOL; //0x20
					CodecCommandBuffer[1] = codec_volume;
					queue_codec_ctrl(CodecCommandBuffer, 2);
					CodecCommandBuffer[0] = CODEC_MAP_MASTER_B_VOL; //0x21
					queue_codec_ctrl(CodecCommandBuffer, 2);
					EEPROM_StoreSettings_B(SETTINGS_CODEC_VOLUME,codec_volume-CODEC_VOLUME_DEFAULT);
				}
				else if(BUTTON_U4_ON) //button 4 turns codec volume up
				{
					codec_volume++;
					if((int8_t)codec_volume > (int8_t)CODEC_VOLUME_MAX)
					{
						codec_volume = CODEC_VOLUME_MAX;
					}
					display_volume_level_indicator_i(codec_volume, CODEC_VOLUME_MIN, CODEC_VOLUME_MAX);

					CodecCommandBuffer[0] = CODEC_MAP_MASTER_A_VOL; //0x20
					CodecCommandBuffer[1] = codec_volume;
					queue_codec_ctrl(CodecCommandBuffer, 2);
					CodecCommandBuffer[0] = CODEC_MAP_MASTER_B_VOL; //0x21
					queue_codec_ctrl(CodecCommandBuffer, 2);
					EEPROM_StoreSettings_B(SETTINGS_CODEC_VOLUME,codec_volume-CODEC_VOLUME_DEFAULT);
				}
			}
			else if(mode_set_flag==3) //voices and timings
			{
				if(BUTTON_U1_ON) //button 1 turns tempo down
				{
					if(!button_hold_state)
					{
						LED_SIG_ON;
						if(tempo_bpm>TEMPO_BPM_MIN)
						{
							tempo_bpm -= TEMPO_BPM_STEP;
							EEPROM_StoreSettings_W(SETTINGS_TEMPO_BPM,tempo_bpm-TEMPO_BPM_DEFAULT);
							TEMPO_BY_SAMPLE = get_tempo_by_BPM(tempo_bpm);
							MELODY_BY_SAMPLE = TEMPO_BY_SAMPLE / 4;
							display_tempo_indicator(tempo_bpm);
						}
						button_hold_state++;
					}
				}
				else if(BUTTON_U2_ON) //button 2 turns tempo up
				{
					if(!button_hold_state)
					{
						LED_SIG_ON;
						if(tempo_bpm<TEMPO_BPM_MAX)
						{
							tempo_bpm += TEMPO_BPM_STEP;
							EEPROM_StoreSettings_W(SETTINGS_TEMPO_BPM,tempo_bpm-TEMPO_BPM_DEFAULT);
							TEMPO_BY_SAMPLE = get_tempo_by_BPM(tempo_bpm);
							MELODY_BY_SAMPLE = TEMPO_BY_SAMPLE / 4;
							display_tempo_indicator(tempo_bpm);
						}
						button_hold_state++;
					}
				}
				else if(BUTTON_U3_ON) //button 3 sets tempo to default value
				{
					if(!button_hold_state)
					{
						LED_SIG_ON;
						tempo_bpm = TEMPO_BPM_DEFAULT;
						EEPROM_StoreSettings_W(SETTINGS_TEMPO_BPM,tempo_bpm-TEMPO_BPM_DEFAULT);
						TEMPO_BY_SAMPLE = get_tempo_by_BPM(tempo_bpm);
						MELODY_BY_SAMPLE = TEMPO_BY_SAMPLE / 4;
						display_tempo_indicator(tempo_bpm);
						button_hold_state++;
					}
				}
				else if(BUTTON_U4_ON) //button 4 selects whether melody is played by filter or sampled sound
				{
					if(!button_hold_state)
					{
						PROG_wavetable_sample = !PROG_wavetable_sample;
						EEPROM_StoreSettings_B(SETTINGS_WAVETABLE_SAMPLE,PROG_wavetable_sample?1:0);
						if(PROG_wavetable_sample)
						{
							ACTIVE_FILTERS_PAIRS = FILTER_PAIRS - 1; //get more computation power by disabling one filter pair
						}
						else
						{
							ACTIVE_FILTERS_PAIRS = FILTER_PAIRS; //all filter pairs active
						}
						button_hold_state++;
					}
				}
			}
			else if(mode_set_flag==4) //more settings
			{
				if(BUTTON_U1_ON) //button 1 allows mixing the original signal in
				{
					if(!button_hold_state)
					{
						PROG_mix_input_signal = !PROG_mix_input_signal;
						button_hold_state++;
					}
				}
				else if(BUTTON_U2_ON) //button 2
				{
				}
				else if(BUTTON_U3_ON) //button 3
				{
				}
				else if(BUTTON_U4_ON) //button 4
				{
				}
			}
		}
		else
		{
			button_hold_state = 0;

			if(++set_mode_counter==SET_MODE_TIMEOUT)
			{
				mode_set_flag = 0;
				LED_SIG_OFF;

				//turn mics back on, if should be in this channel
				if(PROG_audio_input_microphones && input_mux_current_step==0)
				{
					PROG_add_OpAmp_ADC12_signal = true;
				}

				//reset all red ad orange LEDs as they might have been used by indicators
				LED_R8_all_OFF();
				LED_O4_all_OFF();
			}
		}
	}

	if(BUTTON_SET_ON)
	{
		if(button_set_release)
		{
			if(!mode_set_flag)
			{
				LED_O4_all_OFF();
				LED_R8_all_OFF();
			}

			LED_SIG_ON;
			set_mode_counter = 0;
			button_set_release = 0;

			mode_set_flag++;
			if(mode_set_flag==5)
			{
				mode_set_flag = 1;
			}

			if(mode_set_flag==1)
			{
				LED_O4_set_byte(1);

				//turn off mics to not get sound of buttons in all the time
				if(PROG_audio_input_microphones && input_mux_current_step==0)
				{
					PROG_add_OpAmp_ADC12_signal = false;
				}
			}
			else if(mode_set_flag==2)
			{
				LED_O4_set_byte(2);
			}
			else if(mode_set_flag==3)
			{
				LED_O4_set_byte(4);
			}
			else if(mode_set_flag==4)
			{
				LED_O4_set_byte(8);
			}
		}
	}
	else
	{
		button_set_release = 1;
	}
}
/*
int wait_for_sensor_click()
{
	int result = -1;
	int i;
	int holding = 0;
	int found;

	while(1)
	{
		if(ADC_process_sensors()==1)
		{
			ADC_last_result[2] = -ADC_last_result[2];
			ADC_last_result[3] = -ADC_last_result[3];

			found = -1;
			for(i=0;i<ADC_SENSORS;i++)
			{
				if(ADC_last_result[i] > ADC_SENSOR_THRESHOLD)
				{
					found = i;
				}
			}

			if(found > -1)
			{
				holding++;
				if(holding > 3)
				{
					return found+1;
				}
			}
			else
			{
				holding = 0;
			}

			CLEAR_ADC_RESULT_RDY_FLAG;
		}
		Delay(ADC_SENSOR_CLICK_LOOP_DELAY);
	}

	return result;
}

int wait_for_gesture()
{
	int result = -1;
	int started = -1;
	int timeout;
	int found;
	int diff12, diff34, sign12, sign34;
	int init_delay = ADC_SENSOR_GESTURE_INIT_DELAY;

	while(1)
	{
		Delay(ADC_SENSOR_GESTURE_LOOP_DELAY);
		if(init_delay>0)
		{
			init_delay--;
			ADC_process_sensors();
			continue;
		}
		if(ADC_process_sensors()==1)
		{
			ADC_last_result[2] = -ADC_last_result[2];
			ADC_last_result[3] = -ADC_last_result[3];

			diff12 = abs(ADC_last_result[0] - ADC_last_result[1]);
			diff34 = abs(ADC_last_result[2] - ADC_last_result[3]);
			sign12 = (ADC_last_result[0] > ADC_last_result[1]) ? 1 : 2;
			sign34 = (ADC_last_result[2] > ADC_last_result[3]) ? 3 : 4;

			found = -1;

			if(diff12 > ADC_SENSOR_THRESHOLD_DIFF)
			{
				LED_O4_0_ON;

				if(sign12==1)
				{
					LED_R8_0_ON;
				}
				else if(sign12==2)
				{
					LED_R8_1_ON;
				}

				if(started == -1)
				{
					started = sign12;
				}
				else if(started <= 2 && started != sign12)
				{
					LED_O4_1_ON;
					return sign12;
				}
				found = 12;
			}

			if(diff34 > ADC_SENSOR_THRESHOLD_DIFF)
			{
				LED_O4_3_ON;

				if(sign34==3)
				{
					LED_R8_6_ON;
				}
				else if(sign34==4)
				{
					LED_R8_7_ON;
				}

				if(started == -1)
				{
					started = sign34;
				}
				else if(started >= 3 && started != sign34)
				{
					LED_O4_2_ON;
					return sign34;
				}
				found = 34;
			}

			if(found > -1)
			{
				timeout = 4;
			}

			if(timeout > 0)
			{
				timeout--;

				if(timeout==0)
				{
					LED_O4_all_OFF();
					LED_R8_all_OFF();
					started = -1;
				}
			}

			CLEAR_ADC_RESULT_RDY_FLAG;
		}
	}

	return result;
}
*/

void l33tsp34k(char *buffer, int buf_length)
{
	char *buf_ptr = buffer;
	while (buf_ptr < buffer + buf_length)
	{
		if (buf_ptr[0]=='o' || buf_ptr[0]=='O') {buf_ptr[0]='0';}
		if (buf_ptr[0]=='i' || buf_ptr[0]=='I') {buf_ptr[0]='1';}
		if (buf_ptr[0]=='z' || buf_ptr[0]=='Z') {buf_ptr[0]='2';}
		if (buf_ptr[0]=='e' || buf_ptr[0]=='E') {buf_ptr[0]='3';}
		if (buf_ptr[0]=='a' || buf_ptr[0]=='A') {buf_ptr[0]='4';}
		if (buf_ptr[0]=='s' || buf_ptr[0]=='S') {buf_ptr[0]='5';}
		buf_ptr++;
	}
}

void string_to_14(char *buffer, int buf_length)
{
	char *buf_ptr = buffer;
	while (buf_ptr < buffer + buf_length)
	{
		if (buf_ptr[0]=='5' || buf_ptr[0]=='a') {buf_ptr[0]='1';}
		if (buf_ptr[0]=='6' || buf_ptr[0]=='b') {buf_ptr[0]='2';}
		if (buf_ptr[0]=='7' || buf_ptr[0]=='c') {buf_ptr[0]='3';}
		if (buf_ptr[0]=='8' || buf_ptr[0]=='d') {buf_ptr[0]='4';}
		if (buf_ptr[0]=='9' || buf_ptr[0]=='e') {buf_ptr[0]='1';}
		if (buf_ptr[0]=='0' || buf_ptr[0]=='f') {buf_ptr[0]='2';}
		buf_ptr++;
	}
}

void sha1_to_hex(char *code_sha1_hex, uint8_t *code_sha1)
{
	char hex_char[3];

	for (int i=0;i<20;i++)
	{
		//sprintf(code_sha1_hex+(2*i), "%02x", code_sha1[i]); //"%02x" modifier won't work here

		sprintf(hex_char, "%x", code_sha1[i]);

		if (strlen(hex_char)==2)
		{
			strncpy(code_sha1_hex+(2*i), hex_char, 2);
		}
		else if (strlen(hex_char)==1)
		{
			(code_sha1_hex+(2*i))[0] = '0';
			(code_sha1_hex+(2*i))[1] = hex_char[0];
		}
		else
		{
			strncpy(code_sha1_hex+(2*i), "00", 2);
		}
	}
	code_sha1_hex[40] = 0;
}

int preview_composition(char *data)
{
	if(0!=strncmp(data,"SONG=",5))
	{
		return 0;
	}
	data += 5;
	char *end = strstr(data,"&MELODY=");
	if(!end) //if end delimiter not found
	{
		return 0; //data corrupted?
	}

	if(progression_str!=NULL)
	{
		free(progression_str);
	}
	if(melody_str!=NULL)
	{
		free(melody_str);
	}

	int size = (int)end - (int)data;
	progression_str = (char*)malloc(size+2);
	strncpy(progression_str,data,size);
	progression_str[size] = 0;

	end += 8;
	char *settings = strstr(end,"&SET=");
	if(settings) //if end delimiter found
	{
		size = (int)settings - (int)end;
		settings_str = malloc(strlen(settings)+2-5);
		strcpy(settings_str,settings+5);
	}
	else
	{
		size = strlen(end);
		settings_str = NULL;
	}

	if(size) //if any melody present at all
	{
		melody_str = (char*)malloc(size+2);
		strcpy(melody_str,end);
	}

	return 1;
}

int verify_activation_unlock_code(char *code)
{
	if(strlen(code)!=16)
	{
		return 0;
	}

	uint32_t uid_w0 = ((uint32_t*)UID_BASE_ADDRESS)[0];
	uint32_t uid_w1 = ((uint32_t*)UID_BASE_ADDRESS)[1];
	uint32_t uid_w2 = ((uint32_t*)UID_BASE_ADDRESS)[2];

	char *buf = (char*)malloc(50);
	sprintf(buf, "unl0ck_c0d3(%x-%x-%x)", uid_w0, uid_w1, uid_w2);

	uint8_t code_sha1[20];

	SHA1_CTX sha;
	SHA1Init(&sha);
	SHA1Update(&sha, (uint8_t *)buf, strlen(buf));
	SHA1Final(code_sha1, &sha);

	sha1_to_hex(buf, code_sha1);
	string_to_14(buf, 16);
	buf[17] = 0;

	int result = strncmp(buf, code, 16);
	free(buf);

	if(result!=0)
	{
		return 0;
	}

	return 1;
}

void send_activation_info_USART()
{
	GPIO_Init_USART1(115200);//9600); //revert USART1 Rx to input (set both Rx and Tx pins to AF)

	while(1)
	{
		LED_RDY_ON;
		Delay(100);

		LED_SIG_ON;
		uint32_t uid_w0 = ((uint32_t*)UID_BASE_ADDRESS)[0];
		uint32_t uid_w1 = ((uint32_t*)UID_BASE_ADDRESS)[1];
		uint32_t uid_w2 = ((uint32_t*)UID_BASE_ADDRESS)[2];
		char *buf = (char*)malloc(32);
		sprintf(buf, "[%x-%x-%x]\n\r", uid_w0, uid_w1, uid_w2);
		USART_puts(USART1, buf);
		free(buf);
		Delay(20);
		LED_SIG_OFF;
		Delay(100);

		LED_SIG_ON;
		USART_puts(USART1, (char*)BINARY_ID);
		USART_puts(USART1, "\n\r");
		Delay(20);
		LED_SIG_OFF;
		Delay(100);

		LED_SIG_ON;
		USART_puts(USART1, (char*)BINARY_HASH);
		USART_puts(USART1, "\n\r");
		Delay(20);
		LED_SIG_OFF;
		Delay(100);

		LED_SIG_ON;
		USART_puts(USART1, (char*)UID_HASH);
		USART_puts(USART1, "\n\r-------------------\n\r\n\r");
		Delay(20);
		LED_SIG_OFF;
		Delay(100);

		LED_RDY_OFF;
		Delay(500);
	}
}
