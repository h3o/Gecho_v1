/*
 * test_fn.c
 *
 *  Created on: Oct 31, 2016
 *      Author: mayo
 */

#include "Test_fn.h"
#include "hw/gpio.h"
#include "hw/leds.h"
#include "hw/controls.h"
#include "hw/signals.h"
#include "hw/sensors.h"
#include <Interface.h>
#include "notes.h"

void MAGNETIC_sensors_test(int sensor_value)
{
	LED_R8_all_OFF();
	LED_W8_all_OFF();

	int to_scale = (sensor_value - BOARD_MAG_SENSOR_VALUE_OFFSET - magnetic_sensor_calibration) / BOARD_MAG_SENSOR_VALUE_COEF;

	if(to_scale>7) { LED_R8_7_ON; }
	if(to_scale>6) { LED_R8_6_ON; }
	if(to_scale>5) { LED_R8_5_ON; }
	if(to_scale>4) { LED_R8_4_ON; }
	if(to_scale>3) { LED_R8_3_ON; }
	if(to_scale>2) { LED_R8_2_ON; }
	if(to_scale>1) { LED_R8_1_ON; }
	if(to_scale>0) { LED_R8_0_ON; }

	if(to_scale<-7) { LED_W8_7_ON; }
#ifdef CAN_BLOCK_SWD_DEBUG
	if(to_scale<-6) { LED_W8_6_ON; }
	if(to_scale<-5) { LED_W8_5_ON; }
#endif
	if(to_scale<-4) { LED_W8_4_ON; }
	if(to_scale<-3) { LED_W8_3_ON; }
	#ifdef CAN_USE_CH340G_LEDS
	if(to_scale<-2) { LED_W8_2_ON; }
	if(to_scale<-1) { LED_W8_1_ON; }
	#endif
	if(to_scale< 0) { LED_W8_0_ON; }
}

#ifdef GORS
/*
void Goertzel_detectors_capture(int tones_to_find, int *found_tones, char *goertzel_octaves) //if notes_to_find == 0 then loop forever
{
	//get 1/2s one channel sample into buffer with real and imaginary parts
	float *sample_buffer;
	int n_samples = 512;
	sample_buffer = (float*)malloc(n_samples * sizeof(float));
	int timing;

	int led_blue_state=0;

	Detectors *det = new Detectors(goertzel_octaves);
	//detectors utilize the chords generator to get base freqs
	parse_notes(det->gors_freq_notes, det->gors_freqs, NULL); //no led indicators buffer needed

	det->detectors_setup(n_samples);

    const int history_length = 14;
    int history[history_length],history_found[history_length];
    int h_ptr;
    int silence = 0;

    //clear history
    for(h_ptr=0;h_ptr<history_length;h_ptr++)
    {
    	history[h_ptr] = 0;
    	history_found[h_ptr] = 0;
    }

    int tones_found = 0;

	while(ANY_USER_BUTTON_ON); //wait till all buttons released (some still pressed from previous function)

    while((tones_found<tones_to_find || tones_to_find==0) && !ANY_USER_BUTTON_ON) //exit on any user button pressed
	{
		led_blue_state++;
		if(led_blue_state%2)
		{
			LED_SIG_ON;
		}
		else
		{
			LED_SIG_OFF;
		}
		timing = get_millis();
		for(int i=0;i<n_samples;i++)
		{
			sample_buffer[i] = (float)(4096/2 - (int16_t)ADC2_read()) / 4096.0f * PREAMP_BOOST;

			//timing by Codec
			while (!SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE));
            SPI_I2S_SendData(CODEC_I2S, 0);
			while (!SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE));
            SPI_I2S_SendData(CODEC_I2S, 0);
			while (!SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE));
            SPI_I2S_SendData(CODEC_I2S, 0);
			while (!SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE));
            SPI_I2S_SendData(CODEC_I2S, 0);
		}
		timing = get_millis() - timing;

		timing = get_millis();
		for(int i=0;i<n_samples;i++)
		{
			det->process_sample(sample_buffer[i]);
			//need to satisfy codec, otherwise it will emit rattling sound
			while (!SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE));
            SPI_I2S_SendData(CODEC_I2S, 0);
		}
		timing = get_millis() - timing;

		int max_freq = det->find_max_freq();

		det->reset_all();

		int result = max_freq;

		if(result > -1)
		{
			result ++;

			if(result>13)
			{
				result -= 12;
			}

			history[result]++;
		}
		else
		{
			silence++;
			for(h_ptr=0;h_ptr<history_length;h_ptr++)
		    {
		    	history[h_ptr]--;
		    	if(history[h_ptr] < 0)
		    	{
		    		history[h_ptr] = 0;
		    	}
		    }
		}

	    for(h_ptr=0;h_ptr<history_length;h_ptr++)
	    {
	    	if(history[h_ptr] > 3)
	    	{
	    		silence = 0;
	    		KEY_LED_on(h_ptr);
	    		if(history_found[h_ptr]==0) //new note found
	    		{
	    			history_found[h_ptr] = 1;
	    			tones_found++;
	    		}
	    	}
	    }

	    if(silence == 20) //tones_found==4)
	    {
	    	KEY_LED_all_off();
    		tones_found = 0;
	    	silence = 0;

    		//clear history
    		for(h_ptr=0;h_ptr<history_length;h_ptr++)
    	    {
    	    	history[h_ptr] = 0;
    	    	history_found[h_ptr] = 0;
    	    }
	    }
	}
	codec_reset();

	//convert from history array to notes number (301=c3...)
	if(found_tones!=NULL)
	{
		if(tones_found==0) //none found, user cancelled by button
		{
			for(int i=0;i<=tones_to_find;i++)
			{
				found_tones[i] = 0;
			}
		}
		else
		{
			int f_ptr = 0;
			for(int n=1;n<=13;n++)
			{
				if(history_found[n])
				{
					found_tones[f_ptr++] = 300+n;
				}
			}
		}
	}
}
*/

void AutoCorrelation_capture(int tones_to_find, int *found_tones, int preamp_boost) //if notes_to_find == 0 then loop forever
{
	#define N_SAMPLES 512 //256 //I2S_AUDIOFREQ/2
	float *capture_buf;
	float correlation[N_SAMPLES];

	capture_buf = (float*)malloc(N_SAMPLES * sizeof(float));

	//int timing;
	int led_blue_state=0;

    const int history_length = 14;
    int history[history_length],history_found[history_length];
    int h_ptr;
    int silence = 0;

    //clear history
    for(h_ptr=0;h_ptr<history_length;h_ptr++)
    {
    	history[h_ptr] = 0;
    	history_found[h_ptr] = 0;
    }

	memset(found_tones,0,tones_to_find*sizeof(int));

	int tones_found = 0;
	float max_lag_avg = 0;
	int max_lag_cnt = 0;

	#define MAX_LAG_HISTORY_DEPTH 2 //measuring max lag over x sample chunks
	int max_lag_history[MAX_LAG_HISTORY_DEPTH]; //needed to check whether values are close enough
	#define SECOND_LEVEL_HISTORY_HITS_REQUIRED 1 //how many subsequent groups of chunks need to contain good frequency
	#define SILENCE_REQUIRED_TO_RESET 50 //after how many sample chunks to reset found notes (before chord completed)

	while(ANY_USER_BUTTON_ON); //wait till all buttons released (some still pressed from previous function)

	KEY_LED_all_off();
	codec_reset(); //keep codec in reset state, we only need I2S peripheral for timing
	I2S_Cmd(CODEC_I2S, ENABLE);

	while((tones_found<tones_to_find || tones_to_find==0) && !ANY_USER_BUTTON_ON) //exit on any user button pressed
	{
		led_blue_state++;
		if(led_blue_state%2)
		{
			LED_SIG_ON;
		}
		else
		{
			LED_SIG_OFF;
		}
		//timing = get_millis();

		//int zero_crossings = 0;
		//int previous = 0;

		for(int i=0;i<N_SAMPLES;i++)
		{
			//capture_buf[i] = (float)(4096/2 - (int16_t)ADC1_read()) / 4096.0f * preamp_boost;
			capture_buf[i] = (float)(4096/2 - (int16_t)ADC2_read()) / 4096.0f * preamp_boost;

			//capture_buf[i] = (float)(4096/2 - (int16_t)ADC_GetConversionValue(ADC1)) / 4096.0f * preamp_boost;
			//ADC_SoftwareStartConv(ADC1);

			/*
			if(i>1)
			{
				if((capture_buf[i-1] < 0) && (capture_buf[i] > 0))
				{
					zero_crossings++;
				}
			}
			*/

			//timing by Codec
			while (!SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE));
            SPI_I2S_SendData(CODEC_I2S, 0);
		}
		//timing = get_millis() - timing;
		//timing = get_millis();

		float sum;
		float sum_correlations = 0;
		for(int i=0;i<N_SAMPLES/2;i++)
		{
			sum = 0;
			for(int j=0;j<N_SAMPLES-i;j++)
			{
				sum+=capture_buf[j]*capture_buf[j+i];
			}
			correlation[i] = sum;
			sum_correlations += sum;

			//need to feed the codec, otherwise it will emit rattling sound
			//while (!SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE));
            //SPI_I2S_SendData(CODEC_I2S, 0);
		}

		//timing = get_millis() - timing;

		sum_correlations += 1;

		float max_corr = -10000.0f;
		float corr_at_point;
		int max_lag = -1;
		for(int i=15;i<N_SAMPLES/2;i++)
		{
			corr_at_point = correlation[i];// + 0.65 * correlation[i/2];
			/*
			if(i<N_SAMPLES/4)
			{
				corr_at_point += 0.35 * correlation[i*2];
			}
			*/
			if(corr_at_point > max_corr)
			{
				max_corr = corr_at_point;
				max_lag = i;
			}
		}

		int result = -1;

		if(max_lag > 20)
		{
			//if(sum_correlations > 10000)
			{
				if(max_corr > 50)
				{
					max_lag_avg += max_lag;
					max_lag_history[max_lag_cnt] = max_lag;
					max_lag_cnt++;

					if(max_lag_cnt==MAX_LAG_HISTORY_DEPTH)
					{
						max_lag_avg /= (float)max_lag_cnt;

						//check if all values are close enough to the average
						int close_enough = 1;
						for(int v=0;v<MAX_LAG_HISTORY_DEPTH;v++)
						{
							if(fabs(max_lag_avg - (float)max_lag_history[v]) > 4)
							{
								close_enough = 0;
							}
						}

						if(close_enough)
						{
							//translate to note

							while(max_lag_avg >= 87.0f)
							{
								max_lag_avg /= 2;
							}
							if(max_lag_avg <= 44.0f)
							{
								max_lag_avg *= 2;
							}

							if(fabs(max_lag_avg - 84.0f) <= 2.5f)
							{
								result = 0; //note c
							}
							if(fabs(max_lag_avg - 79.0f) <= 1.9f)
							{
								result = 1; //note c#
							}
							if(fabs(max_lag_avg - 75.0f) <= 1.9f)
							{
								result = 2; //note d
							}
							if(fabs(max_lag_avg - 71.0f) <= 1.9f)
							{
								result = 3; //note d#
							}
							if(fabs(max_lag_avg - 67.0f) <= 1.9f)
							{
								result = 4; //note e
							}
							if(fabs(max_lag_avg - 63.0f) <= 1.5f)
							{
								result = 5; //note f
							}
							if(fabs(max_lag_avg - 60.0f) <= 1.4f)
							{
								result = 6; //note f#
							}
							if(fabs(max_lag_avg - 56.0f) <= 1.4f)
							{
								result = 7; //note g
							}
							if(fabs(max_lag_avg - 53.0f) <= 1.4f)
							{
								result = 8; //note g#
							}
							if(fabs(max_lag_avg - 50.0f) <= 1.3f)
							{
								result = 9; //note a
							}
							if(fabs(max_lag_avg - 47.0f) <= 0.9f)
							{
								result = 10; //note a#
							}
							if(fabs(max_lag_avg - 45.0f) <= 0.9f)
							{
								result = 11; //note b
							}
						}
						max_lag_avg = 0;
						max_lag_cnt = 0;
					}
				}
			}
		}
		else
		{
			max_lag_avg = 0;
			max_lag_cnt = 0;
		}

		if(result > -1)
		{
			result++;

			if(result>13)
			{
				result -= 12;
			}

			history[result]++;
		}
		else
		{
			silence++;
			for(h_ptr=0;h_ptr<history_length;h_ptr++)
			{
				history[h_ptr]--;
				if(history[h_ptr] < 0)
				{
					history[h_ptr] = 0;
				}
			}
		}

		for(h_ptr=0;h_ptr<history_length;h_ptr++)
		{
			if(history[h_ptr] >= SECOND_LEVEL_HISTORY_HITS_REQUIRED)
			{
				silence = 0;
				KEY_LED_on(h_ptr);
				if(history_found[h_ptr]==0) //new note found
				{
					history_found[h_ptr] = 1;
					tones_found++;
				}
			}
		}

		if(silence == SILENCE_REQUIRED_TO_RESET)
		{
			KEY_LED_all_off();
			tones_found = 0;
			silence = 0;

			//clear history
			for(h_ptr=0;h_ptr<history_length;h_ptr++)
			{
				history[h_ptr] = 0;
				history_found[h_ptr] = 0;
			}
		}
	}

	free(capture_buf);

	if(ANY_USER_BUTTON_ON)
	{
		return;
	}

	//convert from history array to notes number (301=c3...)
	if(found_tones!=NULL)
	{
		if(tones_found==0) //none found, user cancelled by button
		{
			for(int i=0;i<=tones_to_find;i++)
			{
				found_tones[i] = 0;
			}
		}
		else
		{
			int f_ptr = 0;
			for(int n=1;n<=13;n++)
			{
				if(history_found[n])
				{
					found_tones[f_ptr++] = 300+n;
				}
			}
		}
	}
}

void MagneticRing_capture(int tones_to_find, int *found_tones)
{
	//int tones_found = 0;
	int note_static = 0;
	int last_note = -1, new_note;
	int current_pair = 0;
	#define NOTE_STATIC_ACCEPT_LIMIT 30 //20 * 50ms = 1s ? looked like 2sec -> 10
	#define NOTES_TO_COLLECT 3
	//int finished;
	int notes_collected = 0;
	int collected_notes[NOTES_TO_COLLECT];
	int collected_notes_blink = 0;
	#define COLLECTED_NOTES_BLINK_RATE 10

	//added later - maybe refactor
	int param_i;
	int led_blue_state, led_blue_dim=0;

	memset(found_tones,0,tones_to_find*sizeof(int));

	while(ANY_USER_BUTTON_ON); //wait till all buttons released (some still pressed from previous function)
	KEY_LED_all_off();

	//PROG_audio_input_microphones = false;
	//PROG_magnetic_sensor_test = true;
	//filters_and_signals_init();

	ACTIVE_FILTERS_PAIRS = tones_to_find;
	PROGRESS_UPDATE_FILTERS_RATE = 165; //every 6.06 ms, at 0.213 ms

	for(int i=0;i<ACTIVE_FILTERS_PAIRS;i++) //set initial freqs to none to keep it quiet
	{
		fil->iir2[i].setResonance(0.998f);
		fil->iir2[FILTERS/2 + i].setResonance(0.998f);

		fil->iir2[i].setCutoff(0.001);
		fil->iir2[FILTERS/2 + i].setCutoff(0.001);

		fil->fp.mixing_volumes[i] = 0.0f;
		fil->fp.mixing_volumes[FILTERS/2 + i] = 0.0f;
	}

	//re-init the codec as previewing after capturing has probably disabled it
	codec_restart();

	PROG_add_echo = false; //echo disabled as it does not work - something not initialized there properly

	if(PROG_add_echo)
	{
		init_echo_buffer();
	}

	while((notes_collected<tones_to_find || tones_to_find==0) && !ANY_USER_BUTTON_ON) //exit on any user button pressed
	{
		led_blue_state++;

		if(led_blue_state%(++led_blue_dim%5000)==0)
		{
			LED_SIG_ON;
		}
		else
		{
			LED_SIG_OFF;
		}

		if (!(sampleCounter & 0x00000001)) //left or right channel
		{
			float r = PseudoRNG1a_next_float();
			//float r = PseudoRNG1b_next_float();
			memcpy(&random_value, &r, sizeof(random_value));

			sample_f[0] = ( (float)(32768 - (int16_t)random_value) / 32768.0f) * noise_volume;
			sample_f[1] = ( (float)(32768 - (int16_t)(random_value>>16)) / 32768.0f) * noise_volume;
		}


		if (sampleCounter & 0x00000001) //left or right channel
		{
			sample_mix = IIR_Filter::iir_filter_multi_sum(sample_f[0],fil->iir2,ACTIVE_FILTERS_PAIRS,fil->fp.mixing_volumes)*MAIN_VOLUME; //volume2;
		}
		else
		{
			sample_mix = IIR_Filter::iir_filter_multi_sum(sample_f[1],fil->iir2+FILTERS/2,ACTIVE_FILTERS_PAIRS,fil->fp.mixing_volumes+FILTERS/2)*MAIN_VOLUME; //volume2;
		}

		sample_i16 = (int16_t)(sample_mix);

		if(PROG_add_echo)
		{
			//add echo
			echo_buffer_ptr0++;
			if(echo_buffer_ptr0 >= ECHO_BUFFER_LENGTH)
			{
				echo_buffer_ptr0 = 0;
			}

			echo_buffer_ptr = echo_buffer_ptr0 + 1;
			if(echo_buffer_ptr >= ECHO_BUFFER_LENGTH)
			{
				echo_buffer_ptr = 0;
			}

			sample_i16 += echo_buffer[echo_buffer_ptr];

			//store result to echo
			//echo_buffer[echo_buffer_ptr0] = sample_i16 * 1 / 2; //stm32f4-disc1
			echo_buffer[echo_buffer_ptr0] = sample_i16 * 2 / 3; //gecho v0.01
		}

    	if (sampleCounter%(2*I2S_AUDIOFREQ/20)==0) //every 50ms
    	{
    		//for(int i=0;i<WAVES_FILTERS;i++)
    		//{
    			//param_i = ADC_last_result[i] + base_freq;
    		//int note;
    		float note_freq;

			//param_i = (magnetic_sensor_latest_value - BOARD_MAG_SENSOR_VALUE_OFFSET - magnetic_sensor_calibration);
			param_i = (BOARD_MAG_SENSOR_VALUE_OFFSET + magnetic_sensor_calibration - magnetic_sensor_latest_value);

			#define MAG_SCALING_COEF 50

			KEY_LED_all_off();
			for(int i=0;i<notes_collected;i++)
			{
				KEY_LED_on(collected_notes[i]+1);
			}

			//blink collected notes
			if(notes_collected > 0 && ++collected_notes_blink%COLLECTED_NOTES_BLINK_RATE==0)
			{
				KEY_LED_all_off();
			}

			if(param_i < -MAG_SCALING_COEF)
			{
				if(param_i < -5*MAG_SCALING_COEF)
				{
					new_note = 10;
					note_freq = notes_freqs[10];
					KEY_LED_on(11);
				}
				else if(param_i < -4*MAG_SCALING_COEF)
				{
					new_note = 8;
					note_freq = notes_freqs[8];
					KEY_LED_on(9);
				}
				else if(param_i < -3*MAG_SCALING_COEF)
				{
					new_note = 6;
					note_freq = notes_freqs[6];
					KEY_LED_on(7);
				}
				else if(param_i < -2*MAG_SCALING_COEF)
				{
					new_note = 3;
					note_freq = notes_freqs[3];
					KEY_LED_on(4);
				}
				else
				{
					new_note = 1;
					note_freq = notes_freqs[1];
					KEY_LED_on(2);
				}

				direct_update_filters_id[0] = current_pair;
				direct_update_filters_freq[0] = note_freq;
				fil->fp.mixing_volumes[current_pair] = 1.0f;
				fil->fp.mixing_volumes[FILTERS/2 + current_pair] = 1.0f;

			}
			else if(param_i > MAG_SCALING_COEF)
			{
				/*
				note = (param_i / 40);
				if(note < 2)
				{
					note = 2;
				}
				if(note > 24)
				{
					note = 24;
				}

				//add halftones... 12 halftones is plus one octave -> frequency * 2
				float note_freq = NOTE_FREQ_A4 * pow(HALFTONE_STEP_COEF, note+1);
				*/

				//this will be faster
				if(param_i > 8*MAG_SCALING_COEF)
				{
					new_note = 12;
					note_freq = notes_freqs[12];
					KEY_LED_on(13);
				}
				else if(param_i > 7*MAG_SCALING_COEF)
				{
					new_note = 11;
					note_freq = notes_freqs[11];
					KEY_LED_on(12);
				}
				else if(param_i > 6*MAG_SCALING_COEF)
				{
					new_note = 9;
					note_freq = notes_freqs[9];
					KEY_LED_on(10);
				}
				else if(param_i > 5*MAG_SCALING_COEF)
				{
					new_note = 7;
					note_freq = notes_freqs[7];
					KEY_LED_on(8);
				}
				else if(param_i > 4*MAG_SCALING_COEF)
				{
					new_note = 5;
					note_freq = notes_freqs[5];
					KEY_LED_on(6);
				}
				else if(param_i > 3*MAG_SCALING_COEF)
				{
					new_note = 4;
					note_freq = notes_freqs[4];
					KEY_LED_on(5);
				}
				else if(param_i > 2*MAG_SCALING_COEF)
				{
					new_note = 2;
					note_freq = notes_freqs[2];
					KEY_LED_on(3);
				}
				else
				{
					new_note = 0;
					note_freq = notes_freqs[0];
					KEY_LED_on(1);
				}

				direct_update_filters_id[0] = current_pair;
				direct_update_filters_freq[0] = note_freq;
				fil->fp.mixing_volumes[current_pair] = 1.0f;
				fil->fp.mixing_volumes[FILTERS/2 + current_pair] = 1.0f;
			}
			else
			{
				direct_update_filters_id[0] = current_pair;
				direct_update_filters_freq[0] = 0;
				fil->fp.mixing_volumes[current_pair] = 0.0f;
				fil->fp.mixing_volumes[FILTERS/2 + current_pair] = 0.0f;
				new_note = -1;
				fil->start_update_filters_pairs(direct_update_filters_id, direct_update_filters_freq, 1); //update 1 pair
			}

			if(new_note == last_note && new_note >= 0)
			{
				note_static++;
				if(note_static == NOTE_STATIC_ACCEPT_LIMIT)
				{
					KEY_LED_all_off(); //blink to indicate note was held long enough
					current_pair++;

					collected_notes[notes_collected] = new_note;
					notes_collected++;

					if(current_pair==NOTES_TO_COLLECT)
					{
						//finished = 1;
					}
				}
				if(note_static == 2)
				{
					fil->start_update_filters_pairs(direct_update_filters_id, direct_update_filters_freq, 1); //update 1 pair
				}
			}
			else
			{
				last_note = new_note;
				note_static = 0;
			}

			//effect is repeating - not nice
			//fil->fp.melody_filter_pair = current_pair; //trigger disturb filters to get more resonant note

			//if(ADC_last_result[0] > 100)
			//{
				//fil->start_update_filters_pairs(direct_update_filters_id, direct_update_filters_freq, 1); //update 1 pair
			//}

    	}


    	while (!SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE));
        SPI_I2S_SendData(CODEC_I2S, sample_i16);

    	sampleCounter++;

		if (sampleCounter%(2*I2S_AUDIOFREQ/PROGRESS_UPDATE_FILTERS_RATE)==57) //every X ms, at 0.213 ms
		{
			fil->progress_update_filters(fil, seconds%2==0);
		}

		magnetic_sensor_latest_value = ADC1_read();
	}

	//codec_reset();

	if(ANY_USER_BUTTON_ON)
	{
		return;
	}

	//convert from history array to notes number (301=c3...)
	if(found_tones!=NULL)
	{
		if(notes_collected==0) //none found, user cancelled by button
		{
			for(int i=0;i<=tones_to_find;i++)
			{
				found_tones[i] = 0;
			}
		}
		else
		{
			int f_ptr = 0;
			for(int n=0;n<notes_collected;n++)
			{
				//found_tones[f_ptr++] = 300 + fil->chord->key_numbers_to_note_codes[collected_notes[n]];
				found_tones[f_ptr++] = 301 + collected_notes[n];
			}
		}
	}
}
#endif

void Stylus_test()
{
	while(1)
	{
		if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_4)==1)
		{
			LED_SIG_ON;
		}
		else
		{
			LED_SIG_OFF;
		}
		if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_5)==1)
		{
			LED_O4_0_ON;
		}
		else
		{
			LED_O4_0_OFF;
		}
		if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_6)==1)
		{
			LED_O4_1_ON;
		}
		else
		{
			LED_O4_1_OFF;
		}
		if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_8)==1)
		{
			LED_O4_2_ON;
		}
		else
		{
			LED_O4_2_OFF;
		}
		if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_9)==1)
		{
			LED_O4_3_ON;
		}
		else
		{
			LED_O4_3_OFF;
		}

		if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_8)==1)
		{
			LED_R8_0_ON;
		}
		else
		{
			LED_R8_0_OFF;
		}
		if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_9)==1)
		{
			LED_R8_1_ON;
		}
		else
		{
			LED_R8_1_OFF;
		}
		if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_10)==1)
		{
			LED_R8_2_ON;
		}
		else
		{
			LED_R8_2_OFF;
		}
		if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_11)==1)
		{
			LED_R8_3_ON;
		}
		else
		{
			LED_R8_3_OFF;
		}
		if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_12)==1)
		{
			LED_R8_4_ON;
		}
		else
		{
			LED_R8_4_OFF;
		}
		if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_13)==1)
		{
			LED_R8_5_ON;
		}
		else
		{
			LED_R8_5_OFF;
		}
		if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_14)==1)
		{
			LED_R8_6_ON;
		}
		else
		{
			LED_R8_6_OFF;
		}
		if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_15)==1)
		{
			LED_R8_7_ON;
		}
		else
		{
			LED_R8_7_OFF;
		}
	}
}

/*
int IR_remote_capture()
{
	//AUTOCORRELATION_PREAMP_BOOST_MIC = 12

	int signal;
	int silence = -1; //parked state
	int edges_hilo = 0, edges_lohi = 0;
	int last_signal_state = 0;

	while(1) //wait for some signal
	{
		signal = ADC2_read();

		if(signal<3030) //signal detected now
		{
			signal++;
			silence = 0;
			LED_O4_1_ON;

			if(last_signal_state==0) //in previous loop signal was not detected
			{
				edges_lohi++;
			}

			last_signal_state = 1;
		}
		else //signal not detected
		{
			LED_O4_1_OFF;

			if(last_signal_state==1) //in previous loop signal was detected
			{
				edges_hilo++;
			}

			last_signal_state = 0;
		}
		if(silence>=0)
		{
			silence++;
			if(silence%2==0)
			{
				LED_O4_2_OFF;
			}
			else
			{
				LED_O4_2_ON;
			}
		}
		if(silence==10000) //signal finished
		{
			silence=-1;
			LED_O4_3_ON;
			edges_lohi=0;
			edges_hilo=0;
		}
	}
}
*/

void check_battery_level()
{
	//#define LOW_BAT_THRESHOLD 1600.0f
	#define LOW_BAT_THRESHOLD 1500.0f
	#define LOW_BAT_INDICATOR_OFFSET 1400.0f
	#define LOW_BAT_INDICATOR_RANGE (LOW_BAT_THRESHOLD - LOW_BAT_INDICATOR_OFFSET)

	ADC_configure_VBAT_channel();

	int val, loops=100;
	float sum = 0;

	for(int i=0; i<loops; i++)
	{
		val = ADC1_read();
		sum += val;
		Delay(2);
	}
	sum /= loops;

	//----------------------------------------------------------------
	//1817,1834,1821 @ 3.48V
	//1713,1712,1709 @ 3.10V
	//1442,1454,1447 @ 2.40V
	//1503,1515,1511 @ 2.54V

	//----------------------------------------------------------------
	//@threshold 1600.0f:

	//2.70 -> ok
	//2.68 -> low(8) - total 3.80, plays fine
	//2.27 -> low(4) - total 3.40, does not play well
	//2.36 -> low(3) - total 3.53, does not play well
	//2.60 -> low(6) - total 3.88, plays fine
	//2.55 -> low(5) - total 3.82, plays fine
	//2.53 -> low(5) - total 3.79, plays fine

	//----------------------------------------------------------------
	//@threshold 1500.0f:

	//2.52 -> ok - total 3.76, plays fine
	//2.51 -> low(8) - total 3.75, still plays fine but can hear motorboating now when all LEDs lit (1481,1489,1502)
	//2.49 -> low(7) - total 3.72, still plays fine enough, but overloads slightly, also motorboating is quite noticeable (1485,1481,1486)

	//2AA  3AA  DIO  STAB
	//2.52 3.76 3.40 3.26
	//2.49 3.72 3.37 3.26

	//----------------------------------------------------------------

	if(sum < LOW_BAT_THRESHOLD)
	{
		sum -= LOW_BAT_INDICATOR_OFFSET;

		for(int i=0; i<8; i++)
		{
			LED_R8_0_ON;
			if(sum > LOW_BAT_INDICATOR_RANGE/8) LED_R8_1_ON;
			if(sum > LOW_BAT_INDICATOR_RANGE/8 * 2) LED_R8_2_ON;
			if(sum > LOW_BAT_INDICATOR_RANGE/8 * 3) LED_R8_3_ON;
			if(sum > LOW_BAT_INDICATOR_RANGE/8 * 4) LED_R8_4_ON;
			if(sum > LOW_BAT_INDICATOR_RANGE/8 * 5) LED_R8_5_ON;
			if(sum > LOW_BAT_INDICATOR_RANGE/8 * 6) LED_R8_6_ON;
			if(sum > LOW_BAT_INDICATOR_RANGE/8 * 7) LED_R8_7_ON;

			Delay(40);

			LED_R8_all_OFF();
			Delay(40);
		}

	}
}

void show_board_serial_no()
{
	display_BCD_numbers((char*)BINARY_ID+12,3);
	while(1);
}

void show_firmware_version()
{
	char version[5];
	strncpy(version,FW_VERSION+4,1); //copy over the major version number in compact form
	strncpy(version+1,FW_VERSION+6,3);
	display_BCD_numbers(version,4);
	while(1);
}

void acoustic_location_test()
{
	#define N_SAMPLES 512 //256 //I2S_AUDIOFREQ/2
	float *capture_buf_l, *capture_buf_r;

	capture_buf_l = (float*)malloc(N_SAMPLES * sizeof(float));
	capture_buf_r = (float*)malloc(N_SAMPLES * sizeof(float));

	KEY_LED_all_off();
	codec_reset();
	I2S_Cmd(CODEC_I2S, ENABLE);

	LED_SIG_OFF;
	LED_RDY_OFF;
	LED_O4_all_OFF();

	while(1)
	{
		LED_SIG_ON;
		for(int i=0;i<N_SAMPLES;i++)
		{
			capture_buf_r[i] = (float)(4096/2 - (int16_t)ADC1_read()) / 4096.0f * PREAMP_BOOST;
			capture_buf_l[i] = (float)(4096/2 - (int16_t)ADC2_read()) / 4096.0f * PREAMP_BOOST;

			//timing by Codec
			while (!SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE));
			SPI_I2S_SendData(CODEC_I2S, 0);
		}
		LED_SIG_OFF;

		#define TEST_RANGE 24
		float differences[TEST_RANGE];

		for(int test_pos = 0;test_pos<=TEST_RANGE;test_pos++)
		{
			differences[test_pos] = 0;
			int shift = test_pos - TEST_RANGE/2;
			int from,to;

			if(shift==0)
			{
				from = 0;
				to = N_SAMPLES;
			}
			else if(shift<0)
			{
				from = -shift;
				to = N_SAMPLES;
			}
			else if(shift>0)
			{
				from = 0;
				to = N_SAMPLES-shift;
			}

			for(int i=from;i<to;i++)
			{
				differences[test_pos] += fabs(capture_buf_l[i] - capture_buf_r[i+shift]);
			}
		}

		//detect sound by simply finding minimum
		//int minimum_pos = find_minimum(differences,TEST_RANGE);

		//better results, if we require all correlation values to be below threshold
		//int minimum_pos = find_minimum_with_threshold(differences,TEST_RANGE,1400);
		int minimum_pos = find_minimum_with_threshold_f(differences,TEST_RANGE,100.0f);

		LED_RDY_ON;
		LED_R8_all_OFF();

		if(minimum_pos>=0) //if any sound detected
		{
			int to_scale = minimum_pos / (TEST_RANGE / 8) + 1;

			if(to_scale>7) { LED_R8_7_ON; }
			else if(to_scale>6) { LED_R8_6_ON; }
			else if(to_scale>5) { LED_R8_5_ON; }
			else if(to_scale>4) { LED_R8_4_ON; }
			else if(to_scale>3) { LED_R8_3_ON; }
			else if(to_scale>2) { LED_R8_2_ON; }
			else if(to_scale>1) { LED_R8_1_ON; }
			else if(to_scale>0) { LED_R8_0_ON; }

			Delay(100);
		}

	} //end while(1)

	//never reached normally
	free(capture_buf_l);
	free(capture_buf_r);

	if(ANY_USER_BUTTON_ON)
	{
		return;
	}
}

void direct_buttons_LEDs_test()
{
	//direct buttons & LEDs test

	int cnt=0;
	while(1)
	{
		cnt++;
		if(BUTTON_U1_ON)
		{
			if(cnt<500)
			{
				if(cnt%100==0)
				{
					LED_RDY_ON;
				}
				if(cnt%100==50)
				{
					LED_RDY_OFF;
				}
			}
			else
			{
				while(BUTTON_U1_ON)
				{
					LED_R8_0_ON;
					Delay(100);
					LED_R8_0_OFF;
					Delay(100);

					LED_R8_1_ON;
					Delay(100);
					LED_R8_1_OFF;
					Delay(100);

					LED_R8_2_ON;
					Delay(100);
					LED_R8_2_OFF;
					Delay(100);

					LED_R8_3_ON;
					Delay(100);
					LED_R8_3_OFF;
					Delay(100);

					LED_R8_4_ON;
					Delay(100);
					LED_R8_4_OFF;
					Delay(100);

					LED_R8_5_ON;
					Delay(100);
					LED_R8_5_OFF;
					Delay(100);

					LED_R8_6_ON;
					Delay(100);
					LED_R8_6_OFF;
					Delay(100);

					LED_R8_7_ON;
					Delay(100);
					LED_R8_7_OFF;
					Delay(100);
				}
			}
		}
		else if(BUTTON_U2_ON)
		{
			if(cnt<500)
			{
				if(cnt%50==0)
				{
					LED_RDY_ON;
				}
				if(cnt%50==25)
				{
					LED_RDY_OFF;
				}
			}
			else
			{
				while(BUTTON_U2_ON)
				{
					LED_O4_0_ON;
					Delay(100);
					LED_O4_0_OFF;
					Delay(100);

					LED_O4_1_ON;
					Delay(100);
					LED_O4_1_OFF;
					Delay(100);

					LED_O4_2_ON;
					Delay(100);
					LED_O4_2_OFF;
					Delay(100);

					LED_O4_3_ON;
					Delay(100);
					LED_O4_3_OFF;
					Delay(100);
				}
			}
		}
		else if(BUTTON_U3_ON)
		{
			if(cnt<500)
			{
				if(cnt%24==0)
				{
					LED_RDY_ON;
				}
				if(cnt%24==12)
				{
					LED_RDY_OFF;
				}
			}
			else
			{
				while(BUTTON_U3_ON)
				{
					LED_B5_0_ON;
					Delay(100);
					LED_B5_0_OFF;
					Delay(100);

					LED_B5_1_ON;
					Delay(100);
					LED_B5_1_OFF;
					Delay(100);

					LED_B5_2_ON;
					Delay(100);
					LED_B5_2_OFF;
					Delay(100);

					LED_B5_3_ON;
					Delay(100);
					LED_B5_3_OFF;
					Delay(100);

					LED_B5_4_ON;
					Delay(100);
					LED_B5_4_OFF;
					Delay(100);
				}
			}
		}
		else if(BUTTON_U4_ON)
		{
			if(cnt<500)
			{
				if(cnt%12==0)
				{
					LED_RDY_ON;
				}
				if(cnt%12==6)
				{
					LED_RDY_OFF;
				}
			}
			else
			{
				while(BUTTON_U4_ON)
				{
					LED_W8_0_ON;
					Delay(100);
					LED_W8_0_OFF;
					Delay(100);

					LED_W8_1_ON;
					Delay(100);
					LED_W8_1_OFF;
					Delay(100);

					LED_W8_2_ON;
					Delay(100);
					LED_W8_2_OFF;
					Delay(100);

					LED_W8_3_ON;
					Delay(100);
					LED_W8_3_OFF;
					Delay(100);

					LED_W8_4_ON;
					Delay(100);
					LED_W8_4_OFF;
					Delay(100);

					LED_W8_5_ON;
					Delay(100);
					LED_W8_5_OFF;
					Delay(100);

					LED_W8_6_ON;
					Delay(100);
					LED_W8_6_OFF;
					Delay(100);

					LED_W8_7_ON;
					Delay(100);
					LED_W8_7_OFF;
					Delay(100);
				}
			}
		}
		else
		{
			cnt=0;
		}
		Delay(10);
	}
}
