
//#define BOARD_STM32F4_DISC1
//#define BOARD_GECHO_V001
#define BOARD_GECHO_V002

#ifdef BOARD_STM32F4_DISC1
	#define ADC_SENSORS 3 //STM32F4-DISC1
	#define SENSORS_STM32F4_DISC1
	#define PREAMP_BOOST 12 //stm32f4-disc1
#endif

#ifdef BOARD_GECHO_V001
	#define ADC_SENSORS 4 //GECHO_V001

	#define SENSORS_GECHO_V001
	#define PREAMP_BOOST 12 //8 //gecho board

	#define BOARD_MIC_ADC_CFG_STRING ADC_PA6_PA7

	#define BOARD_ADC_PIN_LINEIN1 GPIO_Pin_2 //PA2
	#define BOARD_ADC_PIN_LINEIN2 GPIO_Pin_3 //PA3, also the SOL sensor is attached here
	#define BOARD_ADC_PORT_LINEIN GPIOA
	#define BOARD_ADC_CHANNEL_LINEIN1 ADC_Channel_2
	#define BOARD_ADC_CHANNEL_LINEIN2 ADC_Channel_3

	//#define BOARD_ADC_PIN_IR_AUDIO1 GPIO_Pin_0
	#define BOARD_ADC_PIN_IR_AUDIO1 GPIO_Pin_1
	//#define BOARD_ADC_PIN_IR_AUDIO2 GPIO_Pin_2
	#define BOARD_ADC_PIN_IR_AUDIO2 GPIO_Pin_3
	#define BOARD_ADC_PORT_IR_AUDIO GPIOC
	//#define BOARD_ADC_CHANNEL_IR_AUDIO1 ADC_Channel_10
	#define BOARD_ADC_CHANNEL_IR_AUDIO1 ADC_Channel_11
	//#define BOARD_ADC_CHANNEL_IR_AUDIO2 ADC_Channel_12
	#define BOARD_ADC_CHANNEL_IR_AUDIO2 ADC_Channel_13

	#define BOARD_ADC_PIN_MAG_SENSOR GPIO_Pin_5 //PA5
	#define BOARD_ADC_PORT_MAG_SENSOR GPIOA
	#define BOARD_ADC_CHANNEL_MAG_SENSOR ADC_Channel_5
	#define BOARD_MAG_SENSOR_VALUE_OFFSET 2048
	#define BOARD_MAG_SENSOR_VALUE_COEF 30

	#define MAG_SENSOR_SIGNAL_BOOST 1000

	//#define CODEC_I2C_ADDRESS_NORMAL_AD0_LOW
	#define CODEC_I2C_ADDRESS_ALTERNATE_AD0_HIGH //board #4
	//#define BYPASS_W8_0 //board #3 has some problem with PA8
	//#define CAN_BLOCK_SWD_DEBUG

	#define IR_PAIRS_CONTROLLED_BY_TWO_LINES //newer revisions - board #3+ but doesn't seem to matter for board #1

	//if developing, or board has no buttons
	//#define DIRECT_PROGRAM_START 42 //arctic wind
	//#define DIRECT_PROGRAM_START 123 //all LEDs test
	//#define DIRECT_PROGRAM_START 1 //main seq
	#define DIRECT_PROGRAM_START 43 //waves

	//#define IGNORE_PA0_IF_PULLDOWN_NOT_ASSEMBLED //prevents program shutdown and/or restart if board not completely assembled

#endif


#ifdef BOARD_GECHO_V002
	#define ADC_SENSORS 4
	#define SENSORS_GECHO_V002

	#define BOARD_MIC_ADC_CFG_STRING ADC_PA6_PA7

	#define BOARD_ADC_PIN_LINEIN1 GPIO_Pin_2 //PA2
	#define BOARD_ADC_PIN_LINEIN2 GPIO_Pin_3 //PA3
	#define BOARD_ADC_PORT_LINEIN GPIOA
	#define BOARD_ADC_CHANNEL_LINEIN1 ADC_Channel_2
	#define BOARD_ADC_CHANNEL_LINEIN2 ADC_Channel_3

	#define BOARD_ADC_PIN_IR_AUDIO1 GPIO_Pin_1
	#define BOARD_ADC_PIN_IR_AUDIO2 GPIO_Pin_3
	#define BOARD_ADC_PORT_IR_AUDIO GPIOC
	#define BOARD_ADC_CHANNEL_IR_AUDIO1 ADC_Channel_11
	#define BOARD_ADC_CHANNEL_IR_AUDIO2 ADC_Channel_13

	#define BOARD_ADC_PIN_MAG_SENSOR GPIO_Pin_1 //PA1
	#define BOARD_ADC_PORT_MAG_SENSOR GPIOA
	#define BOARD_ADC_CHANNEL_MAG_SENSOR ADC_Channel_1
	#define BOARD_MAG_SENSOR_VALUE_OFFSET 2048 //2240
	#define BOARD_MAG_SENSOR_VALUE_COEF 30

	#define MAG_SENSOR_SIGNAL_BOOST 50

	#define CODEC_I2C_ADDRESS_NORMAL_AD0_LOW
	#define IR_PAIRS_CONTROLLED_BY_TWO_LINES //newer revisions - board #3+ but doesn't matter for board #1
	#define LEDS_WHITE_BLUE_INVERTED

	//#define IGNORE_PA0_IF_PULLDOWN_NOT_ASSEMBLED //prevents program shutdown and/or restart if board not completely assembled

	//start a channel immediately - useful while developing, or if board has no buttons assembled yet
	//#define DIRECT_PROGRAM_START 1 //main demo song
	//#define DIRECT_PROGRAM_START 3 //demo song with melody
	//#define DIRECT_PROGRAM_START 12 //arctic wind
	//#define DIRECT_PROGRAM_START 24 //solaris (VIP), test manual control filters
	//#define DIRECT_PROGRAM_START 124 //magnetic sensor test with LEDs
	//#define DIRECT_PROGRAM_START 323 //all LEDs test
	//#define DIRECT_PROGRAM_START 141 //Goertzel detectors test via mic
	//#define DIRECT_PROGRAM_START 142 //Goertzel detectors test via line-in
	//#define DIRECT_PROGRAM_START 143 //stylus test
	//#define DIRECT_PROGRAM_START 22234 //line in with echo, with special effect #222 (line-in overdrive)
	//#define DIRECT_PROGRAM_START 114 //experimental VIP
	//#define DIRECT_PROGRAM_START 13 //custom song programming mode (via mic)
	//#define DIRECT_PROGRAM_START 14 //custom song programming mode (via line-in)
	//#define DIRECT_PROGRAM_START 113 //alien spaceship
	//#define DIRECT_PROGRAM_START 144 //talkie lib port test
	//#define DIRECT_PROGRAM_START 121 //mag sensor direct notes test
	//#define DIRECT_PROGRAM_START 1211 //IR sensor override by CV direct notes test
	//#define DIRECT_PROGRAM_START 1212 //MIDI out demo
	//#define DIRECT_PROGRAM_START 12 //Custom song programming mode (manual input using magnetic ring)
	//#define DIRECT_PROGRAM_START 11223 //drum kit test for tutorial
	//#define DIRECT_PROGRAM_START 12233 //IR remote decoding test
	//#define DIRECT_PROGRAM_START 12234 //song #8 - epic ad
	//#define DIRECT_PROGRAM_START 12243 //song #8 - epic ad + drum kit
	//#define DIRECT_PROGRAM_START 23 //square wave mixing test
	//#define DIRECT_PROGRAM_START 12334 //song #10 - jingle bells
	//#define DIRECT_PROGRAM_START 1111 //automatic generator
	//#define DIRECT_PROGRAM_START 1214 //cv-gate test
	//#define DIRECT_PROGRAM_START 441 //Song #1 with pickups
	//#define DIRECT_PROGRAM_START 444 //Pachelbel with pickups
	//#define DIRECT_PROGRAM_START 4111 //Rachel's song
	//#define DIRECT_PROGRAM_START 4112 //app demo song
	//#define DIRECT_PROGRAM_START 333213243 //code challenge
	//#define DIRECT_PROGRAM_START 333213221112 //code challenge
	//#define DIRECT_PROGRAM_START 333213221112123432 //code challenge - longest possible (can't have 19 digits with 3 at the start)
	//#define DIRECT_PROGRAM_START 1223 //midi direct test
	//#define DIRECT_PROGRAM_START 3141232343//41412 //song of the pi
	//#define DIRECT_PROGRAM_START 33 //DCO synth
	//#define DIRECT_PROGRAM_START 234 //sequencer
	//#define DIRECT_PROGRAM_START 11223 //other tests
	//#define DIRECT_PROGRAM_START 411 //granular sampler
	//#define DIRECT_PROGRAM_START 234 //drum sequencer
	//#define DIRECT_PROGRAM_START 414 //reverb

	#define CAN_USE_CH340G_LEDS //can LEDs driven by USART1 be initialized normally?
	#define USART1_CONTROL_MODE //shall Gecho listen to USART1 commands when idle?
	#define CODEC_OUTPUT_LIMITER //enable hw limiter (by DAC DSP circuitry)

	//#define CAN_BLOCK_SWD_DEBUG //useful to disallow while developing, so you don't have to press reset at every upload

#endif

//#define CODEC_COMM_BLINK_TEST //test for DAC communication when buttons and rest of circuitry not assembled yet
//#define CODEC_TLV
//#define GECHO_V2
