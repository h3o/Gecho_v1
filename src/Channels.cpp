/*
 * Channels.cpp
 *
 *  Created on: Nov 26, 2016
 *      Author: mayo
 */

#include <Channels.h>
#include <Interface.h>
#include <songs.h>
#include <extensions/Chaos.h>
#include <extensions/DCO_Synth.h>
#include <extensions/Granular.h>
#include <extensions/Reverb.h>
#include <extensions/Bytebeat.h>
#include <extensions/Samples.h>
#include <extensions/Sequencer.h>
#include <extensions/MIDI.h>
#include <hw/controls.h>
#include <hw/pi.h>
#include <hw/sensors.h>
#include <hw/leds.h>
#include <hw/sha1.h>
#include <hw/eeprom.h>
#include <hw/flash.h>
#include <stdio.h>
#include <string.h>

uint64_t channel;

float chaotic_coef = 1.0f;
float dc_offset_sum = 0.0f, dc_offset = 0.0f;
int dc_offset_cnt = 0;

int base_freq, param_i;
float param_f;
int waves_freqs[WAVES_FILTERS];

char *melody_str = NULL;
char *progression_str = NULL;
int progression_str_length;
char *settings_str = NULL;

//int first_program_run = 1;

int mixed_sample_buffer_ptr_L, mixed_sample_buffer_ptr_R;

int16_t *mixed_sample_buffer;
int MIXED_SAMPLE_BUFFER_LENGTH;

void custom_program_init(uint64_t prog)
{
	if (prog % 10 == 5) //the SET button was held long
	{
		prog /= 10;

		alt_mode_hi_pass_with_water();
	}

	//-----------------------------------------------------------------
	//parse longer command sequences

	char prog_str[PRESS_ORDER_MAX+2];
	char *prog_str_ptr;
	//modifiers like "%llu" or "%I64u" don't work here

	/*
	sprintf(prog_str, "%u", (unsigned int)(prog/1000000000000));
	sprintf(prog_str+4, "%u", (unsigned int)((prog/100000000)%10000));
	sprintf(prog_str+8, "%u", (unsigned int)((prog/10000)%10000));
	sprintf(prog_str+12, "%u", (unsigned int)(prog%10000));
	*/

	uint64_t pr = prog;
	prog_str[PRESS_ORDER_MAX+1] = 0;
	for (int i = PRESS_ORDER_MAX; i >= 0; i--)
	{
		prog_str[i] = '0' + pr % 10;
		pr /= 10;
	}

	prog_str_ptr = prog_str;
	while (prog_str_ptr[0]=='0')
	{
		prog_str_ptr++;
	}
	//-----------------------------------------------------------------

	//check for activation unlock command

	if (strlen(prog_str_ptr) > 3)
	{
		if (0==strncmp(prog_str_ptr, "444", 3)) //activation unlock starts with 444
		{
			if(verify_activation_unlock_code(prog_str_ptr+3))
			{
				FLASH_erase_all_custom_data();

				//there is more
				play_buffer((uint16_t*)0x08075060, 0x17900, 1); //1=mono
				NVIC_SystemReset();
			}
		}
	}

	if(FLASH_check_activation_lock())
	{
		//a modern equivalent
		play_buffer((uint16_t*)(0x08056B00), 0x1e560, 1); //1=mono
		NVIC_SystemReset();
	}

	int song_melody = channel_to_song_and_melody(prog);
	selected_song = song_melody % 1000;
	selected_melody = song_melody / 1000;

	//--------------------------------------------------------------------------------------
	//load default settings from EEPROM
	//--------------------------------------------------------------------------------------

	if(selected_melody) //only if the melody exists for this channel
	{
		PROG_wavetable_sample = (bool)EEPROM_LoadSettings_B(SETTINGS_WAVETABLE_SAMPLE, 0);
		ACTIVE_FILTERS_PAIRS = FILTER_PAIRS - (PROG_wavetable_sample?1:0); //if needed, get more computation power by disabling one filter pair
	}

	//--------------------------------------------------------------------------------------

	//check if channel number was overridden
	if(FLASH_IsOverriddenChannel(prog))
	{
		if(progression_str!=NULL)
		{
			free(progression_str);
		}
		if(melody_str!=NULL)
		{
			free(melody_str);
		}
		FLASH_LoadOverriddenChannel(prog,&progression_str,&melody_str,&settings_str);
		prog=999;
	}

	if(settings_str) //some settings loaded either by PREVIEW command or from an overridden channel
	{
		if(settings_str[0]=='1') //default settings overridden
		{
			if(settings_str[2]=='0')
			{
				alt_mode_hi_pass_with_water();
			}

			//get input select setting
			PROG_load_input_setting = false;
			PROG_audio_input_microphones = false;
			PROG_audio_input_pickups = false;
			input_mux_current_step = (settings_str[4]-'0');
			if(input_mux_current_step==0)
			{
				PROG_audio_input_microphones = true;
			}
			if(input_mux_current_step==1)
			{
				PROG_audio_input_pickups = true;
			}

			//get tempo before delay setting, as that one depends on this
			tempo_bpm = atoi(settings_str+13);
			if(!tempo_bpm)
			{
				tempo_bpm = 120;
			}
			PROG_load_bpm_setting = false;

			//get delay setting
			PROG_load_echo_setting = false;
			echo_dynamic_loop_current_step = atoi(settings_str+6);
			echo_dynamic_loop_length = get_echo_length(echo_dynamic_loop_current_step);
			PROG_add_echo = (echo_dynamic_loop_length>0); //enable echo only if loop length is not zero

			if(settings_str[9]=='1' && settings_str[11]=='1')
			{
				SHIFT_CHORD_INTERVAL = 2;
			}
			if(settings_str[9]=='2' && settings_str[11]=='1')
			{
				SHIFT_CHORD_INTERVAL = 1;
			}
			if(settings_str[9]=='1' && settings_str[11]=='2')
			{
				SHIFT_CHORD_INTERVAL = 4;
			}
		}
	}

	if(prog==999)
	{
		if(!strlen(progression_str))
		{
			return;
		}

		selected_song = CHANNEL_111_RECENTLY_GENERATED_SONG;
		if(strlen(melody_str))
		{
			selected_song = CHANNEL_112_RECENTLY_GENERATED_SONG_W_MELODY;
		}
		selected_melody = selected_song;
		prog = selected_song;
	}

	codec_ctrl_init();
	I2S_Cmd(CODEC_I2S, ENABLE);

	if(FLASH_IsOverriddenChannel(prog))
	{
	    PROG_enable_S1_control_noise_boost = true;
	    PROG_enable_S2_control_noise_attenuation = true;  //only when rhythm active
		PROG_enable_S3_control_resonance = true;
		PROG_enable_S4_control_arpeggiator = true; //by default all song channels do have arpeggiator
	}

	//-------------------------------------------------------------------------------------------
	// (1-4): interactive demo songs
	//-------------------------------------------------------------------------------------------

	else if (prog <= 4) //channels #1-4 - demo songs
	{
	    PROG_enable_S1_control_noise_boost = true;
	    PROG_enable_S2_control_noise_attenuation = true;  //only when rhythm active
		PROG_enable_S3_control_resonance = true;
		PROG_enable_S4_control_arpeggiator = true; //by default all song channels do have arpeggiator

		if (selected_song == 1) //song #1 with melody and hi-pass filters
		{
			alt_mode_hi_pass_with_water();
		}
		if (selected_song == 2) //song #2 with melody (epic ad)
		{
		}
		if (selected_song == 3) //pick random one out of basic songs and play it with low-pass filters
		{
		}
		if (selected_song == 4) //override - song #21 (GITS)
		{
		}
	}

	//-------------------------------------------------------------------------------------------
	// 1x: non-interactive noise-filtering based channels
	//-------------------------------------------------------------------------------------------

	else if (prog == 11) //pure white
	{
		//disable some features
		//PROG_enable_rhythm = false;
		PROG_enable_chord_loop = false;
		PROG_enable_LED_indicators_sequencer = false;
		PROG_add_OpAmp_ADC12_signal = false;
		PROG_add_echo = false; //no echo
		PROG_enable_filters = false; //no filters

		//only noise
		PROG_add_plain_noise = true;
		noise_volume = 4.0f;
	}

	else if (prog == 12) //arctic wind and ice
	{
		LED_O4_all_OFF();
		LED_W8_all_ON();
		song_of_wind_and_ice();
	}

	else if (prog == 13) //waves of the sea
	{
		//disable some features
		//PROG_enable_rhythm = false;
		PROG_enable_chord_loop = false;
		PROG_enable_LED_indicators_sequencer = false;
		PROG_add_OpAmp_ADC12_signal = false;
		PROG_add_echo = false;
		PROG_enable_filters = true;
		//PROG_effect_sea_waves = true;
		PROG_noise_effects = true;
		PROG_add_plain_noise = true;
	}

	else if (prog == 14) //Solaris (test-control freq and reso by buttons - WIP)
	{
		//disable some features
		//PROG_enable_rhythm = false;
		PROG_enable_chord_loop = false;
		PROG_enable_LED_indicators_sequencer = false;
		PROG_add_OpAmp_ADC12_signal = false;
		PROG_add_echo = false;
		PROG_enable_filters = true;
		//PROG_effect_solaris = true;
		PROG_noise_effects = true;
		PROG_add_plain_noise = true;
	}

	//-------------------------------------------------------------------------------------------
	// 2x: interactive noise-filtering based channels
	//-------------------------------------------------------------------------------------------

	else if (prog >= 21 && prog <= 24) //interactive noise/resonance effects
	{
		/*
		//disable any input
		PROG_add_OpAmp_ADC12_signal = false; //microphone or line-in signal, or anything coming via ADC1 and ADC2
		PROG_audio_input_microphones = false;
		PROG_audio_input_pickups = false;
		*/

		//disable some features
		//PROG_enable_rhythm = false;
		PROG_enable_chord_loop = false;
		PROG_enable_LED_indicators_sequencer = false;

		//enable what is needed
		PROG_add_echo = true;
		PROG_enable_filters = true;
		PROG_add_plain_noise = true;
		PROG_noise_effects = true;

	    //set faster rate so all filters get updated in time
		PROGRESS_UPDATE_FILTERS_RATE = 165; //every 6.06 ms, at 0.213 ms

	    /*
		if (prog == 21)	//waves of the sea
	    if (prog == 22)	//nostromo
	    if (prog == 23)	//alien spaceship
	    if (prog == 24)	//combined resonance (4 levels)
		*/
	}

	//-------------------------------------------------------------------------------------------
	// 3x: theremin-like direct play channels and DCO synth
	//-------------------------------------------------------------------------------------------

	else if (prog >= CHANNEL_31_THEREMIN_BY_MAGNETIC_RING && prog <= CHANNEL_32_THEREMIN_BY_IR_SENSORS) //theremin driven by magnetic ring or IR sensors (VIP)
	{
		if(prog == CHANNEL_31_THEREMIN_BY_MAGNETIC_RING)
		{
			//disable any input
			PROG_add_OpAmp_ADC12_signal = false; //microphone or line-in signal, or anything coming via ADC1 and ADC2
			PROG_audio_input_microphones = false;
			PROG_audio_input_pickups = false;

			PROG_magnetic_sensor_test = true; //ADC1 input from magnetic sensor
			PROG_magnetic_sensor_test_display = false; //will display both directions on the keyboard instead
		}
		PROG_enable_LED_indicators_IR_sensors = false; //don't display input from IR either

		//PROG_load_input_setting = false;

		//disable some features
		//PROG_enable_rhythm = false;
		PROG_enable_chord_loop = false;
		PROG_enable_LED_indicators_sequencer = false;

		//enable what is needed
		PROG_add_echo = true;
		PROG_enable_filters = true;
		PROG_add_plain_noise = true;
		PROG_noise_effects = true;

		//set faster rate so all filters get updated in time
		PROGRESS_UPDATE_FILTERS_RATE = 165; //every 6.06 ms, at 0.213 ms

		ACTIVE_FILTERS_PAIRS = 4; //only use first 4 filter pairs to save computation power
	}
	else if (prog >= CHANNEL_33_DCO_SYNTH_DIRECT && prog <= CHANNEL_34_DCO_SYNTH_PROGRAMMABLE) //DCO synth driven by IR sensors
	{
		DCO_Synth *dco = new DCO_Synth();

		if (prog == CHANNEL_33_DCO_SYNTH_DIRECT)
		{
			dco->v1_init();
			dco->v1_play_loop();
		}
		else if (prog == CHANNEL_34_DCO_SYNTH_PROGRAMMABLE)
		{
			GPIO_Init_USART3(USART_MIDI_BAUD_RATE, 1); //configure USART3 for MIDI in & out
			dco->v2_init();
			dco->v2_play_loop();
			//dco->v3_play_loop();
			//dco->v4_play_loop();
		}
	}

	//-------------------------------------------------------------------------------------------
	// 4x: custom song programming modes (VIP)
	//-------------------------------------------------------------------------------------------

	else if (prog >= 41 && prog <= 44)
	{
		fil = new Filters(0); //no selected song
		fil->setup();

		if (prog==41)
		{
			custom_song_programming_mode(CHORDS_CAPTURE_BUTTONS);
		}
		if (prog==42)
		{
			custom_song_programming_mode(CHORDS_CAPTURE_MAGNETIC_RING);
		}
		if (prog==43)
		{
			custom_song_programming_mode(CHORDS_CAPTURE_MIC);
		}
		if (prog==44)
		{
			custom_song_programming_mode(CHORDS_CAPTURE_PICKUPS);

			//will use line-in instead of microphones
			PROG_audio_input_microphones = false;
			PROG_audio_input_pickups = true;
		}

		//translate to required notation
		/*int encoded_length =*/ encode_temp_song_to_notes(temp_song, temp_total_chords, &progression_str); //memory will be allocated for progression_str
		//also store to EEPROM
		EEPROM_StoreSongAndMelody(progression_str, (char*)""); //no melody

		fil->setup(); //reset all filters

		return custom_program_init(CHANNEL_111_RECENTLY_GENERATED_SONG); //roll over to program #111 and play
	}

	//-------------------------------------------------------------------------------------------
	// 11x: user generated songs and Gecho App related channels
	//-------------------------------------------------------------------------------------------

	else if (prog == CHANNEL_111_RECENTLY_GENERATED_SONG || prog == CHANNEL_112_RECENTLY_GENERATED_SONG_W_MELODY) //user captured or automatically generated song
	{
		if (temp_song==NULL && progression_str==NULL)
		{
			//look if there is any song in Backup SRAM
			EEPROM_LoadSongAndMelody(&progression_str, &melody_str);
		}

		if (temp_song!=NULL || progression_str!=NULL)
		{
			selected_song = prog;

			selected_melody = 0;
			if (/*prog == CHANNEL_112_RECENTLY_GENERATED_SONG_W_MELODY &&*/ melody_str!=NULL && strlen(melody_str))
			{
				selected_melody = CHANNEL_112_RECENTLY_GENERATED_SONG_W_MELODY;
			}

		    PROG_enable_S1_control_noise_boost = true;
		    PROG_enable_S2_control_noise_attenuation = true;  //only when rhythm active
			PROG_enable_S3_control_resonance = true;
			PROG_enable_S4_control_arpeggiator = true; //by default all song channels do have arpeggiator

			//alt mode hi-pass filters for better clarity
			//alt_mode_hi_pass_with_water();

			//re-init the codec as previewing while capturing has disabled it
			codec_ctrl_init();
			I2S_Cmd(CODEC_I2S, ENABLE);
		}
	}

	else if (prog == CHANNEL_113_DIRECT_PLAY_BY_USART) //direct play by USART (for Gecho App)
	{
		//disable some features
		//PROG_enable_rhythm = false;
		PROG_enable_chord_loop = false;
		PROG_enable_LED_indicators_sequencer = false;
		PROG_add_OpAmp_ADC12_signal = false;
		PROG_add_echo = false;
		PROG_enable_filters = false; //no filters!
		PROG_add_plain_noise = false;
		PROG_audio_input_microphones = false;

		PROG_wavetable_sample = true;
		PROG_melody_by_USART = true;
	}

	//-------------------------------------------------------------------------------------------
	// 123: settings
	//-------------------------------------------------------------------------------------------

	else if (prog == 123) //set tempo
	{
		set_tempo_by_buttons();
		NVIC_SystemReset();
	}

	//-------------------------------------------------------------------------------------------
	// 222: random progression generator
	//-------------------------------------------------------------------------------------------

	else if (prog == CHANNEL_222_GENERATE_SONG_FROM_NOISE) //random progression
	{
		//progression_str_length = generate_chord_progression(&progression_str);
		//this shifts 19.1919191919191919191919191919191919191919 -> 19.329762269156163

		//if (first_program_run)
		//{
			//set_pseudo_random_seed(19.329762269156163);
			//set_pseudo_random_seed(PseudoRNG1a_next_float());

			//first_program_run = 0;
		//}

		/*
		if (prog == 1112)
		{
			reset_pseudo_random_seed();
		}
		*/

		progression_str_length = get_music_from_noise(&progression_str, &melody_str);

		EEPROM_StoreSongAndMelody(progression_str, melody_str);

		/*
		char test[100];
		strncpy(test,SONG_STORED_ID,100);
		//strncpy(test,MELODY_STORED_ID,100);
		strncpy(test,SONG_STORED_DATA,100);
		test[0] = test[0];
		*/

		//fil = new filters(CHANNEL_112_RECENTLY_GENERATED_SONG_W_MELODY); //select the song
		//fil->filter_setup02();

		//return custom_program_init(CHANNEL_112_RECENTLY_GENERATED_SONG_W_MELODY); //roll over to program #112 and play
		return custom_program_init(CHANNEL_111_RECENTLY_GENERATED_SONG); //roll over to program #111 and play
	}

	//-------------------------------------------------------------------------------------------
	// 23x: drum kits and sequencer
	//-------------------------------------------------------------------------------------------

	else if (prog >= 231 && prog <= 232) //drum kits
	{
		//disable some features
		//PROG_enable_rhythm = false;
		PROG_enable_chord_loop = false;
		PROG_enable_LED_indicators_sequencer = false;
		PROG_add_OpAmp_ADC12_signal = false;
		PROG_enable_filters = false; //no filters!
		PROG_add_plain_noise = false;
		//PROG_audio_input_microphones = false;

		PROG_load_echo_setting = false; //do not reload this setting from EEPROM

		if (prog == 231)
	    {
		    PROG_drum_kit = true;
	    }
	    else if (prog == 232)
	    {
		    PROG_drum_kit_with_echo = true;
	    }
	}
	else if (prog == 233) //drum kit with song #2
    {
        return custom_program_init(12243); //roll over to program #12243 and play
    }
	else if (prog == 234) //drum sequencer
	{
	    drum_sequencer();
	}

	//-------------------------------------------------------------------------------------------
	// 24x: sampling experiments
	//-------------------------------------------------------------------------------------------

	else if (prog == 241) //sampler
	{
		//PROG_enable_rhythm = false;
		PROG_enable_chord_loop = false;
		PROG_enable_LED_indicators_sequencer = false;
		PROG_enable_filters = false;
		PROG_add_plain_noise = false;
		PROG_add_OpAmp_ADC12_signal = true; //mic or line-in signal
		PROG_audio_input_microphones = true;
		PROG_audio_input_pickups = false;
		PROG_add_echo = true;

		loop_type = LOOP_TYPE_NO_FILTERS; //simplified program loop
	}

	//-------------------------------------------------------------------------------------------
	// 31x: song of the pi + some unused channels
	//-------------------------------------------------------------------------------------------

	/*
	else if (prog >= 311 && prog <= 313) //unused for now
	{

	}
	*/

	else if (prog == 314) //will be parsed later, as a long-channel number
	{
		return custom_program_init(3140); //roll over to program #314x and play
	}

	//-------------------------------------------------------------------------------------------
	// 32x: hw-test channels - sensors and LEDs...
	//-------------------------------------------------------------------------------------------

	else if ((prog >= 321 && prog <= 324) || (prog >= 3231 && prog <= 3232))
	{
		//PROG_enable_rhythm = false;
		PROG_enable_chord_loop = false;
		PROG_enable_LED_indicators_sequencer = false;
		PROG_add_echo = false;
		PROG_enable_filters = false;
		PROG_add_plain_noise = false;

		loop_type = LOOP_TYPE_NO_FILTERS; //simplified program loop

		//-------------------------------------------------------------------------------------------
		// 321,322: direct listen to signal coming via magnetic or IR sensors
		//-------------------------------------------------------------------------------------------

		if (prog == 321) //ADC1/ADC2 direct input from IR sensors
		{
			PROG_add_OpAmp_ADC12_signal = true; //microphone or line-in signal, or anything coming via ADC1 and ADC2
			PROG_audio_input_microphones = false;
			PROG_audio_input_pickups = false;
			PROG_audio_input_IR_sensors = true;

			PROG_add_echo = true;
		}

		if (prog == 322) //ADC1 input from magnetic sensor
		{
			PROG_add_OpAmp_ADC12_signal = false; //mic or line-in signal
			PROG_audio_input_microphones = false;
			PROG_audio_input_pickups = false;
			PROG_magnetic_sensor_test = true; //ADC1 input from magnetic sensor instead
			PROG_audio_input_magnetic_sensor = true; //if false, standard magnetic sensor test with LED indication performed instead
		}

		//-------------------------------------------------------------------------------------------
		// 323[x], 324: LED diodes test and magnetic sensor calibration test
		//-------------------------------------------------------------------------------------------

		if (prog == 323) //all LEDs test
		{
			all_LEDs_test(); //randomly switching each LED on and off
		}

		if (prog == 3231) //all LEDs test (1st sequence - one glowin at the time)
		{
			all_LEDs_test_seq1();
		}

		if (prog == 3232) //all LEDs test (2nd sequence - all glowing, one off at the time)
		{
			all_LEDs_test_seq2();
		}

		if (prog == 324) //magnetic sensor test with LEDs
		{
			PROG_add_OpAmp_ADC12_signal = false; //mic or line-in signal
			PROG_audio_input_microphones = false;
			PROG_audio_input_pickups = false;
			PROG_magnetic_sensor_test = true; //ADC1 input from magnetic sensor instead
			//PROG_audio_input_magnetic_sensor = true; //if false, standard magnetic sensor test with LED indication performed instead
		}
	}

	//-------------------------------------------------------------------------------------------
	// 33x: sw test channels - notes input detection, etc.
	//-------------------------------------------------------------------------------------------

	else if (prog == 331) //Goertzel detectors test via mic
	{
		ADC_configure_MIC(BOARD_MIC_ADC_CFG_STRING); //use built-in microphones
		//Goertzel_detectors_capture(0, NULL, GOERTZEL_OCTAVES_678);
		AutoCorrelation_capture(0, NULL, AUTOCORRELATION_PREAMP_BOOST_MIC);
	}

	else if (prog == 332) //Goertzel detectors test via line-in
	{
		//codec_reset();
		ADC_configure_PICKUPS();	//use LineIn instead of microphones
		//Goertzel_detectors_capture(0, NULL, GOERTZEL_OCTAVES_456);
		AutoCorrelation_capture(0, NULL, AUTOCORRELATION_PREAMP_BOOST_LINEIN);
	}

	//else if (prog == 333) //stylus detection test
	//{
		/*
		//codec_reset();
		#ifdef CAN_BLOCK_SWD_DEBUG
			GPIO_init_keboard_as_inputs(true);
		#else
			GPIO_init_keboard_as_inputs(false);
		#endif

		Stylus_test();
		*/
	//}

	//else if (prog == 334) //talkie lib porting test
	//{
		/*
		Talkie *talkie = new Talkie();
		uint16_t *voice_buffer;
		int bytes_rendered;

		voice_buffer = (uint16_t*)malloc(10000);

		bytes_rendered = talkie->render(spHELLO, voice_buffer);
		*/
		//while (1)
		//{
			//play_buffer(voice_buffer, bytes_rendered, 1); //1=mono
			//play_buffer(voice_buffer, bytes_rendered, 2); //2=stereo
			//play_buffer((uint16_t*)0x080c0000, 0x5e40, 1); //1=mono
			//play_buffer((uint16_t*)0x08080000, 0x17900, 1); //1=mono
		//}
		//while (1);
	//}


	//-------------------------------------------------------------------------------------------
	// 34x: acoustic pathways test
	//-------------------------------------------------------------------------------------------

	else if (prog >= 341 && prog <= 344)
	{
		codec_rate = 1;

		//PROG_enable_rhythm = false;
		PROG_enable_chord_loop = false;
		PROG_enable_LED_indicators_sequencer = false;

		PROG_add_echo = false;
		PROG_load_echo_setting = false; //do not reload this setting from EEPROM
		PROG_load_input_setting = false;

		PROG_enable_filters = false;
		PROG_add_plain_noise = false;

		PROG_add_OpAmp_ADC12_signal = true; //mic or line-in signal

		loop_type = LOOP_TYPE_NO_FILTERS; //simplified program loop

		if (prog == 341) //direct monitor from microphones, no echo
		{
			PROG_audio_input_microphones = true;
			PROG_audio_input_pickups = false;
		}

		if (prog == 342) //direct monitor from microphones, with echo
		{
			PROG_audio_input_microphones = true;
			PROG_audio_input_pickups = false;
			PROG_add_echo = true;
		}

		if (prog == 343) //direct monitor from line-in, no echo
		{
			PROG_audio_input_microphones = false;
			PROG_audio_input_pickups = true;
		}

		if (prog == 344) //direct monitor from line-in, with echo
		{
			PROG_audio_input_microphones = false;
			PROG_audio_input_pickups = true;
			PROG_add_echo = true;
		}
	}

	//-------------------------------------------------------------------------------------------
	// 41x: Granular Sampler
	//-------------------------------------------------------------------------------------------

	else if (prog == 411) //granular sampler
	{
		ADC_configure_MIC(BOARD_MIC_ADC_CFG_STRING); //use built-in microphones
		granular_sampler(0); //the function where everything happens
	}
	else if (prog == 412) //granular sampler with chord progression
	{
		ADC_configure_MIC(BOARD_MIC_ADC_CFG_STRING); //use built-in microphones
		granular_sampler(31); //select song #31
	}
	else if (prog == 413) //granular sampler with chord progression
	{
		ADC_configure_MIC(BOARD_MIC_ADC_CFG_STRING); //use built-in microphones
		granular_sampler(1); //select arbitrary song in sub-menu
	}

	//-------------------------------------------------------------------------------------------
	// 42x: Reverb Chamber
	//-------------------------------------------------------------------------------------------

	else if (prog == 421) //decaying reverb, low frequencies
	{
		ADC_configure_MIC(BOARD_MIC_ADC_CFG_STRING); //use built-in microphones
		reverb_with_echo(0); //basic mode
	}
	else if (prog == 422) //decaying reverb, high frequencies
	{
		ADC_configure_MIC(BOARD_MIC_ADC_CFG_STRING); //use built-in microphones
		reverb_with_echo(1); //
	}
	else if (prog == 423) //reverb chamber controlled by sensors
	{
		ADC_configure_MIC(BOARD_MIC_ADC_CFG_STRING); //use built-in microphones
		reverb_with_echo(2); //
	}

	//-------------------------------------------------------------------------------------------
	// 441: ByteBeat
	//-------------------------------------------------------------------------------------------

	else if (prog == 441) //various bytebeats
	{
		bytebeat(0);
	}

	//else if (prog == 4xx...) //parsed later, as more pre-programmed songs

	//-------------------------------------------------------------------------------------------
	// 1111: stored content editor (song)
	//-------------------------------------------------------------------------------------------

	else if (prog == 1111) //edit currently programmed song by buttons
	{
		codec_reset();

		fil = new Filters(0); //no selected song
		fil->setup();

		//look if there is any song in Backup SRAM
		EEPROM_LoadSongAndMelody(&progression_str, &melody_str);

		if (progression_str!=NULL)
		{
			custom_song_edit_mode(progression_str);

			//translate to required notation
			/*int encoded_length =*/ encode_temp_song_to_notes(temp_song, temp_total_chords, &progression_str); //memory will be allocated for progression_str
			//also store to EEPROM
			EEPROM_StoreSongAndMelody(progression_str, (char*)""); //no melody

			fil->setup(); //reset all filters

			return custom_program_init(CHANNEL_111_RECENTLY_GENERATED_SONG); //roll over to program #111 and play
		}
		else //nothing to edit
		{
			//a modern equivalent
			play_buffer((uint16_t*)(0x08056B00), 0x1e560, 1); //1=mono
			NVIC_SystemReset();
		}
	}

	//-------------------------------------------------------------------------------------------
	// (1-2)xxx[...]: experimental channels, features, demo songs with different logic, WIP
	//-------------------------------------------------------------------------------------------

	else if (prog == 1122) //drum kit samples test
	{
		play_buffer((uint16_t*)0x080D2420, 137246, 1); //1=mono
		NVIC_SystemReset();
	}

	else if (prog == 1211) //IR sensor override by CV direct notes test
	{
		//configure inputs
		PROG_add_OpAmp_ADC12_signal = true; //microphone or line-in signal, or anything coming via ADC1 and ADC2
		PROG_audio_input_microphones = true;
		PROG_audio_input_pickups = false;
		PROG_enable_LED_indicators_IR_sensors = false; //don't display input from IR either

		//disable some features
		//PROG_enable_rhythm = false;
		PROG_enable_chord_loop = false;
		PROG_enable_LED_indicators_sequencer = false;

		//enable what is needed
		PROG_add_echo = true;
		PROG_enable_filters = true;
		PROG_add_plain_noise = true;
		PROG_noise_effects = true;

		//set faster rate so all filters get updated in time
		PROGRESS_UPDATE_FILTERS_RATE = 165; //every 6.06 ms, at 0.213 ms

		ACTIVE_FILTERS_PAIRS = 4; //only use first 4 filter pairs to save computation power

		TEST_enable_V1_control_voice = true;
	    TEST_enable_V2_control_drum = true;

	    PROG_drum_kit = true;
	}

	else if (prog == 1212) //MIDI out test
	{
		GPIO_Init_USART3(USART_MIDI_BAUD_RATE, 0); //configure USART3 for MIDI in & out
		MIDI_out_test();
	}

	else if (prog == 1213) //MIDI record and playback test
	{
		GPIO_Init_USART3(USART_MIDI_BAUD_RATE, 0); //configure USART3 for MIDI in & out
		MIDI_record_playback_test();
	}

	else if (prog == 1223) //MIDI signals direct test
	{
		GPIO_Init_USART3RX_direct_input(); //direct init of MIDI IN signal (USART3 RX) as input pin
		MIDI_direct_signals_test();
	}

	else if (prog == 1214) //cv_gate_test
	{
		custom_program_init(2); //recursive init channel #2

		//disable some controls
		PROG_enable_S1_control_noise_boost = false;
	    PROG_enable_S2_control_noise_attenuation = false;

	    //test-enable CV and Gate
		TEST_enable_V1_control_voice = true;
	    TEST_enable_V2_control_drum = true;

	    PROG_drum_kit = true;
		ACTIVE_FILTERS_PAIRS = FILTERS/2 - 2; //need to set aside some computation power
	}

    else if (prog == 1234) //test all LEDs
	{
		all_LEDs_test_seq1();
	}

	else if (prog == 11223) //acoustic location test
	{
		ADC_configure_MIC(BOARD_MIC_ADC_CFG_STRING); //use built-in microphones
		acoustic_location_test();
	}

	//else if (prog == 11224) //other tests
	//{

	//}

	//else if (prog == 12233) //IR remote experiment
	//{
		//ADC_configure_IR_sensors_as_audio();
		//AutoCorrelation_capture(0, NULL, AUTOCORRELATION_PREAMP_BOOST_MIC);
		//int ir_code = IR_remote_capture();
	//}

	else if (prog == 12234 || prog == 12243) //epic ad
	{
		selected_song = 2;
		selected_melody = 2;
		//DEFAULT_ARPEGGIATOR_FILTER_PAIR = 1; //not sure why but this kills the arp

	    PROG_enable_S1_control_noise_boost = true;
	    PROG_enable_S2_control_noise_attenuation = true;  //only when rhythm active
		PROG_enable_S3_control_resonance = true;
		PROG_enable_S4_control_arpeggiator = true; //arpeggiator is nice to have here too

		//PROG_add_echo = false;
		if (prog == 12243)
		{
			PROG_drum_kit = true;
			ACTIVE_FILTERS_PAIRS = FILTERS/2 - 2; //need to set aside some computation power
			PROG_wavetable_sample = false;
		}
	}

	else if (prog == 12334 || prog == 12344) //xmas
	{
		selected_song = 14;
		selected_melody = 14;
		//DEFAULT_ARPEGGIATOR_FILTER_PAIR = 1; //not sure why but this kills the arp

	    PROG_enable_S1_control_noise_boost = true;
	    PROG_enable_S2_control_noise_attenuation = true;  //only when rhythm active
		PROG_enable_S3_control_resonance = true;
		PROG_enable_S4_control_arpeggiator = true; //arpeggiator is nice to have here too

		//PROG_add_echo = false;

		//PROG_enable_LED_indicators = false;
		//PROG_LED_IR_sensors_display = false;

		SHIFT_CHORD_INTERVAL = 1;

		ECHO_MIXING_GAIN_MUL = 1;
	    ECHO_MIXING_GAIN_DIV = 3;

	    PROG_wavetable_sample = true;
		if (prog == 12334)
		{
			ACTIVE_FILTERS_PAIRS = FILTERS/2 - 2; //need to set aside some computation power (more)
		}
		else
		{
			ACTIVE_FILTERS_PAIRS = FILTERS/2 - 1; //need to set aside some computation power (less)
		}
	}

	else if (prog == 112233) //show FW version
	{
		show_firmware_version();
	}

	else if (prog == 223344) //show board serial number in BCD (red-orange-blue)
	{
		show_board_serial_no();
	}

	else if (prog == 121314) //easter egg #1
	{
		//there is more
		play_buffer((uint16_t*)0x08075060, 0x17900, 1); //1=mono
		NVIC_SystemReset();
	}

	else if (prog == 213141) //easter egg #2
	{
		//a modern equivalent
		play_buffer((uint16_t*)(0x08056B00), 0x1e560, 1); //1=mono
		NVIC_SystemReset();
	}

	else if (prog == 43214321) //reset all settings
	{
		EEPROM_ClearSettings();
		NVIC_SystemReset();
	}

	//-------------------------------------------------------------------------------------------

	else { //check for long number channels

		if (strlen(prog_str_ptr) > 3)
		{
			if (0==strncmp(prog_str_ptr, "222", 3)) //deterministic chaos channels
			{
				//progression_str_length = generate_chord_progression(&progression_str);
				//this shifts 19.1919191919191919191919191919191919191919 -> 19.329762269156163

				//if (first_program_run)
				//{
				char RNG_seed[20];
				strcpy(RNG_seed,"0.");
				strcat(RNG_seed,prog_str_ptr+3);
				float RNG_seed_f = atof(RNG_seed);

				set_pseudo_random_seed(RNG_seed_f);
					//set_pseudo_random_seed(PseudoRNG1a_next_float());

					//first_program_run = 0;
				//}

				/*
				if (prog == 1112)
				{
					reset_pseudo_random_seed();
				}
				*/

				progression_str_length = get_music_from_noise(&progression_str, &melody_str);

				EEPROM_StoreSongAndMelody(progression_str, melody_str);

				/*
				char test[100];
				strncpy(test,SONG_STORED_ID,100);
				//strncpy(test,MELODY_STORED_ID,100);
				strncpy(test,SONG_STORED_DATA,100);
				test[0] = test[0];
				*/

				//fil = new filters(CHANNEL_112_RECENTLY_GENERATED_SONG_W_MELODY); //select the song
				//fil->filter_setup02();

				//return custom_program_init(CHANNEL_112_RECENTLY_GENERATED_SONG_W_MELODY); //roll over to program #112 and play
				return custom_program_init(CHANNEL_111_RECENTLY_GENERATED_SONG); //roll over to program #111 and play
			}

			if (0==strncmp(prog_str_ptr, "314", 3)) //songs of the pi
			{
				int location = 0;
				int digit;
				char dig[10];

				//if(prog_str_ptr[3]=='1')
				//{
					//location = 1;
				//}

				for(unsigned int i=3; i<strlen(prog_str_ptr); i++)
				{
					location *= 4; //we are counting in base-4
					strncpy(dig, prog_str_ptr+i, 1);
					dig[1] = 0;
					digit = atoi(dig);// - 1;
					location += digit;
				}

				if(location==0) //because rolled over from #314 with no extra digits
				{
					location = random_value % (50000/8);
				}
				//location = 1000; //50 sec

				//LED_R8_all_OFF();
				//LED_O4_all_OFF();
				//int pi = calculate_pi_at_nth_digit(location, 1); //with indication
				uint32_t pi = load_pi_at_nth_dword(location-1);
				//LED_R8_all_OFF();
				//LED_O4_set_byte(0x0f); //all orange LEDs on

				set_pseudo_random_seed((double)pi/(double)31415926535); //init random seed by loaded or calculated value of pi starting from certain decimal place
				return custom_program_init(CHANNEL_222_GENERATE_SONG_FROM_NOISE); //roll over to program #222 and play
			}

			if (0==strncmp(prog_str_ptr, "333", 3)) //key code challenge
			{
				uint8_t code_sha1[20];
				char code_sha1_hex[40];

				//doesn't work on STM32F405
				//HASH_SHA1(((uint8_t*)prog_str)+3, strlen((char*)prog_str)-3, code_sha1);

				char *string_to_hash = (char*)malloc(200);

				uint32_t uid_w0 = ((uint32_t*)UID_BASE_ADDRESS)[0];
				uint32_t uid_w1 = ((uint32_t*)UID_BASE_ADDRESS)[1];
				uint32_t uid_w2 = ((uint32_t*)UID_BASE_ADDRESS)[2];

				//char *string_to_hash_ptr = string_to_hash + 100;
				strcpy(string_to_hash + 100, CODE_CHALLENGE_KEY);
				l33tsp34k(string_to_hash + 100, 100);
				/*
				while (string_to_hash_ptr < string_to_hash + 200)
				{
					if (string_to_hash_ptr[0]=='o') {string_to_hash_ptr[0]='0';}
					if (string_to_hash_ptr[0]=='i') {string_to_hash_ptr[0]='1';}
					if (string_to_hash_ptr[0]=='z') {string_to_hash_ptr[0]='2';}
					if (string_to_hash_ptr[0]=='e') {string_to_hash_ptr[0]='3';}
					if (string_to_hash_ptr[0]=='a') {string_to_hash_ptr[0]='4';}
					if (string_to_hash_ptr[0]=='s') {string_to_hash_ptr[0]='5';}
					string_to_hash_ptr++;
				}
				*/

				sprintf(string_to_hash, "%x-%x-%x:%s:%s", uid_w0, uid_w1, uid_w2, prog_str_ptr, string_to_hash + 100);

				SHA1_CTX sha;
				SHA1Init(&sha);
				SHA1Update(&sha, (uint8_t *)string_to_hash, strlen(string_to_hash));
				SHA1Final(code_sha1, &sha);

				free(string_to_hash);
				sha1_to_hex(code_sha1_hex, code_sha1);
				display_code_challenge((uint32_t*)code_sha1);
				while (1);
			}

			if (0==strncmp(prog_str_ptr, "4441444244434444321", 19)) //activation lock
			{
				FLASH_set_activation_lock();
				//there is more
				play_buffer((uint16_t*)0x08075060, 0x17900, 1); //1=mono
				NVIC_SystemReset();
			}

			if (0==strncmp(prog_str_ptr, "4", 1)) //more pre-programmed songs
			{
				//enable all functions controllable by IR sensors
				PROG_enable_S1_control_noise_boost = true;
				PROG_enable_S2_control_noise_attenuation = true;
				PROG_enable_S3_control_resonance = true;
				PROG_enable_S4_control_arpeggiator = true;

				if (prog >= 4111 && prog <= 4144)
				{
					if(selected_song == 14)
					{
						return custom_program_init(12334); //roll over to program #12334 to get correct set up for this special song
					}
				}
				else
				{
					NVIC_SystemReset(); //nothing here
				}
			}
			else
			{
				NVIC_SystemReset(); //nothing here
			}

		}
		else
		{
			NVIC_SystemReset(); //nothing here
		}
	}
}

void custom_effect_init(uint64_t prog)
{
	if (PROG_noise_effects)
	{
		//prepare mixing volumes, set resonance to zero
		for (int i=0;i<FILTERS;i++)
		{
			fil->fp.mixing_volumes[i] = 0;
			fil->iir2[i].setResonance(0.0f); //no resonance
		}

		int mixing_volumes_default = 4.0f;

		if (prog==CHANNEL_21_INTERACTIVE_NOISE_EFFECT)
		{
			base_freq = 400; //base frequency
			param_f = 0.0f; //no base resonance
		}
		else if (prog==CHANNEL_22_INTERACTIVE_NOISE_EFFECT)
		{
			base_freq = 150; //base frequency
			param_f = 0.9f; //some base resonance
		}
		else if (prog==CHANNEL_23_INTERACTIVE_NOISE_EFFECT)
		{
			base_freq = 0; //base frequency
			param_f = 0.995f; //maximum base resonance
		}
		else if (prog==CHANNEL_31_THEREMIN_BY_MAGNETIC_RING || prog == CHANNEL_32_THEREMIN_BY_IR_SENSORS || prog==1211)
		{
			base_freq = 0; //base frequency
			param_f = 0.998f; //maximum base resonance
			mixing_volumes_default = 1.0f;
		}

		for (int i=0;i<WAVES_FILTERS;i++) //enable mixing of first four pairs
		{
			waves_freqs[i] = base_freq;

			fil->fp.mixing_volumes[FILTERS/2 + i] = mixing_volumes_default;
			fil->fp.mixing_volumes[i] = mixing_volumes_default;

			fil->iir2[i].setResonanceKeepFeedback(param_f);//just update variable, will calculate feedback when setting freq
			fil->iir2[FILTERS/2 + i].setResonanceKeepFeedback(param_f);
		}

		if (prog==CHANNEL_24_INTERACTIVE_NOISE_EFFECT)
		{
			fil->iir2[0].setResonanceKeepFeedback(0.0f);
			fil->iir2[FILTERS/2 + 0].setResonanceKeepFeedback(0.0f);
			fil->iir2[1].setResonanceKeepFeedback(0.85f);
			fil->iir2[FILTERS/2 + 1].setResonanceKeepFeedback(0.85f);
			fil->iir2[2].setResonanceKeepFeedback(0.95f);
			fil->iir2[FILTERS/2 + 2].setResonanceKeepFeedback(0.95f);
			fil->iir2[3].setResonanceKeepFeedback(0.995f);
			fil->iir2[FILTERS/2 + 3].setResonanceKeepFeedback(0.995f);
		}

		if (prog==CHANNEL_31_THEREMIN_BY_MAGNETIC_RING || prog == CHANNEL_32_THEREMIN_BY_IR_SENSORS || prog==1211) //need to update filters as not all of them will be updated straight away
		{
			for (int i=0;i<WAVES_FILTERS;i++) //set initial freqs
			{
				fil->iir2[i].setCutoff(base_freq);
				fil->iir2[FILTERS/2 + i].setCutoff(base_freq);
			}
		}
	}

    //if(PROG_add_echo) //need to always init in case the channel is overridden with echo-off setting
    {
		init_echo_buffer();
    }
}

int note_static = 0;
int last_note = -1, new_note;
int current_pair = 0;
#define NOTE_STATIC_ACCEPT_LIMIT 30 //20 * 50ms = 1s ? looked like 2sec -> 10
#define NOTES_TO_COLLECT 3
int finished;
int notes_collected = 0;
int collected_notes[NOTES_TO_COLLECT];
int collected_notes_blink = 0;
#define COLLECTED_NOTES_BLINK_RATE 10

void custom_effect_filter_process()
{
	//alien spaceship / waves of the sea / nostromo / all combined
	if (channel >= CHANNEL_21_INTERACTIVE_NOISE_EFFECT && channel <= CHANNEL_24_INTERACTIVE_NOISE_EFFECT)
	{
        if (TIMING_BY_SAMPLE_EVERY_100_MS==0) //10Hz
    	{
    		for (int i=0;i<WAVES_FILTERS;i++)
    		{
    			param_i = ADC_last_result[i] + base_freq;
    			if (param_i < 10) {
    				param_i = 10;
    			}

    			direct_update_filters_id[i] = i;
    			direct_update_filters_freq[i] = param_i;
    		}

    		fil->start_update_filters_pairs(direct_update_filters_id, direct_update_filters_freq, WAVES_FILTERS);
    	}
	}

	//CHANNEL_31_THEREMIN_BY_MAGNETIC_RING
	//CHANNEL_32_THEREMIN_BY_IR_SENSORS

	/*
	//this is older version, with collecting notes - disabled now
	if (channel==0) //magnetic ring controlled
	{
    	//if (sampleCounter%(2*I2S_AUDIOFREQ/2)==0) //every 500ms
        //if (sampleCounter%(2*I2S_AUDIOFREQ/5)==0) //every 200ms
        //if (sampleCounter%(2*I2S_AUDIOFREQ/10)==0) //every 100ms
    	if (sampleCounter%(2*I2S_AUDIOFREQ/20)==0) //every 50ms
    	{
    		//for (int i=0;i<WAVES_FILTERS;i++)
    		//{
    			//param_i = ADC_last_result[i] + base_freq;
    		//int note;
    		float note_freq;

			//param_i = (magnetic_sensor_latest_value - BOARD_MAG_SENSOR_VALUE_OFFSET - magnetic_sensor_calibration);
			param_i = (BOARD_MAG_SENSOR_VALUE_OFFSET + magnetic_sensor_calibration - magnetic_sensor_latest_value);

			#define MAG_SCALING_COEF 50

			KEY_LED_all_off();
			for (int i=0;i<notes_collected;i++)
			{
				KEY_LED_on(collected_notes[i]+1);
			}

			//blink collected notes
			if (notes_collected > 0 && ++collected_notes_blink%COLLECTED_NOTES_BLINK_RATE==0)
			{
				KEY_LED_all_off();
			}

			if (param_i < -MAG_SCALING_COEF)
			{
				if (param_i < -5*MAG_SCALING_COEF)
				{
					new_note = 10;
					note_freq = notes_freqs[10];
					KEY_LED_on(11);
				}
				else if (param_i < -4*MAG_SCALING_COEF)
				{
					new_note = 8;
					note_freq = notes_freqs[8];
					KEY_LED_on(9);
				}
				else if (param_i < -3*MAG_SCALING_COEF)
				{
					new_note = 6;
					note_freq = notes_freqs[6];
					KEY_LED_on(7);
				}
				else if (param_i < -2*MAG_SCALING_COEF)
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
			}
			else if (param_i > MAG_SCALING_COEF)
			{
				//note = (param_i / 40);
				//if (note < 2)
				//{
				//	note = 2;
				//}
				//if (note > 24)
				//{
				//	note = 24;
				//}
				//
				////add halftones... 12 halftones is plus one octave -> frequency * 2
				//float note_freq = NOTE_FREQ_A4 * pow(HALFTONE_STEP_COEF, note+1);

				//this will be faster
				if (param_i > 8*MAG_SCALING_COEF)
				{
					new_note = 12;
					note_freq = notes_freqs[12];
					KEY_LED_on(13);
				}
				else if (param_i > 7*MAG_SCALING_COEF)
				{
					new_note = 11;
					note_freq = notes_freqs[11];
					KEY_LED_on(12);
				}
				else if (param_i > 6*MAG_SCALING_COEF)
				{
					new_note = 9;
					note_freq = notes_freqs[9];
					KEY_LED_on(10);
				}
				else if (param_i > 5*MAG_SCALING_COEF)
				{
					new_note = 7;
					note_freq = notes_freqs[7];
					KEY_LED_on(8);
				}
				else if (param_i > 4*MAG_SCALING_COEF)
				{
					new_note = 5;
					note_freq = notes_freqs[5];
					KEY_LED_on(6);
				}
				else if (param_i > 3*MAG_SCALING_COEF)
				{
					new_note = 4;
					note_freq = notes_freqs[4];
					KEY_LED_on(5);
				}
				else if (param_i > 2*MAG_SCALING_COEF)
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
			}
			else
			{
				direct_update_filters_id[0] = current_pair;
				direct_update_filters_freq[0] = 0;
				new_note = -1;
				fil->start_update_filters_pairs(direct_update_filters_id, direct_update_filters_freq, 1); //update 1 pair
			}

			if (new_note == last_note && new_note >= 0)
			{
				note_static++;
				if (note_static == NOTE_STATIC_ACCEPT_LIMIT)
				{
					KEY_LED_all_off(); //blink to indicate note was held long enough
					current_pair++;

					collected_notes[notes_collected] = new_note;
					notes_collected++;

					if (current_pair==NOTES_TO_COLLECT)
					{
						finished = 1;
					}
				}
				if (note_static == 2)
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

			//if (ADC_last_result[0] > 100)
			//{
				//fil->start_update_filters_pairs(direct_update_filters_id, direct_update_filters_freq, 1); //update 1 pair
			//}
    	}
	}
	*/

	if (channel==CHANNEL_31_THEREMIN_BY_MAGNETIC_RING) //magnetic ring controlled
	{
    	//if (sampleCounter%(2*I2S_AUDIOFREQ/2)==0) //every 500ms
        //if (sampleCounter%(2*I2S_AUDIOFREQ/5)==0) //every 200ms
        //if (sampleCounter%(2*I2S_AUDIOFREQ/10)==0) //every 100ms
    	if (sampleCounter%(2*I2S_AUDIOFREQ/20)==0) //every 50ms
    	{
    		float note_freq;

			param_i = (BOARD_MAG_SENSOR_VALUE_OFFSET + magnetic_sensor_calibration - magnetic_sensor_latest_value);

			#define MAG_SCALING_COEF 50

			KEY_LED_all_off();

			if (param_i < -MAG_SCALING_COEF)
			{
				if (param_i < -5*MAG_SCALING_COEF)
				{
					new_note = 10;
					note_freq = notes_freqs[10];
					KEY_LED_on(11);
				}
				else if (param_i < -4*MAG_SCALING_COEF)
				{
					new_note = 8;
					note_freq = notes_freqs[8];
					KEY_LED_on(9);
				}
				else if (param_i < -3*MAG_SCALING_COEF)
				{
					new_note = 6;
					note_freq = notes_freqs[6];
					KEY_LED_on(7);
				}
				else if (param_i < -2*MAG_SCALING_COEF)
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
			}
			else if (param_i > MAG_SCALING_COEF)
			{
				if (param_i > 8*MAG_SCALING_COEF)
				{
					new_note = 12;
					note_freq = notes_freqs[12];
					KEY_LED_on(13);
				}
				else if (param_i > 7*MAG_SCALING_COEF)
				{
					new_note = 11;
					note_freq = notes_freqs[11];
					KEY_LED_on(12);
				}
				else if (param_i > 6*MAG_SCALING_COEF)
				{
					new_note = 9;
					note_freq = notes_freqs[9];
					KEY_LED_on(10);
				}
				else if (param_i > 5*MAG_SCALING_COEF)
				{
					new_note = 7;
					note_freq = notes_freqs[7];
					KEY_LED_on(8);
				}
				else if (param_i > 4*MAG_SCALING_COEF)
				{
					new_note = 5;
					note_freq = notes_freqs[5];
					KEY_LED_on(6);
				}
				else if (param_i > 3*MAG_SCALING_COEF)
				{
					new_note = 4;
					note_freq = notes_freqs[4];
					KEY_LED_on(5);
				}
				else if (param_i > 2*MAG_SCALING_COEF)
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
			}
			else
			{
				direct_update_filters_id[0] = current_pair;
				direct_update_filters_freq[0] = 0;
				new_note = -1;
				fil->start_update_filters_pairs(direct_update_filters_id, direct_update_filters_freq, 1); //update 1 pair
			}

			if (new_note == last_note && new_note >= 0)
			{
				note_static++;

				if (note_static == 2)
				{
					fil->start_update_filters_pairs(direct_update_filters_id, direct_update_filters_freq, 1); //update 1 pair
				}
			}
			else
			{
				last_note = new_note;
				note_static = 0;
			}
    	}
	}

	if (channel==CHANNEL_32_THEREMIN_BY_IR_SENSORS) //distance sensors controlled
	{
    	//if (sampleCounter%(2*I2S_AUDIOFREQ/2)==0) //every 500ms
        //if (sampleCounter%(2*I2S_AUDIOFREQ/5)==0) //every 200ms
        //if (sampleCounter%(2*I2S_AUDIOFREQ/10)==0) //every 100ms
    	if (sampleCounter%(2*I2S_AUDIOFREQ/20)==0) //every 50ms
    	{
    		float note_freq;

			KEY_LED_all_off();

			if (ADC_last_result[0] > 100 || ADC_last_result[1] > 100)
			{
				if (ADC_last_result[1] > 800)
				{
					new_note = 10;
					note_freq = notes_freqs[10];
					KEY_LED_on(11);
				}
				else if (ADC_last_result[1] > 400)
				{
					new_note = 8;
					note_freq = notes_freqs[8];
					KEY_LED_on(9);
				}
				else if (ADC_last_result[1] > 200)
				{
					new_note = 6;
					note_freq = notes_freqs[6];
					KEY_LED_on(7);
				}
				else if (ADC_last_result[0] > 800)
				{
					new_note = 3;
					note_freq = notes_freqs[3];
					KEY_LED_on(4);
				}
				else if (ADC_last_result[0] > 400)
				{
					new_note = 1;
					note_freq = notes_freqs[1];
					KEY_LED_on(2);
				}

				direct_update_filters_id[0] = current_pair;
				direct_update_filters_freq[0] = note_freq;
			}
			else if (ADC_last_result[2] > 100 || ADC_last_result[3] > 100)
			{
				if (ADC_last_result[2] > 1600)
				{
					new_note = 0;
					note_freq = notes_freqs[0];
					KEY_LED_on(1);
				}
				else if (ADC_last_result[2] > 800)
				{
					new_note = 2;
					note_freq = notes_freqs[2];
					KEY_LED_on(3);
				}
				else if (ADC_last_result[2] > 400)
				{
					new_note = 4;
					note_freq = notes_freqs[4];
					KEY_LED_on(5);
				}
				else if (ADC_last_result[2] > 200)
				{
					new_note = 5;
					note_freq = notes_freqs[5];
					KEY_LED_on(6);
				}
				else if (ADC_last_result[3] > 1600)
				{
					new_note = 7;
					note_freq = notes_freqs[7];
					KEY_LED_on(8);
				}
				else if (ADC_last_result[3] > 800)
				{
					new_note = 9;
					note_freq = notes_freqs[9];
					KEY_LED_on(10);
				}
				else if (ADC_last_result[3] > 400)
				{
					new_note = 11;
					note_freq = notes_freqs[11];
					KEY_LED_on(12);
				}
				else if (ADC_last_result[3] > 200)
				{
					new_note = 12;
					note_freq = notes_freqs[12];
					KEY_LED_on(13);
				}

				direct_update_filters_id[0] = current_pair;
				direct_update_filters_freq[0] = note_freq;
			}
			else
			{
				direct_update_filters_id[0] = current_pair;
				direct_update_filters_freq[0] = 0;
				//new_note = -1;
				fil->start_update_filters_pairs(direct_update_filters_id, direct_update_filters_freq, 1); //update 1 pair
			}

			if (new_note == last_note && new_note >= 0)
			{
				note_static++;

				if (note_static == 2)
				{
					fil->start_update_filters_pairs(direct_update_filters_id, direct_update_filters_freq, 1); //update 1 pair
				}
			}
			else
			{
				last_note = new_note;
				note_static = 0;
			}
    	}
	}

	if (channel>=1211) //IR sensor override by CV direct notes test
	{
    	if (TIMING_BY_SAMPLE_EVERY_20_MS==0) //50Hz
    	{
			//float note_freq = NOTE_FREQ_A4 * pow(HALFTONE_STEP_COEF, ADC_measured_vals[0] / 100);
			float note_freq = ADC_measured_vals[0] / 2 + 10;

			direct_update_filters_id[0] = 0;
			direct_update_filters_freq[0] = note_freq;
			fil->start_update_filters_pairs(direct_update_filters_id, direct_update_filters_freq, 1); //update 1 pair
    	}
	}

	/*
	if (PROG_effect_sea_waves) //waves passive program
	{
    	if (sampleCounter%(2*I2S_AUDIOFREQ/5)==0) //every 200ms
        //if (sampleCounter%(2*I2S_AUDIOFREQ/10)==0) //every 100ms
    	//if (sampleCounter%(2*I2S_AUDIOFREQ/20)==0) //every 50ms
    	{
    		fil->fp.reso2 = 0;
    		fil->fp.reso2lim[0] = fil->fp.reso2 / 2;
    		fil->fp.reso2lim[1] = fil->fp.reso2 * 1.1;

    		chaotic_coef += 0.001;
    	}

    	if (sampleCounter%(2*I2S_AUDIOFREQ/5)==2) //every 200ms
        //if (sampleCounter%(2*I2S_AUDIOFREQ/10)==0) //every 100ms
    	//if (sampleCounter%(2*I2S_AUDIOFREQ/20)==0) //every 50ms
    	{
    		for (int i=0;i<WAVES_FILTERS;i++)
    		{
    			direct_update_filters_id[i] = i;
    			direct_update_filters_freq[i] = 45.6789 * i * chaotic_coef;
    		}

    		fil->start_update_filters_pairs(direct_update_filters_id, direct_update_filters_freq, WAVES_FILTERS);
    	}
	}
	*/

	/*
	if (PROG_effect_solaris)
	{
    	if (sampleCounter%(2*I2S_AUDIOFREQ/5)==0) //every 200ms
        //if (sampleCounter%(2*I2S_AUDIOFREQ/10)==0) //every 100ms
    	//if (sampleCounter%(2*I2S_AUDIOFREQ/20)==0) //every 50ms
    	{
    		fil->fp.reso2 = 0;
    		fil->fp.reso2lim[0] = fil->fp.reso2 / 2;
    		fil->fp.reso2lim[1] = fil->fp.reso2 * 1.1;

    		chaotic_coef += 0.001;
    	}

    	if (sampleCounter%(2*I2S_AUDIOFREQ/5)==2) //every 200ms
        //if (sampleCounter%(2*I2S_AUDIOFREQ/10)==0) //every 100ms
    	//if (sampleCounter%(2*I2S_AUDIOFREQ/20)==0) //every 50ms
    	{
    		for (int i=0;i<WAVES_FILTERS;i++)
    		{
    			direct_update_filters_id[i] = i;
    			direct_update_filters_freq[i] = 45.6789 * i * chaotic_coef;
    		}

    		fil->start_update_filters_pairs(direct_update_filters_id, direct_update_filters_freq, WAVES_FILTERS);
    	}

    	if (sampleCounter%(2*I2S_AUDIOFREQ)==10) //every 1s
    	{
    		if (BUTTON_U1_ON)
    		{
    			fil->fp.mixing_volumes[FILTERS/2-1] = 1 - fil->fp.mixing_volumes[FILTERS/2-1];
    			fil->fp.mixing_volumes[FILTERS-1] = 1 - fil->fp.mixing_volumes[FILTERS-1];
    		}
    		if (BUTTON_U2_ON)
    		{
    			fil->fp.mixing_volumes[FILTERS/2-2] = 1 - fil->fp.mixing_volumes[FILTERS/2-2];
    			fil->fp.mixing_volumes[FILTERS-2] = 1 - fil->fp.mixing_volumes[FILTERS-2];
    		}
    		if (BUTTON_U3_ON)
    		{
    			fil->fp.mixing_volumes[FILTERS/2-3] = 1 - fil->fp.mixing_volumes[FILTERS/2-3];
    			fil->fp.mixing_volumes[FILTERS-3] = 1 - fil->fp.mixing_volumes[FILTERS-3];
    		}
    		if (BUTTON_U4_ON)
    		{
    			fil->fp.mixing_volumes[FILTERS/2-4] = 1 - fil->fp.mixing_volumes[FILTERS/2-4];
    			fil->fp.mixing_volumes[FILTERS-4] = 1 - fil->fp.mixing_volumes[FILTERS-4];
    		}
    	}
	}
	*/

	/*
	if (PROG_effect_VIP)
	{
    	//if (sampleCounter%(2*I2S_AUDIOFREQ/5)==0) //every 200ms
        //if (sampleCounter%(2*I2S_AUDIOFREQ/10)==0) //every 100ms
	    //if (sampleCounter%(2*I2S_AUDIOFREQ/20)==0) //every 50ms
	    if (sampleCounter%(2*I2S_AUDIOFREQ/50)==700) //every 20ms -> 882 samples cycle
    	{
    		//for (int i=0;i<WAVES_FILTERS;i++)
	    	int i = random_value % WAVES_FILTERS;
    		{
    			/ *
    			param_i = ADC_last_result[i];
    			if (param_i < 10)
    			{
    				param_i = 10;
    			}
    			* /

    			waves_freqs[i] += 10 - ((random_value >> (8 * i)) % 20);

    			if (waves_freqs[i] < 150)
    			{
    				waves_freqs[i] = 150;
    			}

    			if (waves_freqs[i] > 1500)
    			{
    				waves_freqs[i] = 1500;
    			}

    			//direct_update_filters_id[2*i] = i;
    			//direct_update_filters_freq[2*i] = param_i; //error!
    			//direct_update_filters_id[2*i+1] = i + FILTERS/2;
    			//direct_update_filters_freq[2*i+1] = param_i;

    			direct_update_filters_id[0] = i;
    			direct_update_filters_freq[0] = waves_freqs[i] + (100 - (random_value>>10) % 200);;
    			direct_update_filters_id[1] = i + FILTERS/2;
    			direct_update_filters_freq[1] = waves_freqs[i] + (100 - random_value % 200);
    		}
			//fil->start_update_filters_pairs(direct_update_filters_id, direct_update_filters_freq, WAVES_FILTERS*2);
			fil->start_update_filters_pairs(direct_update_filters_id, direct_update_filters_freq, 2);
    	}

    	/ *
    	if (sampleCounter%(2*I2S_AUDIOFREQ/5)==2) //every 200ms
        //if (sampleCounter%(2*I2S_AUDIOFREQ/10)==0) //every 100ms
    	//if (sampleCounter%(2*I2S_AUDIOFREQ/20)==0) //every 50ms
    	{
    		for (int i=0;i<WAVES_FILTERS;i++)
    		{
    			direct_update_filters_id[i] = i;
    			direct_update_filters_freq[i] = 45.6789 * i * chaotic_coef;
    		}

    		fil->start_update_filters_pairs(direct_update_filters_id, direct_update_filters_freq, WAVES_FILTERS);
    	}
        * /

		/ *
    	//if (sampleCounter%(2*I2S_AUDIOFREQ)==10) //every 1s
		if (sampleCounter%(2*I2S_AUDIOFREQ/10)==0) //every 100ms
    	{
    		if (BUTTON_U1_ON)
    		{
    			param_i += 10;
    		}
    		if (BUTTON_U2_ON)
    		{
    			param_i -= 10;
    		}
    		if (BUTTON_U3_ON)
    		{
    			if (param_f >= 0.99)
    			{
    				param_f += 0.001;
    			}
    			else if (param_f >= 0.9)
    			{
    				param_f += 0.01;
    			}
    			else
    			{
    				param_f += 0.1;
    			}
    			if (param_f > 1)
    			{
    				param_f = 1;
    			}
    		}
    		if (BUTTON_U4_ON)
    		{
    			if (param_f > 0.99)
    			{
    				param_f -= 0.001;
    			}
    			else if (param_f > 0.9)
    			{
    				param_f -= 0.01;
    			}
    			else
    			{
    				param_f -= 0.1;
    			}
    			if (param_f < 0)
    			{
    				param_f = 0;
    			}
    		}
    	}
		* /
	}
	*/
}

void alt_mode_hi_pass_with_water()
{
	//PROG_enable_rhythm = false;
	PROG_mix_sample_from_flash = true;
	FILTERS_TYPE_AND_ORDER = FILTERS_TYPE_HIGH_PASS + FILTERS_ORDER_4;

	//water
	mixed_sample_buffer = (int16_t*)0x080A2300;
	MIXED_SAMPLE_BUFFER_LENGTH = (196896/2);

	mixed_sample_buffer_ptr_L = 0;
	mixed_sample_buffer_ptr_R = MIXED_SAMPLE_BUFFER_LENGTH / 2; //set second channel to mid of the buffer for better stereo effect

	//noise_volume_max = 1.0f;
	noise_volume = 1.0f;
}
