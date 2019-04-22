/*
 * DCO_synth.cpp
 *
 *  Created on: 11 May 2017
 *      Author: mayo
 *
 * Based on "The Tiny-TS Touch Synthesizer" by Janost 2016, Sweden
 * https://janostman.wordpress.com/the-tiny-ts-diy-touch-synthesizer/
 *
 */

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "DCO_Synth.h"

const uint8_t DCO_Synth::sinetable256[256] = {
	0,0,0,0,1,1,1,2,2,3,4,5,5,6,7,9,10,11,12,14,15,16,18,20,21,23,25,27,29,31,
	33,35,37,39,42,44,46,49,51,54,56,59,62,64,67,70,73,76,78,81,84,87,90,93,96,99,102,105,108,111,115,118,121,124,
	127,130,133,136,139,143,146,149,152,155,158,161,164,167,170,173,176,178,181,184,187,190,192,195,198,200,203,205,208,210,212,215,217,219,221,223,225,227,229,231,233,234,236,238,239,240,
	242,243,244,245,247,248,249,249,250,251,252,252,253,253,253,254,254,254,254,254,254,254,253,253,253,252,252,251,250,249,249,248,247,245,244,243,242,240,239,238,236,234,233,231,229,227,225,223,
	221,219,217,215,212,210,208,205,203,200,198,195,192,190,187,184,181,178,176,173,170,167,164,161,158,155,152,149,146,143,139,136,133,130,127,124,121,118,115,111,108,105,102,99,96,93,90,87,84,81,78,
	76,73,70,67,64,62,59,56,54,51,49,46,44,42,39,37,35,33,31,29,27,25,23,21,20,18,16,15,14,12,11,10,9,7,6,5,5,4,3,2,2,1,1,1,0,0,0
};

uint8_t DCO_Synth::*sawtable256;
uint8_t DCO_Synth::*squaretable256;

DCO_Synth::DCO_Synth()
{
	envtick = 549;
	DOUBLE = 0;
	resonant_peak_mod_volume = 0;

	//-------- Synth parameters --------------
	//volatile uint8_t VCA=255;         //VCA level 0-255
	ATTACK = 1;        // ENV Attack rate 0-255
	RELEASE = 1;       // ENV Release rate 0-255
	//ENVELOPE=0;      // ENV Shape
	//TRIG=0;          //MIDItrig 1=note ON
	//-----------------------------------------

	FREQ = 0;         //DCO pitch

	DCO_BLOCKS_ACTIVE = DCO_BLOCKS_MAX;

	DCO_mode = 0;
	SET_mode = 0;
	button_set_held = 0;
	MIDI_note_set = 0;//, MIDI_note_on = 0;

	use_table256 = (uint8_t*)sinetable256;

	sawtable256 = (uint8_t*)malloc(256*sizeof(uint8_t));
	squaretable256 = (uint8_t*)malloc(256*sizeof(uint8_t));

	for(int i=0;i<256;i++)
	{
		sawtable256[i] = i;
		squaretable256[i] = (i<128)?0:255;
	}
}

void DCO_Synth::v2_init()
{
	init_codec();

	//-----------------------------------------------------------------------------------
	// Gecho variables and peripherals init
	//-----------------------------------------------------------------------------------

	GPIO_LEDs_Buttons_Reset();
	//ADC_DeInit(); //reset ADC module
	ADC_configure_SENSORS(ADCConvertedValues);

	//ADC_configure_CV_GATE(CV_GATE_PC0_PB0); //PC0 stays the same as if configured for IRS1
	//int calibration_success = Calibrate_CV(); //if successful, array with results is stored in *calibrated_cv_values

	//-----------------------------------------------------------------------------------
	// Tiny-TS variables init
	//-----------------------------------------------------------------------------------

	sample_i16 = 128;
	FREQ=440;//MIDI2FREQ(key+12+(COARSEPITCH/21.3125));
	DOUBLE=12;
	PHASEamt=9;
	ENVamt=123;
	ATTACK=64;//ATTrates[12];
	RELEASE=64;//RELrates[6];
	DCO_BLOCKS_ACTIVE = 24;
	resonant_peak_mod_volume = 64;

	//if(mode==1)
	//{
		//load the default sequence
		//char *seq = "a3a2a3a2c3a2d3c3a2a2a3a2g3c3f#3e3";
		//parse_notes(seq, sequence, NULL);
		//SEQUENCER_THRESHOLD = 200;
	//}

	//echo loop
	init_echo_buffer();

	ECHO_MIXING_GAIN_MUL = 1; //amount of signal to feed back to echo loop, expressed as a fragment
    ECHO_MIXING_GAIN_DIV = 4; //e.g. if MUL=2 and DIV=3, it means 2/3 of signal is mixed in

	for(int i=0;i<128;i++)
	{
		MIDI_notes_to_freq[i] = NOTE_FREQ_A4 * pow(HALFTONE_STEP_COEF, i - 33);
	}
}

void DCO_Synth::v2_play_loop()
{
	while(1)
	{
		//-------------------- DCO block ------------------------------------------
		DCO_output[0] = 0;
		DCO_output[1] = 0;

		for (int b=0; b<DCO_BLOCKS_ACTIVE; b++)
		{
			phacc[b] += FREQ + (b * DOUBLE);
		    if (phacc[b] & 0x8000) {
				phacc[b] &= 0x7FFF;
				otone1[b] += 2;
				pdacc[b] += PDmod;
		    }
		    if (!otone1[b]) pdacc[b] = 0;
		    otone2[b] = (pdacc[b] >> 3) & 255;
		    uint8_t env = (255-otone1[b]);
		    DCO_output[b%2] += (use_table256[otone2[b]] + use_table256[(otone2[b] + (127 - env)) & 255]);
		}

		if (!(envtick--)) {
			envtick=582;

			//--------- Resonant Peak ENV modulation ----------------------------------------

			//if(!DCO_mode)
			{
				PDmod=((resonant_peak_mod_volume*ENVamt)>>8)+PHASEamt;
				if (PDmod>255) PDmod=255;
			}
		}

		//-------------------------------------------------------------------------
		// send sample to codec
		//-------------------------------------------------------------------------

		sample_i16 = (int16_t)DCO_output[0];
		sample_i16 += echo_buffer[echo_buffer_ptr];

		while (!SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE));
		SPI_I2S_SendData(CODEC_I2S, sample_i16);

		sample_i16 = (int16_t)DCO_output[1];

		//wrap in the echo loop
		echo_buffer_ptr0++;
		if(echo_buffer_ptr0 >= echo_dynamic_loop_length)
		{
			echo_buffer_ptr0 = 0;
		}

		echo_buffer_ptr = echo_buffer_ptr0 + 1;
		if(echo_buffer_ptr >= echo_dynamic_loop_length)
		{
			echo_buffer_ptr = 0;
		}

		//add echo from the loop
		sample_i16 += echo_buffer[echo_buffer_ptr];

		//store result to echo, the amount defined by a fragment
		echo_buffer[echo_buffer_ptr0] = sample_i16 * ECHO_MIXING_GAIN_MUL / ECHO_MIXING_GAIN_DIV;

		while (!SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE));
		SPI_I2S_SendData(CODEC_I2S, sample_i16);

		sampleCounter++;

		if (TIMING_BY_SAMPLE_ONE_SECOND_W_CORRECTION)
		{
			sampleCounter = 0;
			seconds++;
		}

		/*
		if ((sampleCounter%(2*I2S_AUDIOFREQ/5)==1234) && PROG_buttons_control_tone_volume) //every 200ms at sample #1234
		{
			buttons_volume_control();
		}

		queue_codec_ctrl_process();
		*/

		if (TIMING_BY_SAMPLE_EVERY_10_MS==0) //100Hz
		{
			if(ADC_process_sensors()==1)
			{
				ADC_last_result[2] = -ADC_last_result[2];
				ADC_last_result[3] = -ADC_last_result[3];
				CLEAR_ADC_RESULT_RDY_FLAG;

				sensors_loop++;

				IR_sensors_LED_indicators(ADC_last_result);

				if(SET_mode==0)
				{
					/*
					if(GATE_PB0)
					{
						//FREQ = ADC_measured_vals[0] * 16; //raw values must be used, not differential (IRS LED on/off)
						FREQ = find_nearest_note_freq(ADC_measured_vals[0]);
						LED_SIG_ON;
					}
					else
					{
						FREQ = 0;
						LED_SIG_OFF;
					}
					*/

					FREQ = ADC_last_result[0] * 4;

					//DOUBLE = ADC_last_result[1] / 8;
					DOUBLE = ADC_last_result[1] / 12;

					//PHASEamt = ADC_last_result[2] / 8;
					PHASEamt = ADC_last_result[2] / 16;

					//if(!DCO_mode)
						ENVamt = ADC_last_result[3] * 16; /// 4;
					//}
					//else
					//{
					//	if(ADC_last_result[3] > 200)
					//	{
					//		//PDmod = ADC_last_result[3] / 2; /// 4;
					//		ENVamt = ADC_last_result[3] * 32; /// 4;
					//	}
					//	else
					//	{
					//		PDmod = 255;
					//	}
					//}
				}
				else
				{
					if(BUTTON_U1_ON)
					{
						FREQ = ADC_last_result[0] * 8;
					}
					if(BUTTON_U2_ON)
					{
						DOUBLE = ADC_last_result[1] / 12;
					}
					if(BUTTON_U3_ON)
					{
						PHASEamt = ADC_last_result[2] / 16;
					}
					if(BUTTON_U4_ON)
					{
						ENVamt = ADC_last_result[3] * 16;
					}

					/*
					if(USART_GetFlagStatus(USART3, USART_FLAG_RXNE))
					{
						// Read received char
						MIDI_data = USART_ReceiveData(USART3);
						if(MIDI_data!=248 && MIDI_data!=254)
						{
							if(MIDI_data!=144 && MIDI_data!=128)
							{
								//FREQ = data * 40;
								FREQ = MIDI_notes_to_freq[MIDI_data];
							}
							if(data==144)
							{
								USART_GetFlagStatus(USART3, USART_FLAG_RXNE);

								data = USART_ReceiveData(USART3);
								if(data!=144)
								{
									FREQ = MIDI_notes_to_freq[data];
								}
							}
							if(data==128)
							{
								USART_GetFlagStatus(USART3, USART_FLAG_RXNE);

								data = USART_ReceiveData(USART3);
								if(data!=128)
								{
									FREQ = MIDI_notes_to_freq[data];
								}
							}

							if(MIDI_note_on==0 && data==144) //note on
							{
								MIDI_note_on = 1;
							}
							else if(MIDI_note_on==1 && MIDI_note_set==0) //this is note value
							{
								if(data!=128) //not a note off
								{
									MIDI_note_set = data;
									FREQ = data * 40;
								}
							}
							else if(MIDI_note_on==1 && MIDI_note_set==1)
							{
								//if(data==128) //note off
								//{
									MIDI_note_set = 0;
									MIDI_note_on = 0;
									//FREQ = 0;

									//while(!USART_GetFlagStatus(USART3, USART_FLAG_RXNE));
									//data = USART_ReceiveData(USART3);
								//}
								//else //this is note velocity. ignore it
								//{
									//data = data + 2;
								//}
							}
						}
					}
					*/
				}
			}

			if(BUTTON_SET_ON)
			{
				if(!button_set_held)
				{
					if(!SET_mode)
					{
						LED_SIG_ON;
						SET_mode = 1;
						button_set_held = 1;
					}
					else
					{
						LED_SIG_OFF;
						SET_mode = 0;
						button_set_held = 1;
					}
				}
			}
			else
			{
				button_set_held = 0;
			}
		}

		/*
    	if (TIMING_BY_SAMPLE_EVERY_100_MS==1234) //10Hz periodically, at sample #1234
    	{
    		buttons_controls_during_play();
    	}

		if(DCO_mode==1)
		{
			//if (TIMING_BY_SAMPLE_EVERY_250_MS==0) //4Hz
			if (TIMING_BY_SAMPLE_EVERY_125_MS==0) //8Hz
			{
				if(ADC_last_result[0] < SEQUENCER_THRESHOLD)
				{
					FREQ = sequence[sequencer_head];
					if (++sequencer_head == SEQUENCE_LENGTH)
					{
						sequencer_head = 0;
					}
				}
			}
		}
		*/
	}
}

//---------------------------------------------------------------------------------------------------------------------
// Old, simpler version (no controls during play, apart from IR sensors) but keeping here as it is fun to play
// Some leftovers from the original Tiny-TS code
//---------------------------------------------------------------------------------------------------------------------

//mixed sound output variable
//int16_t OutSample = 128;
//uint8_t GATEIO=0; //0=Gate is output, 1=Gate is input
//uint16_t COARSEPITCH;
//uint8_t ENVsmoothing;
//uint8_t envcnt=10;
//int16_t ENV;
//uint8_t k=0;
//uint8_t z;
//int8_t MUX=0;

void DCO_Synth::v1_init()//int mode)
{
	init_codec();

	//-----------------------------------------------------------------------------------
	// Gecho variables and peripherals init
	//-----------------------------------------------------------------------------------

	GPIO_LEDs_Buttons_Reset();
	//program_settings_reset();

	//ADC_DeInit(); //reset ADC module
	ADC_configure_SENSORS(ADCConvertedValues);

	//ADC_configure_CV_GATE(CV_GATE_PC0_PB0); //PC0 stays the same as if configured for IRS1
	//int calibration_success = Calibrate_CV(); //if successful, array with results is stored in *calibrated_cv_values

	//-----------------------------------------------------------------------------------
	// Tiny-TS variables init
	//-----------------------------------------------------------------------------------

	//DCOSynth_setup();
	sample_i16 = 128;

	//test-play a note
	//keydown = 1;
	//key = 5;

	//if (!GATEIO)
		FREQ=440;//MIDI2FREQ(key+12+(COARSEPITCH/21.3125));
	//uint16_t CVout=COARSEPITCH+(key*21.33);
	//OCR1AH = (CVout>>8);
	//OCR1AL = CVout&255;
	//if (!GATEIO) digitalWrite(13,keydown);
	//if (GATEIO) TRIG=digitalRead(13)&1;
	//if (!GATEIO)
		//TRIG=keydown;

	//---------------------------------------------------------------

	//--------------- ADC block -------------------------------------

	//if (!(ADCSRA & 64)) {
	//if (MUX==0) COARSEPITCH=(ADCL+(ADCH<<8));
	//COARSEPITCH=0;//(uint16_t)123;//45;
	//if (MUX==1) DOUBLE=(ADCL+(ADCH<<8))>>2;
	DOUBLE = 12;
	//if (MUX==2) PHASEamt=(((ADCL+(ADCH<<8)))>>3);
	PHASEamt = 9;
	//if (MUX==3) ENVamt=((ADCL+(ADCH<<8))>>3);
	ENVamt = 123;
	//if (MUX==4) ATTACK=ATTrates[15-((ADCL+(ADCH<<8))>>6)];
	ATTACK = 64;//ATTrates[12];
	//if (MUX==5) RELEASE=RELrates[15-((ADCL+(ADCH<<8))>>6)];
	RELEASE = 64;//RELrates[6];
	//MUX++;
	//if (MUX>5) MUX=0;
	//ADMUX = 64 | MUX; //Select MUX
	//sbi(ADCSRA, ADSC); //start next conversation
	//}

		//load the default sequence
		//char *seq = "a3a2a3a2c3a2d3c3a2a2a3a2g3c3f#3e3";
		//parse_notes(seq, sequence, NULL);

		//SEQUENCER_THRESHOLD = 200;

	init_echo_buffer();

	ECHO_MIXING_GAIN_MUL = 1; //amount of signal to feed back to echo loop, expressed as a fragment
    ECHO_MIXING_GAIN_DIV = 4; //e.g. if MUL=2 and DIV=3, it means 2/3 of signal is mixed in

}

void DCO_Synth::v1_play_loop()
{
	while(1)
	{
		//-------------------- DCO block ------------------------------------------

		DCO_output[0] = 0;
		DCO_output[1] = 0;

		for (int b=0; b<DCO_BLOCKS_ACTIVE; b++)
		{
			phacc[b] += FREQ + (b * DOUBLE);
		    if (phacc[b] & 0x8000) {
				phacc[b] &= 0x7FFF;
				otone1[b] += 2;
				pdacc[b] += PDmod;
		    }
		    if (!otone1[b]) pdacc[b] = 0;
		    otone2[b] = (pdacc[b] >> 3) & 255;
		    uint8_t env = (255-otone1[b]);
		    DCO_output[b%2] += (use_table256[otone2[b]] + use_table256[(otone2[b] + (127 - env)) & 255]);
		}
		//---------------------------------------------------------------------------

		//------------------ VCA block ------------------------------------
		/*
			if ((ATTACK==255)&&(TRIG==1)) VCA=255;
		    if (!(envcnt--)) {
				envcnt=20;
				if (VCA<volume) VCA++;
				if (VCA>volume) VCA--;
			}
		*/
		/*
		    #define M(MX, MX1, MX2) \
		      asm volatile ( \
		        "clr r26 \n\t"\
		        "mulsu %B1, %A2 \n\t"\
		        "movw %A0, r0 \n\t"\
		        "mul %A1, %A2 \n\t"\
		        "add %A0, r1 \n\t"\
		        "adc %B0, r26 \n\t"\
		        "clr r1 \n\t"\
		        : \
		        "=&r" (MX) \
		        : \
		        "a" (MX1), \
		        "a" (MX2) \
		        :\
		        "r26"\
		      )
		*/
		//DCO>>=1;
		//DCO>>=2;
		//DCO_output>>=4;
		////M(ENV, (int16_t)DCO, VCA);
		//ENV = (int16_t)DCO * VCA;
		//OCR2A = ENV>>1;

		//OutSample = (int16_t)DCO * VCA;
		//sample_i16 = (int16_t)DCO_output * VCA;

		//sample_i16 = (int16_t)DCO_output;

		//-----------------------------------------------------------------

		if (!(envtick--)) {
			envtick=582;

			//--------------------- ENV block ---------------------------------

			/*
			if ((TRIG==1)&&(volume<255)) {
				volume+=ATTACK;
				if (volume>255) volume=255;
			}
			if ((TRIG==0)&&(volume>0)) {
				volume-=RELEASE;
				if (volume<0) volume=0;
			}
			*/
		    //-----------------------------------------------------------------

		    //--------- Resonant Peak ENV modulation ----------------------------------------

		    //if(!DCO_mode)
		    {
		    	PDmod=((resonant_peak_mod_volume*ENVamt)>>8)+PHASEamt;
		    	if (PDmod>255) PDmod=255;
		    	//if (PDmod>8192) PDmod=8192;
		    }
		    //-----------------------------------------------------------------
		}

		//-------------------------------------------------------------------------
		// send sample to codec
		//-------------------------------------------------------------------------

		sample_i16 = (int16_t)DCO_output[0];
		sample_i16 += echo_buffer[echo_buffer_ptr];

		while (!SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE));
		SPI_I2S_SendData(CODEC_I2S, sample_i16);

		sample_i16 = (int16_t)DCO_output[1];

		//if(PROG_add_echo)
		//{
			//wrap the echo loop
			echo_buffer_ptr0++;
			if(echo_buffer_ptr0 >= echo_dynamic_loop_length)
			{
				echo_buffer_ptr0 = 0;
			}

			echo_buffer_ptr = echo_buffer_ptr0 + 1;
			if(echo_buffer_ptr >= echo_dynamic_loop_length)
			{
				echo_buffer_ptr = 0;
			}

			//add echo from the loop
			sample_i16 += echo_buffer[echo_buffer_ptr];

			//store result to echo, the amount defined by a fragment
			echo_buffer[echo_buffer_ptr0] = sample_i16 * ECHO_MIXING_GAIN_MUL / ECHO_MIXING_GAIN_DIV;
		//}

		while (!SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE));
		SPI_I2S_SendData(CODEC_I2S, sample_i16);

		sampleCounter++;

		if (TIMING_BY_SAMPLE_ONE_SECOND_W_CORRECTION)
		{
			sampleCounter = 0;
			seconds++;
		}

		/*
		if (sampleCounter%(2*I2S_AUDIOFREQ/5)==1234) //every 200ms at sample #1234
		{
			key ++;
			FREQ=MIDI2FREQ(key+12+(COARSEPITCH/21.3125));
			if(key == 256)
			{
				key = 0;
			}
		}
		*/

		/*
		if ((sampleCounter%(2*I2S_AUDIOFREQ/5)==1234) && PROG_buttons_control_tone_volume) //every 200ms at sample #1234
		{
			buttons_volume_control();
		}

		queue_codec_ctrl_process();
		*/

		if (TIMING_BY_SAMPLE_EVERY_10_MS==0) //100Hz
		{
			if(ADC_process_sensors()==1)
			{
				ADC_last_result[2] = -ADC_last_result[2];
				ADC_last_result[3] = -ADC_last_result[3];
				CLEAR_ADC_RESULT_RDY_FLAG;

				sensors_loop++;

				IR_sensors_LED_indicators(ADC_last_result);

				FREQ = ADC_last_result[0] * 4;

				//if (MUX==1) DOUBLE=(ADCL+(ADCH<<8))>>2;

				//DOUBLE = ADC_last_result[1] / 8;
				DOUBLE = ADC_last_result[1] / 12;

				//if (MUX==2) PHASEamt=(((ADCL+(ADCH<<8)))>>3);

				//PHASEamt = ADC_last_result[2] / 8;
				PHASEamt = ADC_last_result[2] / 16;

				//if(!DCO_mode)
				//{
					//if (MUX==3) ENVamt=((ADCL+(ADCH<<8))>>3);
					ENVamt = ADC_last_result[3] * 16; /// 4;
				//}
				//else
				//{
				//	if(ADC_last_result[3] > 200)
				//	{
				//		//PDmod = ADC_last_result[3] / 2; /// 4;
				//		ENVamt = ADC_last_result[3] * 32; /// 4;
				//	}
				//	else
				//	{
				//		PDmod = 255;
				//	}
				//}
			}

			/*
			if(BUTTON_U1_ON)
			{
				FREQ+=16;
			}
			if(BUTTON_U2_ON)
			{
				DOUBLE++;
			}
			if(BUTTON_U3_ON)
			{
				PHASEamt++;
			}
			if(BUTTON_U4_ON)
			{
				ENVamt++;
			}
			*/
		}
	}
}

void DCO_Synth::init_codec()
{
	//-----------------------------------------------------------------------------------
	// Gecho codec init
	//-----------------------------------------------------------------------------------

	codec_init(); //audio codec - init but keep off (RESET -> LOW)
	codec_ctrl_init(); //audio codec - release reset, init controller

	//-----------------------------------------------------------------------------------
	//set codec master volume

	//CodecCommandBuffer[1] = CODEC_VOLUME_DEFAULT;
	CodecCommandBuffer[1] = 0x08; //4dB
	//CodecCommandBuffer[1] = 0x00; //0dB
	//CodecCommandBuffer[1] = 0xf8; //-4dB

	CodecCommandBuffer[0] = CODEC_MAP_MASTER_A_VOL; //0x20: master volume set - MSTA
	send_codec_ctrl(CodecCommandBuffer, 2);
	CodecCommandBuffer[0] = CODEC_MAP_MASTER_B_VOL; //0x21: master volume set - MSTB
	send_codec_ctrl(CodecCommandBuffer, 2);

	//-----------------------------------------------------------------------------------
	//set headphones volume

	//CodecCommandBuffer[1] = 0xd0; //
	//CodecCommandBuffer[1] = 0xe8; //
	//CodecCommandBuffer[1] = 0x00; //+0db
	CodecCommandBuffer[1] = HP_VOLUME_DEFAULT;

	CodecCommandBuffer[0] = CODEC_MAP_HP_A_VOL; //0x22: Headphone Volume - HPA
	send_codec_ctrl(CodecCommandBuffer, 2);
	CodecCommandBuffer[0] = CODEC_MAP_HP_B_VOL; //0x23: Headphone Volume - HPB
	send_codec_ctrl(CodecCommandBuffer, 2);

	//-----------------------------------------------------------------------------------
	//enable the codec

	I2S_Cmd(CODEC_I2S, ENABLE);
}

// This is the interrupt request handler (IRQ) for ALL USART3 interrupts
void USART3_IRQHandler(void){

	extern DCO_Synth *dco;

	// check if the USART1 receive interrupt flag was set
	if(USART_GetITStatus(USART3, USART_IT_RXNE))
	{
		char MIDI_data = USART3->DR; // read the character from the USART3 data register

		if(MIDI_data!=248 && MIDI_data!=254)
		{
			if(MIDI_data!=144 && MIDI_data!=128)
			{
				if(!dco->MIDI_note_set)  //this is note byte
				{
					dco->FREQ = MIDI_notes_to_freq[(int)MIDI_data];
					dco->MIDI_note_set = 1;
				}
				else //this is velocity byte
				{
					dco->MIDI_note_set = 0;
				}
			}
		}
	}
}

//---------------------------------------------------------------------------------
// version with sequencer
//---------------------------------------------------------------------------------

void DCO_Synth::v3_play_loop()
{
	#define SEQ_MAX_NOTES 32		//maximum length = 32 notes
	#define SEQ_NOTE_DIVISION 8		//steps per note
	float sequence_notes[SEQ_MAX_NOTES * SEQ_NOTE_DIVISION];
	int seq_ptr = 0, seq_length;
	const char *sequence = "a3a3a3a3 c4a3d4c4 a3a3a3a3 d4c4e4d4 a4g4e4a4 g4e4a4g4 f#4d4a3f#4 d4a3c4b3";

	//MusicBox *chord = new MusicBox();

	seq_length = MusicBox::get_song_total_melody_notes((char*)sequence);
	parse_notes((char*)sequence, sequence_notes, NULL);

	#define SEQ_SPREAD 1
	spread_notes(sequence_notes, seq_length, SEQ_SPREAD); //spread the sequence 8 times
	seq_length *= SEQ_SPREAD;

	//FREQ = MIDI_notes_to_freq[data];

	DOUBLE=8;
	PHASEamt=217;
	ENVamt=144;
	SET_mode = 1;
	DCO_BLOCKS_ACTIVE = 2;//16;

	#define FREQ_MULTIPLIER 12

	//use_table256 = (uint8_t*)sawtable256;
	use_table256 = (uint8_t*)squaretable256;

	while(1)
	{
		//-------------------- DCO block ------------------------------------------
		DCO_output[0] = 0;
		DCO_output[1] = 0;

		for (int b=0; b<DCO_BLOCKS_ACTIVE; b++)
		{
			/*
			if(b%5==1)
			{
				phacc[b] += 2*(FREQ + (b * DOUBLE));
			}
			else if(b%5==2)
			{
				phacc[b] += (FREQ + (b * DOUBLE))/2;
			}
			else if(b%5==3)
			{
				phacc[b] += (FREQ + (b * DOUBLE))/4;
			}
			else
			{*/
				phacc[b] += FREQ + (b * DOUBLE);
			//}

			if (phacc[b] & 0x8000) {
				phacc[b] &= 0x7FFF;
				otone1[b] += 2;
				pdacc[b] += PDmod;
		    }
		    if (!otone1[b]) pdacc[b] = 0;
		    otone2[b] = (pdacc[b] >> 3) & 255;
		    uint8_t env = (255-otone1[b]);
		    DCO_output[b%2] += (use_table256[otone2[b]] + use_table256[(otone2[b] + (127 - env)) & 255]);
		}

		if (!(envtick--)) {
			envtick=582;

			//--------- Resonant Peak ENV modulation ----------------------------------------

			//if(!DCO_mode)
			{
				PDmod=((resonant_peak_mod_volume*ENVamt)>>8)+PHASEamt;
				if (PDmod>255) PDmod=255;
			}
		}

		//-------------------------------------------------------------------------
		// send sample to codec
		//-------------------------------------------------------------------------

		sample_i16 = (int16_t)DCO_output[0];
		sample_i16 += echo_buffer[echo_buffer_ptr];

		while (!SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE));
		SPI_I2S_SendData(CODEC_I2S, sample_i16);

		sample_i16 = (int16_t)DCO_output[1];

		//wrap in the echo loop
		echo_buffer_ptr0++;
		if(echo_buffer_ptr0 >= echo_dynamic_loop_length)
		{
			echo_buffer_ptr0 = 0;
		}

		echo_buffer_ptr = echo_buffer_ptr0 + 1;
		if(echo_buffer_ptr >= echo_dynamic_loop_length)
		{
			echo_buffer_ptr = 0;
		}

		//add echo from the loop
		sample_i16 += echo_buffer[echo_buffer_ptr];

		//store result to echo, the amount defined by a fragment
		echo_buffer[echo_buffer_ptr0] = sample_i16 * ECHO_MIXING_GAIN_MUL / ECHO_MIXING_GAIN_DIV;

		while (!SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE));
		SPI_I2S_SendData(CODEC_I2S, sample_i16);

		sampleCounter++;

		if (TIMING_BY_SAMPLE_ONE_SECOND_W_CORRECTION)
		{
			sampleCounter = 0;
			seconds++;
		}

		/*
		if ((sampleCounter%(2*I2S_AUDIOFREQ/5)==1234) && PROG_buttons_control_tone_volume) //every 200ms at sample #1234
		{
			buttons_volume_control();
		}

		queue_codec_ctrl_process();
		*/

		if (TIMING_BY_SAMPLE_EVERY_10_MS==0) //100Hz
		{
			if(ADC_process_sensors()==1)
			{
				ADC_last_result[2] = -ADC_last_result[2];
				ADC_last_result[3] = -ADC_last_result[3];
				CLEAR_ADC_RESULT_RDY_FLAG;

				sensors_loop++;

				IR_sensors_LED_indicators(ADC_last_result);

				if(SET_mode==0)
				{
					/*
					if(GATE_PB0)
					{
						//FREQ = ADC_measured_vals[0] * 16; //raw values must be used, not differential (IRS LED on/off)
						FREQ = find_nearest_note_freq(ADC_measured_vals[0]);
						LED_SIG_ON;
					}
					else
					{
						FREQ = 0;
						LED_SIG_OFF;
					}
					*/

					FREQ = ADC_last_result[0] * 4;

					//DOUBLE = ADC_last_result[1] / 8;
					DOUBLE = ADC_last_result[1] / 12;

					//PHASEamt = ADC_last_result[2] / 8;
					PHASEamt = ADC_last_result[2] / 16;

					//if(!DCO_mode)
						ENVamt = ADC_last_result[3] * 16; /// 4;
					//}
					//else
					//{
					//	if(ADC_last_result[3] > 200)
					//	{
					//		//PDmod = ADC_last_result[3] / 2; /// 4;
					//		ENVamt = ADC_last_result[3] * 32; /// 4;
					//	}
					//	else
					//	{
					//		PDmod = 255;
					//	}
					//}
				}
				else
				{
					if(BUTTON_U1_ON)
					{
						FREQ = ADC_last_result[0] * 8;
					}
					if(BUTTON_U2_ON)
					{
						DOUBLE = ADC_last_result[1] / 12;
					}
					if(BUTTON_U3_ON)
					{
						PHASEamt = ADC_last_result[2] / 16;
					}
					if(BUTTON_U4_ON)
					{
						ENVamt = ADC_last_result[3] * 16;
					}
				}
			}

			if(BUTTON_SET_ON)
			{
				if(!button_set_held)
				{
					if(!SET_mode)
					{
						LED_SIG_ON;
						SET_mode = 1;
						button_set_held = 1;
					}
					else
					{
						LED_SIG_OFF;
						SET_mode = 0;
						button_set_held = 1;
					}
				}
			}
			else
			{
				button_set_held = 0;
			}


		}

		//if (TIMING_BY_SAMPLE_EVERY_250_MS==0) //4Hz
		if (TIMING_BY_SAMPLE_EVERY_125_MS==0) //8Hz
		{
			//if(ADC_last_result[0] < SEQUENCER_THRESHOLD)
			//{
				if(sequence_notes[seq_ptr] > 0)
				{
					FREQ = sequence_notes[seq_ptr] * FREQ_MULTIPLIER;
				}

				if (++seq_ptr == seq_length)
				{
					seq_ptr = 0;
				}
			//}
		}

	}
}

void DCO_Synth::v4_play_loop()
//SIGNAL(PWM_INTERRUPT)
{

  uint8_t value;
  uint16_t output;

  // incorporating DuaneB's volatile fix
  uint16_t syncPhaseAcc;
  volatile uint16_t syncPhaseInc;
  uint16_t grainPhaseAcc;
  volatile uint16_t grainPhaseInc;
  uint16_t grainAmp;
  volatile uint8_t grainDecay;
  uint16_t grain2PhaseAcc;
  volatile uint16_t grain2PhaseInc;
  uint16_t grain2Amp;
  volatile uint8_t grain2Decay;

  int led_status = 0;

  while(1)
  {
	  syncPhaseAcc += syncPhaseInc;
	  if (syncPhaseAcc < syncPhaseInc) {
		// Time to start the next grain
		grainPhaseAcc = 0;
		grainAmp = 0x7fff;
		grain2PhaseAcc = 0;
		grain2Amp = 0x7fff;
		//LED_PORT ^= 1 << LED_BIT; // Faster than using digitalWrite
		led_status?LED_SIG_ON:LED_SIG_OFF;
		led_status=led_status?0:1;
	  }

	  // Increment the phase of the grain oscillators
	  grainPhaseAcc += grainPhaseInc;
	  grain2PhaseAcc += grain2PhaseInc;

	  // Convert phase into a triangle wave
	  value = (grainPhaseAcc >> 7) & 0xff;
	  if (grainPhaseAcc & 0x8000) value = ~value;
	  // Multiply by current grain amplitude to get sample
	  output = value * (grainAmp >> 8);

	  // Repeat for second grain
	  value = (grain2PhaseAcc >> 7) & 0xff;
	  if (grain2PhaseAcc & 0x8000) value = ~value;
	  output += value * (grain2Amp >> 8);

	  // Make the grain amplitudes decay by a factor every sample (exponential decay)
	  grainAmp -= (grainAmp >> 8) * grainDecay;
	  grain2Amp -= (grain2Amp >> 8) * grain2Decay;

	  // Scale output to the available range, clipping if necessary
	  output >>= 9;
	  if (output > 255) output = 255;

	  // Output to PWM (this is faster than using analogWrite)
	  //PWM_VALUE = output;

		//-------------------------------------------------------------------------
		// send sample to codec
		//-------------------------------------------------------------------------

		sample_i16 = (int16_t)output;//[0];
		sample_i16 += echo_buffer[echo_buffer_ptr];

		while (!SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE));
		SPI_I2S_SendData(CODEC_I2S, sample_i16);

		sample_i16 = (int16_t)output;//[1];

		//wrap in the echo loop
		echo_buffer_ptr0++;
		if(echo_buffer_ptr0 >= echo_dynamic_loop_length)
		{
			echo_buffer_ptr0 = 0;
		}

		echo_buffer_ptr = echo_buffer_ptr0 + 1;
		if(echo_buffer_ptr >= echo_dynamic_loop_length)
		{
			echo_buffer_ptr = 0;
		}

		//add echo from the loop
		sample_i16 += echo_buffer[echo_buffer_ptr];

		//store result to echo, the amount defined by a fragment
		echo_buffer[echo_buffer_ptr0] = sample_i16 * ECHO_MIXING_GAIN_MUL / ECHO_MIXING_GAIN_DIV;

		while (!SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE));
		SPI_I2S_SendData(CODEC_I2S, sample_i16);

		sampleCounter++;

		if (TIMING_BY_SAMPLE_ONE_SECOND_W_CORRECTION)
		{
			sampleCounter = 0;
			seconds++;
		}

  }
}
