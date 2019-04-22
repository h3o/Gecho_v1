/*
 * AudioLoopNoFilters.cpp
 *
 *  Created on: Sep 04, 2017
 *      Author: mayo
 *
 *  This file is part of the Gecho Loopsynth Firmware Development Framework.
 *  It can be used within the terms of CC-BY-NC-SA license.
 *  It must not be distributed separately.
 *
 *  Find more information at: http://gechologic.com/gechologists/
 */

#include <AudioLoopNoFilters.h>
#include <Channels.h>
#include <Interface.h>
#include <hw/controls.h>
#include <hw/sensors.h>
#include <hw/leds.h>

void audio_loop_no_filters()
{
	LED_R8_all_OFF();
	LED_O4_all_OFF();
	LED_B5_all_OFF();
	LED_W8_all_OFF();

	while(run_program)
	{
		if(PROG_enable_LED_indicators_sequencer)
		{
			LED_sequencer_indicators(0,0);
		}

		if (sampleCounter & 0x00000001) //stereo sample - left channel
		{
			#ifndef CODEC_TLV
			//Start the ADC conversion
			ADC_SoftwareStartConv(ADC1); //left mic or pickups input
			ADC_SoftwareStartConv(ADC2); //right mic or pickups input
			#else
			if(PROG_add_OpAmp_ADC12_signal)
			{
				sample_f[0] = (float)(ADC_sample_recv) * OpAmp_ADC12_signal_conversion_factor;
			}
			#endif

			if(PROG_mix_sample_from_flash)
			{
				random_value = 0; //remove white noise

				sample_f[0] += ( (float)(32768 - (int16_t)mixed_sample_buffer[mixed_sample_buffer_ptr_L++]) / 32768.0f) * noise_volume * noise_boost_by_sensor;

				if(mixed_sample_buffer_ptr_L == MIXED_SAMPLE_BUFFER_LENGTH)
				{
					mixed_sample_buffer_ptr_L = 0;
				}
			}

			/*if(PROG_enable_filters)
			{
				sample_mix = IIR_Filter::iir_filter_multi_sum_w_noise(
					sample_f[0],
					fil->iir2,
					ACTIVE_FILTERS_PAIRS,
					fil->fp.mixing_volumes,
					(int16_t)random_value,
					noise_volume * noise_boost_by_sensor
				) * MAIN_VOLUME;
			}
			else //if not PROG_enable_filters
			{*/
				if(PROG_add_plain_noise)
				{
					//mix in noise signal
					sample_f[0] += ( (float)(32768 - (int16_t)random_value) / 32768.0f) * noise_volume * noise_boost_by_sensor;
				}
				sample_mix = sample_f[0] * UNFILTERED_SIGNAL_VOLUME;
			//}
		}
		else //stereo sample - right channel
		{
			rnd_f = PseudoRNG1a_next_float();
			//float r = PseudoRNG1b_next_float();
			memcpy(&random_value, &rnd_f, sizeof(random_value));
			//random_value = PseudoRNG2_next_int32();
			//PseudoRNG_next_value(&random_value); //load next random value to the variable

			if(PROG_add_OpAmp_ADC12_signal)
			{
				#ifndef CODEC_TLV
				//mix in signal from microphone or pickups
				//sample_f[0] = (float)(4096/2 - (int16_t)ADC1_read()) / 4096.0f * PREAMP_BOOST;
				//sample_f[1] = (float)(4096/2 - (int16_t)ADC2_read()) / 4096.0f * PREAMP_BOOST;

				//optimization
				//sample_f[0] = (float)(4096/2 - (int16_t)ADC1->DR) / 4096.0f * PREAMP_BOOST; //left stereo channel
				//sample_f[1] = (float)(4096/2 - (int16_t)ADC2->DR) / 4096.0f * PREAMP_BOOST; //right stereo channel

				//dynamic control
				sample_f[0] = (float)(2048 - (int16_t)ADC1->DR) * OpAmp_ADC12_signal_conversion_factor; //left stereo channel
				sample_f[1] = (float)(2048 - (int16_t)ADC2->DR) * OpAmp_ADC12_signal_conversion_factor; //right stereo channel
				#else
				//sample_f[1] = (float)(32000-ADC_sample_recv) * OpAmp_ADC12_signal_conversion_factor; //left stereo channel
				//sample_f[1] = (float)(ADC_sample_recv) * 0.001f;
				sample_f[1] = (float)(ADC_sample_recv) * OpAmp_ADC12_signal_conversion_factor;
				#endif
			}
			else
			{
				/*
				if(PROG_add_plain_noise)
				{
					//mix in noise signal
					sample_f[0] = ( (float)(32768 - (int16_t)random_value) / 32768.0f) * noise_volume * noise_boost_by_sensor;
					sample_f[1] = ( (float)(32768 - (int16_t)(random_value>>16)) / 32768.0f) * noise_volume * noise_boost_by_sensor;
				}
				else
				{
				*/
				if(PROG_audio_input_magnetic_sensor)
				{
					sample_f[0] = (float)(4096 - (int16_t)ADC1_read()) / 4096.0f * MAG_SENSOR_SIGNAL_BOOST;
					sample_f[1] = sample_f[0];
				}
				else
				{
					sample_f[0] = 0;
					sample_f[1] = 0;
				}
			}

			if(PROG_mix_sample_from_flash)
			{
				random_value = 0; //remove white noise

				sample_f[1] += ( (float)(32768 - (int16_t)mixed_sample_buffer[mixed_sample_buffer_ptr_R++]) / 32768.0f) * noise_volume * noise_boost_by_sensor;

				if(mixed_sample_buffer_ptr_R == MIXED_SAMPLE_BUFFER_LENGTH)
				{
					mixed_sample_buffer_ptr_R = 0;
				}
			}

			/*if(PROG_enable_filters)
			{
				sample_mix = IIR_Filter::iir_filter_multi_sum_w_noise(
					sample_f[1],
					fil->iir2 + FILTERS / 2,
					ACTIVE_FILTERS_PAIRS,
					fil->fp.mixing_volumes + FILTERS / 2,
					(int16_t)random_value >> 16,
					noise_volume * noise_boost_by_sensor
				) * MAIN_VOLUME;
			}
			else //if not PROG_enable_filters
			{*/
				if(PROG_add_plain_noise)
				{
					//mix in noise signal
					sample_f[1] += ( (float)(32768 - (int16_t)(random_value>>16)) / 32768.0f) * noise_volume * noise_boost_by_sensor;
				}
				sample_mix = sample_f[1] * UNFILTERED_SIGNAL_VOLUME;
			//}
		}

		//if(sample_mix > smx0)
		//{
		//	smx0 = sample_mix;
		//}
		//if(sample_mix < smx1)
		//{
		//	smx1 = sample_mix;
		//}

		sample_mix *= COMPUTED_SAMPLES_MIXING_VOLUME;

		if(sample_mix > COMPUTED_SAMPLE_MIXING_LIMIT_UPPER)
		{
			sample_mix = COMPUTED_SAMPLE_MIXING_LIMIT_UPPER;
		}

		if(sample_mix < COMPUTED_SAMPLE_MIXING_LIMIT_LOWER)
		{
			sample_mix = COMPUTED_SAMPLE_MIXING_LIMIT_LOWER;
		}

		sample_i16 = (int16_t)(sample_mix);

		if(PROG_drum_kit_with_echo)
		{
			//sample_i16 += drum_kit_process();
		}

		if(PROG_add_echo)
		{
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
			echo_mix_f = float(sample_i16) + float(echo_buffer[echo_buffer_ptr]) * ECHO_MIXING_GAIN_MUL / ECHO_MIXING_GAIN_DIV;

			if(echo_mix_f > COMPUTED_SAMPLE_MIXING_LIMIT_UPPER)
			{
				echo_mix_f = COMPUTED_SAMPLE_MIXING_LIMIT_UPPER;
			}

			if(echo_mix_f < COMPUTED_SAMPLE_MIXING_LIMIT_LOWER)
			{
				echo_mix_f = COMPUTED_SAMPLE_MIXING_LIMIT_LOWER;
			}

			sample_i16 = (int16_t)echo_mix_f;

			//store result to echo, the amount defined by a fragment
			//echo_mix_f = ((float)sample_i16 * ECHO_MIXING_GAIN_MUL / ECHO_MIXING_GAIN_DIV);
			//echo_mix_f *= ECHO_MIXING_GAIN_MUL / ECHO_MIXING_GAIN_DIV;
			//echo_buffer[echo_buffer_ptr0] = (int16_t)echo_mix_f;
			echo_buffer[echo_buffer_ptr0] = sample_i16;
		}

		/*
		if(PROG_drum_kit)
		{
			sample_i16 += drum_kit_process();
			//sample_i16 += drum_machine_process_cv_trigger();
		}

		if(PROG_wavetable_sample)
		{
			sample_i16 += flash_sample_process(-1.0f);
		}
		*/

		/*
		if(PROG_melody_by_MIDI)
		{
			//if ((sampleCounter%(2*I2S_AUDIOFREQ/5)==333)) //every 200ms at sample #333
			//{

		    // Wait until data is received
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
		    			}
		    			else if(MIDI_new_volume==0)
		    			{
		    				MIDI_new_volume = data;
		    			}
		    		}
		    	}
		    }

		    if (TIMING_BY_SAMPLE_EVERY_25_MS == 345) //40Hz periodically, at sample #345
		    {
		    	if(MIDI_new_note)
		    	{
		    		if(MIDI_new_volume > 0)
		    		{
		    			//MIDI_new_note_freq = NOTE_FREQ_A4 * pow(HALFTONE_STEP_COEF, MIDI_new_note - 81);
		    			MIDI_new_note_freq = MIDI_notes_to_freq[MIDI_new_note];
		    		}
		    		MIDI_new_note = 0;
		    		MIDI_new_volume = 0;
		    	}
		    }

		    if (TIMING_BY_SAMPLE_EVERY_25_MS == 350) //40Hz periodically, at sample #350
		    {
		    	if(MIDI_new_note_freq)
		    	{
		    		flash_sample_process(MIDI_new_note_freq);
		    		MIDI_new_note_freq = 0;
		    	}
		    }
		    //}
		}
		*/

		/*
		if(PROG_melody_by_USART)
		{
			if(usart_command_received)
			{
				if(0==strncmp(usart_command,"FREQ=",5))
				{
					int freq = atof(usart_command+5);
					flash_sample_process(freq);
				}
				else if(0==strncmp(usart_command,"EXIT",4))
				{
					run_program = 0;
				}
				else if(0==strncmp(usart_command,"BTN=RST",7))
				{
					NVIC_SystemReset();
				}

	    		usart_command_received = 0;
			}
		}
		*/

		/*
		if(PROG_enable_filters)
		{
			if(PROG_enable_chord_loop)
			{
				//melody
				if (TIMING_BY_SAMPLE_EVERY_250_MS == SHIFT_MELODY_NOTE_TIMING_BY_SAMPLE) //4Hz periodically, at given sample
				{
					if(PROG_wavetable_sample)
					{
						current_melody_freq = fil->chord->get_current_melody_freq();
						flash_sample_process(current_melody_freq);
						fil->start_next_melody_note();
					}
					else
					{
						if(fil->fp.melody_filter_pair >= 0)
						{
							fil->start_next_melody_note();
							current_melody_LED = fil->chord->get_current_melody_note();
							display_chord(&current_melody_LED, 1);
						}
					}
				}

				if (TIMING_BY_SAMPLE_EVERY_250_MS == 2400) //4Hz periodically, at sample #2400
				{
					if(fil->fp.melody_filter_pair >= 0)
					{
						display_chord(NULL, 0);
					}
				}

				if((seconds % SHIFT_CHORD_INTERVAL == 0) && (sampleCounter==SHIFT_CHORD_TIMING_BY_SAMPLE)) //test - every 2 seconds, at given sample
				{
					current_chord_LEDs = fil->chord->get_current_chord_LED();
					display_chord(current_chord_LEDs, 3);
					fil->start_next_chord();
				}
			}

			if(PROG_noise_effects)
			{
				custom_effect_filter_process();
			}
		}
		*/

		/*
		if(PROG_enable_rhythm)
		{
			if (sampleCounter % (I2S_AUDIOFREQ/(progressive_rhythm_factor*2)) == 0)
			{
				LEDs_ORANGE_off();
			}

			if (sampleCounter % (I2S_AUDIOFREQ/progressive_rhythm_factor) == 0)
			{
				noise_volume = noise_volume_max;
				LEDs_ORANGE_next(progressive_rhythm_factor);
			}
			else
			{
				if(noise_volume>noise_volume_max/PROGRESSIVE_RHYTHM_BOOST_RATIO)
				{
					noise_volume *= PROGRESSIVE_RHYTHM_RAMP_COEF;
				}
			}
		}
		else
		{
			if (sampleCounter % (I2S_AUDIOFREQ) == 0)
			{
				noise_volume = noise_volume_max;
			}
		}
		*/

		//if(sample_i16 > si0)
		//{
		//	si0 = sample_i16;
		//}
		//if(sample_i16 < si1)
		//{
		//	si1 = sample_i16;
		//}
		//sample_u16 = sample_i16;

		//send sample to codec
		while (!SPI_I2S_GetFlagStatus(CODEC_I2S, SPI_I2S_FLAG_TXE));
        //SPI_I2S_SendData(CODEC_I2S, sample_u16);
        SPI_I2S_SendData(CODEC_I2S, sample_i16);

		//while (SPI_I2S_GetFlagStatus (CODEC_I2SEXT, SPI_I2S_FLAG_RXNE) == RESET);
        /* Data Received through I2Sx_ext SD pin */
    	//while (SPI_I2S_GetFlagStatus(CODEC_I2SEXT, SPI_I2S_FLAG_RXNE ) != SET);
        //while (SPI_I2S_GetFlagStatus(CODEC_I2SEXT, SPI_FLAG_RXNE) == RESET);
		ADC_sample_recv = SPI_I2S_ReceiveData(CODEC_I2SEXT);
		//ADC_sample_recv = SPI_I2S_ReceiveData(CODEC_I2S); //seems to cause feedback by receiving the same digital data back

		sampleCounter++;

    	if (TIMING_BY_SAMPLE_ONE_SECOND_W_CORRECTION) //one full second passed
    	{
    		seconds++;
    		sampleCounter = 0;

    		/*if(PROG_enable_filters)
    		{
    			if(PROG_enable_rhythm)
    			{
    				//after looping through all the chords twice, increment rhythm timing by one
    				if(seconds%(fil->chord->total_chords*2*2)==0)
    				{
    					progressive_rhythm_factor++;
    					if(progressive_rhythm_factor==5)
    					{
    						progressive_rhythm_factor = 1;
    					}
    					LEDs_ORANGE_reset();
    				}
    			}
    		}*/
    	}

    	if ((TIMING_BY_SAMPLE_EVERY_100_MS==1234) && PROG_buttons_controls_during_play) //10Hz periodically, at sample #1234
    	{
    		buttons_controls_during_play();
    	}

    	/*if(PROG_enable_filters)
    	{
    		if (sampleCounter%(2*I2S_AUDIOFREQ/PROGRESS_UPDATE_FILTERS_RATE)==57) //every X ms, at 1.29 ms (57/44100)
    		{
    			fil->progress_update_filters(fil, seconds%2==0);
    		}
    	}*/

		#ifdef USE_IR_SENSORS
			if (TIMING_BY_SAMPLE_EVERY_10_MS == 0) //100Hz periodically, at 0ms
			{
				//only if IR sensors are not used as direct audio input
				if(!PROG_audio_input_IR_sensors && ADC_process_sensors()==1)
				{
					ADC_last_result[2] = -ADC_last_result[2];
					ADC_last_result[3] = -ADC_last_result[3];
					CLEAR_ADC_RESULT_RDY_FLAG;

					sensors_loop++;

					if(PROG_enable_LED_indicators_IR_sensors)
					{
						IR_sensors_LED_indicators(ADC_last_result);
					}
					IR_sensors_level_process(ADC_last_result);

					/*if(PROG_enable_S3_control_resonance)
					{
						if(sensors_loop%FILTERS == fil->fp.melody_filter_pair || sensors_loop%FILTERS == fil->fp.melody_filter_pair + CHORD_MAX_VOICES)
						{
							//fil->iir2[sensors_loop%FILTERS].setResonance(0.999f);
						}
						else
						{
							if(ADC_last_result[2] > 400)
							{
								fil->iir2[sensors_loop%FILTERS].setResonance(0.999f);
							}
							else if(ADC_last_result[2] > 200)
							{
								fil->iir2[sensors_loop%FILTERS].setResonance(0.996f);
							}
							else if(ADC_last_result[2] > 100)
							{
								fil->iir2[sensors_loop%FILTERS].setResonance(0.993f);
							}
							else
							{
								fil->iir2[sensors_loop%FILTERS].setResonance(0.99f);
							}
						}
					}*/
				}

				if (TIMING_BY_SAMPLE_EVERY_83_MS==441) //every 83ms, at sample #441
				{
					/*if(PROG_enable_S4_control_arpeggiator)
					{
						if(ADC_last_result[3] > 400)
						{
							if(fil->fp.arpeggiator_filter_pair==-1)
							{
								fil->fp.arpeggiator_filter_pair = DEFAULT_ARPEGGIATOR_FILTER_PAIR;
								//fil->iir2[0].setCutoff(fil->chord->chords[fil->chord->current_chord].freqs[0]);
								//fil->iir2[arpeggiator_pair].setResonanceKeepFeedback(0.999f);
								//fil->iir2[arpeggiator_pair+FILTERS/2].setResonanceKeepFeedback(0.999f);
							}

							//fil->fp.melody_filter_pair = 0;
							int next_chord = fil->chord->current_chord-1;
							if(next_chord > fil->chord->total_chords)
							{
								next_chord = 0;
							}
							else if(next_chord < 0)
							{
								next_chord = fil->chord->total_chords-1;
							}

							fil->add_to_update_filters_pairs(0,fil->chord->chords[next_chord].freqs[arpeggiator_loop]);

							arpeggiator_loop++;
							if(arpeggiator_loop==CHORD_MAX_VOICES)
							{
								arpeggiator_loop=0;
							}

							if(ADC_last_result[3] > 800)
							{
								fil->fp.mixing_volumes[fil->fp.arpeggiator_filter_pair] = 2.0f;
								fil->fp.mixing_volumes[fil->fp.arpeggiator_filter_pair+FILTERS/2] = 2.0f;
							}
						}
						else
						{
							if(fil->fp.arpeggiator_filter_pair>=0)
							{
								fil->fp.arpeggiator_filter_pair = -1;
								//back to default resonance
								//fil->iir2[fil->fp.arpeggiator_filter_pair].setResonanceKeepFeedback(0.99f);
								//fil->iir2[fil->fp.arpeggiator_filter_pair+FILTERS/2].setResonanceKeepFeedback(0.99f); //refactor

								//volume back to normal
								fil->fp.mixing_volumes[fil->fp.arpeggiator_filter_pair] = 1.0f;
								fil->fp.mixing_volumes[fil->fp.arpeggiator_filter_pair+FILTERS/2] = 1.0f;
							}
							//fil->fp.melody_filter_pair = -1;
						}
					}*/
				}

				/*if(PROG_enable_S4_control_arpeggiator)
				{
					if (sampleCounter%(2*I2S_AUDIOFREQ/PROGRESS_UPDATE_FILTERS_RATE)==27) //every X ms, at 0.213 ms
					{
						fil->progress_update_filters(fil, false);
					}
				}*/
			}
		#endif

		if (TIMING_BY_SAMPLE_EVERY_100_MS == 3) //10Hz periodically, at sample #3
		{
			check_for_reset();
		}

		if(PROG_magnetic_sensor_test && !PROG_audio_input_magnetic_sensor) //only if not used as audio, might interfere with ADC conversion
		{
			magnetic_sensor_latest_value = ADC1_read();
			if(PROG_magnetic_sensor_test_display)
			{
				MAGNETIC_sensors_test(magnetic_sensor_latest_value);
			}
		}
		queue_codec_ctrl_process();
	}

}
