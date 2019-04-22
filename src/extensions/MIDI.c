/*
 * MIDI.c
 *
 *  Created on: Jan 9, 2017
 *      Author: mayo
 *
 *  This file is part of the Gecho Loopsynth Firmware Development Framework.
 *  It can be used within the terms of CC-BY-NC-SA license.
 *  It must not be distributed separately.
 *
 *  Find more information at: http://gechologic.com/gechologists/
 */

#include "MIDI.h"
#include <hw/gpio.h>
#include <hw/leds.h>

int MIDI_notes_to_freq[128];

void MIDI_out_test()
{
	while(1)
	{
		for(int i=36;i<=72;i++)
		{
			USART_SendData(USART3, 144); //new note
			Delay(1);
			USART_SendData(USART3,i); //note value
			Delay(1);
			USART_SendData(USART3, 64); //velocity
			Delay(1900);

			USART_SendData(USART3, 128); //new note
			Delay(1);
			USART_SendData(USART3, i); //note value
			Delay(1);
			USART_SendData(USART3, 64); //velocity
			Delay(96);
		}
	}
}

void MIDI_record_playback_test()
{
	int MIDI_new_note = 0;
	//int MIDI_new_volume = 0;
	//int MIDI_new_note_freq = 0;
	//int MIDI_notes_to_freq[100];

	while(1)
	{
		if(USART_GetFlagStatus(USART3, USART_FLAG_RXNE))
		{
			// Read received char
			int data = USART_ReceiveData(USART3);
			if(data!=248 && data!=254)
			{
				if(data!=144)
				{
					if(MIDI_new_note==0)
					{
						MIDI_new_note = data;
						LED_R8_set_byte(data);
						LED_B5_1_ON;
						Delay(20);
						LED_B5_1_OFF;
					}
				}
			}
			else
			{
				MIDI_new_note = 0;
				LED_B5_0_ON;
				Delay(20);
				LED_B5_0_OFF;
			}
		}
	}
}

void MIDI_direct_signals_test()
{
	while (1)
	{
		if (USART3_RX)
		{
			LED_O4_2_ON;
		}
		else
		{
			LED_O4_2_OFF;
		}

		if (BUTTON_U1_ON)
		{
			LED_O4_0_ON; //USART3_TX_ON;
		}
		else
		{
			LED_O4_0_OFF; //USART3_TX_OFF;
		}

		Delay(50);
	}
}
