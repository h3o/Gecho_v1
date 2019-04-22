/*
 * Bytebeat.cpp
 *
 *  Created on: 19 July 2018
 *      Author: mario (http://gechologic.com/contact)
 *
 *  Used bytebeat math formulas from: https://youtu.be/GtQdIYUtAHg
 *
 *  This file is part of the Gecho Loopsynth Firmware Development Framework.
 *  It can be used within the terms of CC-BY-NC-SA license.
 *
 *  Find more information at: http://gechologic.com/gechologists/
 */

#include <extensions/Bytebeat.h>

void bytebeat(int mode)
{
	//there is an external sampleCounter variable already, it will be used to timing and counting seconds
	sampleCounter = 0;
	seconds = 0;

	//set input gain
	//const float op_gain = PREAMP_BOOST * UNFILTERED_SIGNAL_VOLUME * 2;

	//if(mode)
	//{
	//}
	//else
	//{
	//}

	//-------------------------------------------------------------------

	while(ANY_USER_BUTTON_ON); //wait till all buttons released (some may still be pressed from selecting a channel)

	LED_R8_all_OFF();
	LED_O4_all_OFF(); //turn off all LEDs that might be on
	LED_B5_all_OFF();
	LED_W8_all_OFF();

	LED_R8_0_ON; //first red LED indicates song #1
	LED_W8_2_ON; //two white LEDs indicate default stereo panning option
	LED_W8_5_ON;

	//init and configure audio codec
	codec_init();
	codec_ctrl_init();
	I2S_Cmd(CODEC_I2S, ENABLE);

	program_settings_reset();

	int button_state[4] = {0,0,0,0}; //de-bouncing of the buttons
	#define BUTTON_PRESS_THRESHOLD 2 //200ms

	int bytebeat_song_ptr = 0;
	int bytebeat_song = 0;
	#define BYTEBEAT_SONGS 8

	#define STEREO_MIXING_STEPS 4
	float stereo_mixing[STEREO_MIXING_STEPS] = {0.5f,0.4f,0.2f,0}; //50, 60, 80 and 100% mixing ratio
	int stereo_mixing_step = 1; //default mixing 60:40

	int tt[4];
	unsigned char s[4];
	float mix1, mix2;

	#define BYTEBEAT_MIXING_VOLUME 2.5f

	while(1) //function loop
	{
		sampleCounter++;
		if (TIMING_BY_SAMPLE_ONE_SECOND_W_CORRECTION) //one full second passed
		{
			seconds++;
			sampleCounter = 0;
		}

		//t[0] = bytebeat_song_ptr/6;
		//t[0] = bytebeat_song_ptr/12;
		//t[0] = bytebeat_song_ptr/24;

        bytebeat_song_ptr++;

		if(bytebeat_song==0)
		{
			tt[0] = bytebeat_song_ptr/6;

			tt[1] = tt[0]/4;

			s[0] = tt[0]*(tt[0]>>11&tt[0]>>8&123&tt[0]>>3);
			s[1] = tt[1]*(tt[1]>>11&tt[1]>>8&123&tt[1]>>3);

			mix1 = (float)s[0];
			mix2 = (float)s[1];
		}
		else if(bytebeat_song==1)
		{
			tt[0] = bytebeat_song_ptr/6;

			//t[1] = t[0];
			tt[1] = tt[0]/2;
			//t[1] = t[0]/4;

			tt[2] = bytebeat_song_ptr/3;
			tt[3] = tt[1]/2;

			s[0] = (tt[0]*(tt[0]>>5|tt[0]>>8))>>(tt[0]>>16);
			s[1] = (tt[1]*(tt[1]>>5|tt[1]>>8))>>(tt[1]>>16);
			s[2] = (tt[2]*(tt[2]>>5|tt[2]>>8))>>(tt[2]>>16);
			s[3] = (tt[3]*(tt[3]>>5|tt[3]>>8))>>(tt[3]>>16);

			mix1 = (float)(s[0]+s[2]);
			mix2 = (float)(s[1]+s[3]);
		}
		else if(bytebeat_song==2)
		{
			tt[0] = bytebeat_song_ptr/6;

			//t[1] = t[0];
			tt[1] = tt[0]/2;
			//t[1] = t[0]/4;

			//t[2] = t[1]/2;
			//t[3] = t[2]/2;

			s[0] = tt[0]*((tt[0]>>12|tt[0]>>8)&63&tt[0]>>4);
			s[1] = tt[1]*((tt[1]>>12|tt[1]>>8)&63&tt[1]>>4);
			//s[2] = t[2]*((t[2]>>12|t[2]>>8)&63&t[2]>>4);
			//s[3] = t[3]*((t[3]>>12|t[3]>>8)&63&t[3]>>4);

			mix1 = (float)s[0];
			mix2 = (float)s[1];
			//mix1 = (float)(s[0]+s[2]);
			//mix2 = (float)(s[1]+s[3]);
		}
		else if(bytebeat_song==3)
		{
			tt[0] = bytebeat_song_ptr;

			tt[1] = tt[0]/2;

			s[0] = tt[0]*((tt[0]>>9|tt[0]>>13)&25&tt[0]>>6);
			s[1] = tt[1]*((tt[1]>>9|tt[1]>>13)&25&tt[1]>>6);

			mix1 = (float)s[0];
			mix2 = (float)s[1];
		}
		else if(bytebeat_song==4)
		{
			tt[0] = bytebeat_song_ptr/3;

			tt[1] = tt[0]/2;

			#define t tt[0]
			s[0] = ((-t&4095)*(255&t*(t&t>>13))>>12)+(127&t*(234&t>>8&t>>3)>>(3&t>>14));
			#define t tt[1]
			s[1] = ((-t&4095)*(255&t*(t&t>>13))>>12)+(127&t*(234&t>>8&t>>3)>>(3&t>>14));
			#undef t

			//s[0] = (t[0]*(t[0]>>8*(t[0]>>15|t[0]>>8)&(20|(t[0]>>19)*5>>t[0]|t[0]>>3));
			//s[1] = (t[1]*(t[1]>>8*(t[1]>>15|t[1]>>8)&(20|(t[1]>>19)*5>>t[1]|t[1]>>3));

			//s[0] = ((((5&((3 *(23*(4^t[0])))+t[0]))*(9*((15>>((9&((t[0]&12)^15))>>5))*2)))*((((t[0]*(t[0]*8))>>10)&t[0])>>42))^15);
			//s[1] = ((((5&((3 *(23*(4^t[1])))+t[1]))*(9*((15>>((9&((t[1]&12)^15))>>5))*2)))*((((t[1]*(t[1]*8))>>10)&t[1])>>42))^15);

			mix1 = (float)s[0];
			mix2 = (float)s[1];
		}
		else if(bytebeat_song==5)
		{
			tt[0] = bytebeat_song_ptr/3;
			tt[1] = tt[0]/2;

			/*
			//long gaps of silence but very interesting bits!
			#define t tt[0]
			s[0] = t*(t>>((t>>9|t>>8))&63&t>>4);
			#define t tt[1]
			s[1] = t*(t>>((t>>9|t>>8))&63&t>>4);
			#undef t
			*/

			/*
			//(t^t>>8)|t<<3&56^t
			//that one seems to produce something interesting :D

			//low tone motorboating with quieter high buzzing
			#define t tt[0]
			s[0] = (t^t>>8)|t<<3&56^t;
			#define t tt[1]
			s[1] = (t^t>>8)|t<<3&56^t;
			#undef t
			*/

			//combined:
			#define t tt[0]
			s[0] = t*(t>>((t>>9|t>>8))&63&t>>4) + (t^t>>8)|t<<3&56^t;
			#define t tt[1]
			s[1] = t*(t>>((t>>9|t>>8))&63&t>>4) + (t^t>>8)|t<<3&56^t;
			#undef t

			/*
			//not fast enough within this sampling rate timing

			//long gaps of silence but very interesting bits - now compressed
			s[0] = 0;
			s[1] = 0;
			while(s[0]==0 && s[1]==0)
			{
				tt[0] = bytebeat_song_ptr/3;
				tt[1] = tt[0]/2;
				#define t tt[0]
				s[0] = t*(t>>((t>>9|t>>8))&63&t>>4);
				#define t tt[1]
				s[1] = t*(t>>((t>>9|t>>8))&63&t>>4);
				#undef t
				bytebeat_song_ptr++;
			}
			bytebeat_song_ptr--;
			*/

			mix1 = (float)s[0];
			mix2 = (float)s[1];
		}
		/*
		else if(bytebeat_song==6)
		{
			tt[0] = bytebeat_song_ptr;///3;
			tt[1] = tt[0]/2;

			//put (t>>1)+(sin(t))*(t>>4)  at 44,100 sample rate
			//it sounds like voices and static

			#define t tt[0]
			s[0] = (t>>1)+(sin(t))*(t>>4);
			#define t tt[1]
			s[1] = (t>>1)+(sin(t))*(t>>4);
			#undef t

			mix1 = (float)s[0];
			mix2 = (float)s[1];
		}
		else if(bytebeat_song==7)
		{
			tt[0] = bytebeat_song_ptr;///3;
			tt[1] = tt[0]/2;

			//try this at 44,100 rate (t>>1)+(cos(t<<1))*(t>>4)

			#define t tt[0]
			s[0] = (t>>1)+(cos(t<<1))*(t>>4);
			#define t tt[1]
			s[1] = (t>>1)+(cos(t<<1))*(t>>4);
			#undef t

			mix1 = (float)s[0];
			mix2 = (float)s[1];
		}
		*/
		else if(bytebeat_song==6)
		{
			tt[0] = bytebeat_song_ptr/3;
			tt[1] = tt[0]/2;

			/*
			//nothing?
			#define t tt[0]
			s[0] = (t>>((t>>(3)|t>>(5)))&(63)&t>>7)*t;
			#define t tt[1]
			s[1] = (t>>((t>>(3)|t>>(5)))&(63)&t>>7)*t;
			#undef t
			*/

			//cool bagpipe-like sound with rhythm
			#define t tt[0]
			s[0] = t*t*~((t>>16|t>>12)&215&~t>>8);
			#define t tt[1]
			s[1] = t*t*~((t>>16|t>>12)&215&~t>>8);
			#undef t

			mix1 = (float)s[0];
			mix2 = (float)s[1];
		}
		else if(bytebeat_song==7)
		{
			tt[0] = bytebeat_song_ptr/3;
			tt[1] = tt[0]/2;

			//Here's one that sounds exactly like music.
			//Hasn't repeated once at all during 66 seconds.
			//t * ((t>>14|t>>9)&92&t>>5)
			//even has an ending! :D SWEET!

			#define t tt[0]
			s[0] = t*((t>>14|t>>9)&92&t>>5);
			#define t tt[1]
			s[1] = t*((t>>14|t>>9)&92&t>>5);
			#undef t

			mix1 = (float)s[0];
			mix2 = (float)s[1];
		}

		//------------------------------------------------------------------------------------------------------

		//mix1 = (float)bytebeat_echo(s[0]+s[2]);
		//mix2 = (float)bytebeat_echo(s[1]+s[3]);

		//sample_mix = (int16_t)(4096/2 - (int16_t)ADC1_read()) / 4096.0f * op_gain;
		//s[0] = (int16_t)(mix1*0.7f+mix2*0.3f);
		sample_mix = (mix1*stereo_mixing[stereo_mixing_step]+mix2*(1.0f-stereo_mixing[stereo_mixing_step])) * BYTEBEAT_MIXING_VOLUME;

		//send data to codec, with added reverb and echo
        while (!SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE));
        SPI_I2S_SendData(CODEC_I2S, (uint16_t)sample_mix);

        //sample_mix = (int16_t)(4096/2 - (int16_t)ADC2_read()) / 4096.0f * op_gain;
		//s[1] = (int16_t)(mix2*0.7f+mix1*0.3f);
		sample_mix = (mix2*stereo_mixing[stereo_mixing_step]+mix1*(1.0f-stereo_mixing[stereo_mixing_step])) * BYTEBEAT_MIXING_VOLUME;

		//send data to codec, with added reverb and echo
        while (!SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE));
        SPI_I2S_SendData(CODEC_I2S, (uint16_t)sample_mix);

		//we will enable default controls too (user buttons B1-B4 to control volume, inputs and echo on/off)
		if (TIMING_BY_SAMPLE_EVERY_100_MS==1234) //10Hz periodically, at sample #1234
		{
			buttons_controls_during_play();

			if(BUTTON_U3_ON)
			{
				button_state[2]++;
			}
			else
			{
				button_state[2]=0;
			}
			if(BUTTON_U4_ON)
			{
				button_state[3]++;
			}
			else
			{
				button_state[3]=0;
			}

			/*
			//if the setting goes beyond allocated buffer size, set to maximum
			if(echo_dynamic_loop_length > REVERB_ECHO_BUFFER_SIZE)
			{
				echo_dynamic_loop_length = REVERB_ECHO_BUFFER_SIZE; //set default value
			}

			//it does not make sense to have microphones off here, skip this setting and re-enable them
			if(input_mux_current_step==INPUT_MUX_OFF)
			{
				ADC_set_input_multiplexer(input_mux_current_step = INPUT_MUX_MICS);
			}
			*/
		}

		if (TIMING_BY_SAMPLE_EVERY_100_MS==1235) //10Hz periodically, at sample #1235
		{
			if(button_state[3]==BUTTON_PRESS_THRESHOLD)
			{
				bytebeat_song_ptr = 0;
				bytebeat_song++;
				if(bytebeat_song==BYTEBEAT_SONGS)
				{
					bytebeat_song = 0;
				}


				LED_R8_all_OFF();
				LED_R8_set(bytebeat_song, 1);
			}
			if(button_state[2]==BUTTON_PRESS_THRESHOLD)
			{
				/*
				bytebeat_song_ptr = 0;
				bytebeat_song--;
				if(bytebeat_song==-1)
				{
					bytebeat_song = BYTEBEAT_SONGS-1;
				}
				*/

				stereo_mixing_step++;
				if(stereo_mixing_step==STEREO_MIXING_STEPS)
				{
					stereo_mixing_step = 0;
				}
				if(stereo_mixing_step==0)
				{
					LED_W8_set_byte(0x18);
				}
				else if(stereo_mixing_step==1)
				{
					LED_W8_set_byte(0x24);
				}
				else if(stereo_mixing_step==2)
				{
					LED_W8_set_byte(0x42);
				}
				else if(stereo_mixing_step==3)
				{
					LED_W8_set_byte(0x81);
				}
			}
		}

		//process any I2C commands in queue (e.g. volume change by buttons)
		queue_codec_ctrl_process();

	}	//end while(1)
}

