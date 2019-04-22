/*
 * signals.c
 *
 *  Created on: Apr 27, 2016
 *      Author: mayo
 *
 * Based on "Simple ADC use on the STM32" by Peter Harrison
 * http://www.micromouseonline.com/2009/05/26/simple-adc-use-on-the-stm32/
 *
 *  This file is part of the Gecho Loopsynth Firmware Development Framework.
 *  It can be used within the terms of CC-BY-NC-SA license.
 *  It must not be distributed separately.
 *
 *  Find more information at: http://gechologic.com/gechologists/
 */

#include <board_config.h>
#include "signals.h"
#include <stdlib.h>
#include <string.h>
#include <float.h>

//-------------------------- random numbers variables ------------------

float rnd_f;
uint32_t random_value;

//-------------------------- sample and echo buffers -------------------

//int16_t echo_buffer[ECHO_BUFFER_LENGTH];
int16_t *echo_buffer;

int echo_buffer_ptr0, echo_buffer_ptr;
int echo_dynamic_loop_length = (I2S_AUDIOFREQ); //default
int echo_dynamic_loop_current_step = 0;
int echo_buffer_low_memory = 0;
float ECHO_MIXING_GAIN_MUL, ECHO_MIXING_GAIN_DIV; //todo replace with one fragment value
float echo_mix_f;

#define ECHO_BUFFER_SIZE_LIMIT (I2S_AUDIOFREQ * 5 / 2)
#define ECHO_BUFFER_SIZE_LIMIT_LOW_MEM (I2S_AUDIOFREQ * 4 / 2)

const int echo_dynamic_loop_steps[ECHO_DYNAMIC_LOOP_STEPS] = {
	(I2S_AUDIOFREQ),			//default
	(I2S_AUDIOFREQ * 3 / 2),	//best one
	(I2S_AUDIOFREQ * 2), 		//interesting
	(I2S_AUDIOFREQ * 5 / 2),	//not bad either
	(I2S_AUDIOFREQ / 3 * 2),	//short delay
	(I2S_AUDIOFREQ / 2),		//short delay
	(I2S_AUDIOFREQ / 3),		//short delay
	(I2S_AUDIOFREQ / 4),		//short delay
	(I2S_AUDIOFREQ / 6),		//short delay
	-11,						//by tempo (1/1)
	-12,						//by tempo (1/2)
	-13,						//by tempo (1/3)
	-14,						//by tempo (1/4)
	-34,						//by tempo (3/4)
	0							//delay off
};

//#define ECHO_BUFFER_LENGTH ((int)(I2S_AUDIOFREQ * 1.23456789f)) //-> nonsense
//#define ECHO_BUFFER_LENGTH ((int)(I2S_AUDIOFREQ * 2.54321f)) //-> max fitting to ram
//#define ECHO_BUFFER_LENGTH (I2S_AUDIOFREQ * 9 / 4) //-> hard to follow
//#define ECHO_BUFFER_LENGTH (I2S_AUDIOFREQ * 4 / 3) //-> not too interesting

int16_t reverb_buffer[REVERB_BUFFER_LENGTH+8];	//the buffer is allocated statically
int reverb_buffer_ptr0, reverb_buffer_ptr;		//pointers for reverb buffer
int reverb_dynamic_loop_length;
float REVERB_MIXING_GAIN_MUL, REVERB_MIXING_GAIN_DIV;
float reverb_mix_f;

//-------------------------- inputs switching during play -------------------

int input_mux_current_step = 0;

//-------------------------- limits and gain multipliers --------------------

float computed_sample_dynamic_limit = 2500.0f;
float OpAmp_ADC12_signal_conversion_factor = OPAMP_ADC12_CONVERSION_FACTOR_DEFAULT;

//-------------------------- inputs init functions  -------------------

void ADC_configure_MIC(int pins_channels)
{
	ADC_InitTypeDef ADC_init_structure;	//Structure for adc confguration
	GPIO_InitTypeDef GPIO_initStructre;	//Structure for analog input pin

	//Clock configuration
	//The ADC1 is connected the APB2 peripheral bus thus we will use its clock source
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC2,ENABLE);
	//Clock for the ADC port!! Do not forget about this one ;)
	RCC_AHB1PeriphClockCmd(RCC_AHB1ENR_GPIOCEN,ENABLE);

	//Analog pin configuration
	if(pins_channels == ADC_PA0_PC0)
	{
		GPIO_initStructre.GPIO_Pin = GPIO_Pin_0;	//The channels 0/10 are connected to PA0/PC0
	}
	else if(pins_channels == ADC_PA2_PC2)
	{
		GPIO_initStructre.GPIO_Pin = GPIO_Pin_2;	//The channels 2/12 are connected to PA2/PC2
	}
	if(pins_channels == ADC_PA0_PC0 || pins_channels == ADC_PA2_PC2) {
		GPIO_initStructre.GPIO_Mode = GPIO_Mode_AN;	//The PC0 pin is configured in analog mode
		GPIO_initStructre.GPIO_PuPd = GPIO_PuPd_NOPULL;	//We don't need any pull up or pull down
		GPIO_Init(GPIOA,&GPIO_initStructre);	//Affecting the port with the initialization structure configuration
		GPIO_Init(GPIOC,&GPIO_initStructre);	//Affecting the port with the initialization structure configuration
	}
	else if(pins_channels == ADC_PA6_PA7)
	{
		GPIO_initStructre.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;	//The channels 6/7 are connected to PA6/PA7
		GPIO_initStructre.GPIO_Mode = GPIO_Mode_AN;	//The PC0 pin is configured in analog mode
		GPIO_initStructre.GPIO_PuPd = GPIO_PuPd_NOPULL;	//We don't need any pull up or pull down
		GPIO_Init(GPIOA,&GPIO_initStructre);	//Affecting the port with the initialization structure configuration
	}

	//ADC structure configuration
	//ADC_DeInit(); //not now, would interfere with IR sensors configuration, if switched during play
	ADC_init_structure.ADC_DataAlign = ADC_DataAlign_Right;	//data converted will be shifted to right
	//Input voltage is converted into a 12bit number giving a maximum value of 4096
	ADC_init_structure.ADC_Resolution = ADC_Resolution_12b;
	//the conversion is continuous, the input data is converted more than once
	ADC_init_structure.ADC_ContinuousConvMode = ENABLE;
	//conversion is synchronous with TIM1 and CC1 (actually I'm not sure about this one :/)
	ADC_init_structure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_CC1;
	ADC_init_structure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;	//no trigger for conversion
	ADC_init_structure.ADC_NbrOfConversion = 1;	//I think this one is clear :p
	ADC_init_structure.ADC_ScanConvMode = DISABLE;	//The scan is configured in one channel
	ADC_Init(ADC1,&ADC_init_structure);	//Initialize ADC with the previous configuration
	ADC_Init(ADC2,&ADC_init_structure);	//Initialize ADC #2 with the previous configuration

	if(pins_channels == ADC_PA0_PC0)
	{
		ADC_RegularChannelConfig(ADC1,ADC_Channel_0,1,ADC_SampleTime_3Cycles); //Select the channel to be read from
		ADC_RegularChannelConfig(ADC2,ADC_Channel_10,1,ADC_SampleTime_3Cycles); //Select the channel to be read from
	}
	else if(pins_channels == ADC_PA2_PC2)
	{
		ADC_RegularChannelConfig(ADC1,ADC_Channel_2,1,ADC_SampleTime_3Cycles); //Select the channel to be read from
		ADC_RegularChannelConfig(ADC2,ADC_Channel_12,1,ADC_SampleTime_3Cycles); //Select the channel to be read from
	}
	else if(pins_channels == ADC_PA6_PA7)
	{
		ADC_RegularChannelConfig(ADC1,ADC_Channel_6,1,ADC_SampleTime_3Cycles); //Select the channel to be read from
		ADC_RegularChannelConfig(ADC2,ADC_Channel_7,1,ADC_SampleTime_3Cycles); //Select the channel to be read from
	}

	ADC_Cmd(ADC1,ENABLE); //Enable ADC conversion
	ADC_Cmd(ADC2,ENABLE); //Enable ADC conversion
}

void ADC_configure_PICKUPS()
{
	ADC_InitTypeDef ADC_init_structure;	//Structure for adc confguration
	GPIO_InitTypeDef GPIO_initStructre;	//Structure for analog input pin

	//Clock configuration
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1,ENABLE);	//The ADC1 is connected the APB2 peripheral bus thus we will use its clock source
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC2,ENABLE); //ADC2 is connected the same way
	RCC_AHB1PeriphClockCmd(RCC_AHB1ENR_GPIOCEN,ENABLE);	//Clock for the ADC port!! Do not forget about this one ;)

	//Analog pin configuration
	GPIO_initStructre.GPIO_Pin = BOARD_ADC_PIN_LINEIN1 | BOARD_ADC_PIN_LINEIN2;	//Pins to which the channels are connected
	GPIO_initStructre.GPIO_Mode = GPIO_Mode_AN;	//Configure in analog mode
	GPIO_initStructre.GPIO_PuPd = GPIO_PuPd_NOPULL;	//We don't need any pull up or pull down
	GPIO_Init(BOARD_ADC_PORT_LINEIN,&GPIO_initStructre);	//Affecting the port with the initialization structure configuration

	//ADC structure configuration
	//ADC_DeInit(); //not now, would interfere with IR sensors configuration, if switched during play
	ADC_init_structure.ADC_DataAlign = ADC_DataAlign_Right;	//data converted will be shifted to right
	ADC_init_structure.ADC_Resolution = ADC_Resolution_12b;	//input voltage is converted into a 12bit number giving a maximum value of 4096
	ADC_init_structure.ADC_ContinuousConvMode = ENABLE;	//the conversion is continuous, the input data is converted more than once
	ADC_init_structure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_CC1;	//conversion is synchronous with TIM1 and CC1 (actually I'm not sure about this one :/)
	ADC_init_structure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;	//no trigger for conversion
	ADC_init_structure.ADC_NbrOfConversion = 1;	//one channel per ADC
	ADC_init_structure.ADC_ScanConvMode = DISABLE;	//the scan is configured in one channel
	//Initialize ADCs with the configuration
	ADC_Init(ADC1,&ADC_init_structure);
	ADC_Init(ADC2,&ADC_init_structure);

	//Select the channels to read from
	ADC_RegularChannelConfig(ADC1,BOARD_ADC_CHANNEL_LINEIN1,1,ADC_SampleTime_3Cycles);
	ADC_RegularChannelConfig(ADC2,BOARD_ADC_CHANNEL_LINEIN2,1,ADC_SampleTime_3Cycles);

	ADC_Cmd(ADC1,ENABLE); //Enable ADC conversion
	ADC_Cmd(ADC2,ENABLE); //Enable ADC conversion
}

void ADC_configure_ADC12_mute()
{
	ADC_Cmd(ADC1,DISABLE); //Disable ADC conversion
	ADC_Cmd(ADC2,DISABLE); //Disable ADC conversion
}

void ADC_set_input_multiplexer(int select)
{
	if(select==0)
	{
		ADC_configure_MIC(BOARD_MIC_ADC_CFG_STRING); //use built-in microphones
	}
	else if(select==1)
	{
		ADC_configure_PICKUPS(); //use pickups input instead of microphones
	}
	else if(select==2)
	{
		ADC_configure_ADC12_mute(); //disable both mics and pickups input
	}
}

/*
int adc_convert()
{
	ADC_SoftwareStartConv(ADC1);					//Start the conversion
	while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC));	//Processing the conversion
	return ADC_GetConversionValue(ADC1);			//Return the converted data
}
*/

void ADC_configure_IR_sensors_as_audio()
{
	ADC_InitTypeDef ADC_init_structure;	//Structure for adc confguration
	GPIO_InitTypeDef GPIO_initStructre;	//Structure for analog input pin

	//Clock configuration
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1,ENABLE);	//The ADC1 is connected the APB2 peripheral bus thus we will use its clock source
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC2,ENABLE); //ADC2 is connected the same way
	RCC_AHB1PeriphClockCmd(RCC_AHB1ENR_GPIOCEN,ENABLE);	//Clock for the ADC port!! Do not forget about this one ;)

	//Analog pin configuration
	GPIO_initStructre.GPIO_Pin = BOARD_ADC_PIN_IR_AUDIO1 | BOARD_ADC_PIN_IR_AUDIO2;	//Pins to which the channels are connected
	GPIO_initStructre.GPIO_Mode = GPIO_Mode_AN;	//Configure in analog mode
	GPIO_initStructre.GPIO_PuPd = GPIO_PuPd_NOPULL;	//We don't need any pull up or pull down
	GPIO_Init(BOARD_ADC_PORT_IR_AUDIO,&GPIO_initStructre);	//Affecting the port with the initialization structure configuration

	//ADC structure configuration
	ADC_DeInit();
	ADC_init_structure.ADC_DataAlign = ADC_DataAlign_Right;	//data converted will be shifted to right
	ADC_init_structure.ADC_Resolution = ADC_Resolution_12b;	//input voltage is converted into a 12bit number giving a maximum value of 4096
	ADC_init_structure.ADC_ContinuousConvMode = ENABLE;	//the conversion is continuous, the input data is converted more than once
	ADC_init_structure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_CC1;	//conversion is synchronous with TIM1 and CC1 (actually I'm not sure about this one :/)
	ADC_init_structure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;	//no trigger for conversion
	ADC_init_structure.ADC_NbrOfConversion = 1;	//one channel per ADC
	ADC_init_structure.ADC_ScanConvMode = DISABLE;	//the scan is configured in one channel
	//Initialize ADCs with the configuration
	ADC_Init(ADC1,&ADC_init_structure);
	ADC_Init(ADC2,&ADC_init_structure);

	//Select the channels to read from
	ADC_RegularChannelConfig(ADC1,BOARD_ADC_CHANNEL_IR_AUDIO1,1,ADC_SampleTime_3Cycles);
	ADC_RegularChannelConfig(ADC2,BOARD_ADC_CHANNEL_IR_AUDIO2,1,ADC_SampleTime_3Cycles);

	ADC_Cmd(ADC1,ENABLE); //Enable ADC conversion
	ADC_Cmd(ADC2,ENABLE); //Enable ADC conversion
}

int magnetic_sensor_calibrated = 0;
int magnetic_sensor_calibration;
int magnetic_sensor_latest_value;

void ADC_configure_MAGNETIC_SENSOR()
{
	ADC_InitTypeDef ADC_init_structure;	//Structure for adc confguration
	GPIO_InitTypeDef GPIO_initStructre;	//Structure for analog input pin

	//Clock configuration
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1,ENABLE);	//The ADC1 is connected the APB2 peripheral bus thus we will use its clock source
	//RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC2,ENABLE); //ADC2 is connected the same way
	RCC_AHB1PeriphClockCmd(RCC_AHB1ENR_GPIOCEN,ENABLE);	//Clock for the ADC port!! Do not forget about this one ;)

	//Analog pin configuration
	GPIO_initStructre.GPIO_Pin = BOARD_ADC_PIN_MAG_SENSOR; //Pins to which the channels are connected
	GPIO_initStructre.GPIO_Mode = GPIO_Mode_AN;	//Configure in analog mode
	GPIO_initStructre.GPIO_PuPd = GPIO_PuPd_NOPULL;	//We don't need any pull up or pull down
	GPIO_Init(BOARD_ADC_PORT_MAG_SENSOR,&GPIO_initStructre);	//Affecting the port with the initialization structure configuration

	//ADC structure configuration
	ADC_DeInit();
	ADC_init_structure.ADC_DataAlign = ADC_DataAlign_Right;	//data converted will be shifted to right
	ADC_init_structure.ADC_Resolution = ADC_Resolution_12b;	//input voltage is converted into a 12bit number giving a maximum value of 4096
	ADC_init_structure.ADC_ContinuousConvMode = ENABLE;	//the conversion is continuous, the input data is converted more than once
	ADC_init_structure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_CC1;	//conversion is synchronous with TIM1 and CC1 (actually I'm not sure about this one :/)
	ADC_init_structure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;	//no trigger for conversion
	ADC_init_structure.ADC_NbrOfConversion = 1;	//one channel per ADC
	ADC_init_structure.ADC_ScanConvMode = DISABLE;	//the scan is configured in one channel
	//Initialize ADCs with the configuration
	ADC_Init(ADC1,&ADC_init_structure);
	//ADC_Init(ADC2,&ADC_init_structure);

	//Select the channels to read from
	ADC_RegularChannelConfig(ADC1,BOARD_ADC_CHANNEL_MAG_SENSOR,1,ADC_SampleTime_3Cycles);
	//ADC_RegularChannelConfig(ADC2,BOARD_ADC_CHANNEL_LINEIN2,1,ADC_SampleTime_3Cycles);

	ADC_Cmd(ADC1,ENABLE); //Enable ADC conversion
	//ADC_Cmd(ADC2,ENABLE); //Enable ADC conversion

	if(!magnetic_sensor_calibrated)
	{
		//calibration
		#define CALIBRATION_STEPS 1024

		int sum = 0;
		for(int i=0;i<CALIBRATION_STEPS;i++)
		{
			sum += ADC1_read();
		}
		magnetic_sensor_calibration = sum / CALIBRATION_STEPS - BOARD_MAG_SENSOR_VALUE_OFFSET;

		magnetic_sensor_calibrated = 1;
	}
}

//IR sensors 1-2-3 by pins as ordered on STM32F407G-DISC1
//PC1 -> ADC123_IN11
//PC3 -> ADC123_IN13
//PA1 -> ADC123_IN1

void ADC_configure_SENSORS(volatile uint16_t *converted_values)
{
	ADC_InitTypeDef ADC_init_structure;	//Structure for adc confguration
	ADC_CommonInitTypeDef	ADC_CommonInitStructure;
	DMA_InitTypeDef			DMA_InitStructure;
	NVIC_InitTypeDef		NVIC_InitStructure2;
	GPIO_InitTypeDef		GPIO_initStructre;	//Structure for analog input pin

	//Clock configuration
	//The ADC3 is connected the APB2 peripheral bus thus we will use its clock source
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC3,ENABLE);
	//Clock for the ADC port!! Do not forget about this one ;)
	RCC_AHB1PeriphClockCmd(RCC_AHB1ENR_GPIOCEN,ENABLE);
	//Clock for the DMA
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);

#ifdef SENSORS_STM32F4_DISC1
	//Analog pin configuration
	GPIO_initStructre.GPIO_Pin = GPIO_Pin_1;	//The channel 1 is connected to PA1
	GPIO_initStructre.GPIO_Mode = GPIO_Mode_AN;	//The PA1 pin is configured in analog mode
	GPIO_initStructre.GPIO_PuPd = GPIO_PuPd_NOPULL;	//We don't need any pull up or pull down
	GPIO_Init(GPIOA,&GPIO_initStructre);	//Affecting the port with the initialization structure configuration
	GPIO_initStructre.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_3;	//The channel 11 is connected to PC1, channel 13 is on PC3
	GPIO_Init(GPIOC,&GPIO_initStructre);	//Affecting the port with the initialization structure configuration
#endif

#if defined(SENSORS_GECHO_V001) || defined(SENSORS_GECHO_V002)
	//Analog pin configuration
	GPIO_initStructre.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3;	//The channels 10..13 are connected to PC0..PC3
	GPIO_initStructre.GPIO_Mode = GPIO_Mode_AN;	//Configure pins in analog mode
	GPIO_initStructre.GPIO_PuPd = GPIO_PuPd_NOPULL;	//We don't need any pull up or pull down
	GPIO_Init(GPIOC,&GPIO_initStructre);	//Affecting the port with the initialization structure configuration
#endif

	/*
	// DMA2_Stream0 channel0 configuration
	DMA_DeInit(DMA2_Stream0);
	DMA_InitStructure.DMA_Channel = DMA_Channel_0;
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&ADC3->DR;
	DMA_InitStructure.DMA_Memory0BaseAddr = converted_values;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
	DMA_InitStructure.DMA_BufferSize = 2;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;
	DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
	DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;
	DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
	DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
	DMA_Init(DMA2_Stream0, &DMA_InitStructure);
	// DMA2_Stream0 enable
	DMA_Cmd(DMA2_Stream0, ENABLE);
	*/

	// DMA2 Stream0 channel2 configuration for ADC3
	DMA_DeInit(DMA2_Stream0);
	DMA_InitStructure.DMA_Channel = DMA_Channel_2;
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&ADC3->DR; // Address of ADC3 Data register RM0090 Table.72
	//DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)&converted_values;
	DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)converted_values;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
	DMA_InitStructure.DMA_BufferSize = ADC_SENSORS;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;//Disable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;
	DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
	DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;
	DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
	DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
	DMA_Init(DMA2_Stream0, &DMA_InitStructure);
	DMA_Cmd(DMA2_Stream0, ENABLE);

	/* ADC Common Init **********************************************************/
	ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div8;
	ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;
	ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;
	ADC_CommonInit(&ADC_CommonInitStructure);

	//ADC structure configuration
	//ADC_DeInit(); //no deinit here, already deinited in MIC config fn
	ADC_init_structure.ADC_DataAlign = ADC_DataAlign_Right;	//data converted will be shifted to right
	//Input voltage is converted into a 12bit number giving a maximum value of 4096
	ADC_init_structure.ADC_Resolution = ADC_Resolution_12b;
	//the conversion is continuous, the input data is converted more than once
	ADC_init_structure.ADC_ContinuousConvMode = ENABLE;
	//conversion is synchronous with TIM1 and CC1 (actually I'm not sure about this one :/)
	ADC_init_structure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_CC1;
	ADC_init_structure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;	//no trigger for conversion

	ADC_init_structure.ADC_NbrOfConversion = ADC_SENSORS;	//I think this one is clear :p
	ADC_init_structure.ADC_ScanConvMode = ENABLE;	//The scan is configured for more channels

	//ADC_init_structure.ADC_NbrOfConversion = 1;	//I think this one is clear :p
	//ADC_init_structure.ADC_ScanConvMode = DISABLE;	//The scan is configured for one channel

	ADC_Init(ADC3,&ADC_init_structure);	//Initialize ADC with the previous configuration

#ifdef SENSORS_STM32F4_DISC1
	//Select the channel to be read from and order
	ADC_RegularChannelConfig(ADC3,ADC_Channel_1,1,ADC_SampleTime_28Cycles);
	ADC_RegularChannelConfig(ADC3,ADC_Channel_13,2,ADC_SampleTime_28Cycles);
	ADC_RegularChannelConfig(ADC3,ADC_Channel_11,3,ADC_SampleTime_28Cycles);
#endif

#ifdef SENSORS_GECHO_V001
	//Select the channel to be read from and order
	ADC_RegularChannelConfig(ADC3,ADC_Channel_10,1,ADC_SampleTime_28Cycles);
	ADC_RegularChannelConfig(ADC3,ADC_Channel_11,2,ADC_SampleTime_28Cycles);
	ADC_RegularChannelConfig(ADC3,ADC_Channel_12,3,ADC_SampleTime_28Cycles);
	ADC_RegularChannelConfig(ADC3,ADC_Channel_13,4,ADC_SampleTime_28Cycles);
#endif

#ifdef SENSORS_GECHO_V002 //swapped S3 and S4 by evolution :)
	//Select the channel to be read from and order
	ADC_RegularChannelConfig(ADC3,ADC_Channel_10,1,ADC_SampleTime_56Cycles);
	ADC_RegularChannelConfig(ADC3,ADC_Channel_11,2,ADC_SampleTime_56Cycles);
	ADC_RegularChannelConfig(ADC3,ADC_Channel_13,3,ADC_SampleTime_56Cycles);
	ADC_RegularChannelConfig(ADC3,ADC_Channel_12,4,ADC_SampleTime_56Cycles);
#endif

	//ADC_EOCOnEachRegularChannelCmd(ADC3,ENABLE);

	/* Enable DMA request after last transfer (Single-ADC mode) */
	ADC_DMARequestAfterLastTransferCmd(ADC3, ENABLE);

	/* Enable ADC3 DMA */
	ADC_DMACmd(ADC3, ENABLE);

	/* Enable ADC3 */
	ADC_Cmd(ADC3,ENABLE); //Enable ADC conversion

	/* Start ADC3 Software Conversion */
	ADC_SoftwareStartConv(ADC3);

	//Once transfer of ADC3 is done, generate interrupt
	NVIC_InitStructure2.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure2.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure2.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure2.NVIC_IRQChannel = DMA2_Stream0_IRQn;
	NVIC_Init(&NVIC_InitStructure2);
}

void DMA2_Stream0_IRQHandler(void)
{
    if(DMA_GetITStatus(DMA2_Stream0, DMA_IT_TCIF0)!=RESET)
    {
        /*
    	test++;

        if (test%2==1)
        {
            GPIOC->BSRRH = GPIO_Pin_13;
        }
        else
        {
            GPIOC->BSRRL = GPIO_Pin_13;
        }

		if (test>=100) test=0;
        */
    	DMA_ClearITPendingBit(DMA2_Stream0, DMA_IT_TCIF0);
    }
}

void ADC_configure_VBAT_channel()
{
	ADC_InitTypeDef ADC_InitStructure;	//Structure for adc confguration
	ADC_CommonInitTypeDef ADC_CommonInitStructure;

	//Clock configuration
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1,ENABLE);	//The ADC1 is connected the APB2 peripheral bus thus we will use its clock source
	RCC_AHB1PeriphClockCmd(RCC_AHB1ENR_GPIOCEN,ENABLE);	//Clock for the ADC port!! Do not forget about this one ;)

	//ADC structure configuration
	ADC_DeInit();

	/* ADC Common Init **********************************************************/
	ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div2;
	ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;
	ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;
	ADC_CommonInit(&ADC_CommonInitStructure);

	/* ADC1 Init ****************************************************************/
	ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
	ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_CC1;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfConversion = 1;
	ADC_Init(ADC1, &ADC_InitStructure);

    /* ADC1 regular channel18 (VBAT) configuration ******************************/
	ADC_RegularChannelConfig(ADC1, ADC_Channel_Vbat, 1, ADC_SampleTime_15Cycles);

	/* Enable VBAT channel */
	ADC_VBATCmd(ENABLE);

	/* Enable ADC1 */
	ADC_Cmd(ADC1,ENABLE); //Enable ADC conversion
}

int ADC1_read()
{
	//ADC_RegularChannelConfig(ADC1,ADC_Channel_0,1,ADC_SampleTime_144Cycles); //Select the channel to be read from
	ADC_SoftwareStartConv(ADC1);					//Start the conversion
	while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC));	//Processing the conversion
	return ADC_GetConversionValue(ADC1);			//Return the converted data
}

int ADC2_read()
{
	//ADC_RegularChannelConfig(ADC1,ADC_Channel_10,2,ADC_SampleTime_144Cycles); //Select the channel to be read from
	//return adc_convert();
	ADC_SoftwareStartConv(ADC2);					//Start the conversion
	while(!ADC_GetFlagStatus(ADC2, ADC_FLAG_EOC));	//Processing the conversion
	return ADC_GetConversionValue(ADC2);			//Return the converted data
}

/*
int ADC3_read()
{
	ADC_SoftwareStartConv(ADC3);					//Start the conversion
	while(!ADC_GetFlagStatus(ADC3, ADC_FLAG_EOC));	//Processing the conversion
	//ADC_ClearFlag(ADC3, ADC_FLAG_EOC); //???
	return ADC_GetConversionValue(ADC3);			//Return the converted data
}
*/

void RNG_Config (void)
{
	RCC_AHB2PeriphClockCmd(RCC_AHB2Periph_RNG, ENABLE);
	RNG_Cmd(ENABLE);
}

//===== BEGIN NOISE SEED BLOCK ===========================================

//#define NOISE_SEED 19.0000000000000000000000000000000000000001
#define NOISE_SEED 19.1919191919191919191919191919191919191919
//#define NOISE_SEED 13.1313131313131313131313131313131313131313
//#define NOISE_SEED 19.7228263362716688952313412753412622364256 //k.l.
//#define NOISE_SEED 19.8381511698353141312312412453512642346256 //d.r.

//===== END NOISE SEED BLOCK =============================================

double static b_noise = NOISE_SEED;

void reset_pseudo_random_seed()
{
	b_noise = NOISE_SEED;
}

void set_pseudo_random_seed(double new_value)
{
	b_noise = new_value;
}

float PseudoRNG1a_next_float()
{
	b_noise = b_noise * b_noise;
	int i_noise = b_noise;
	b_noise = b_noise - i_noise;

	float b_noiseout;
	b_noiseout = b_noise - 0.5;

	b_noise = b_noise + 19;

	return b_noiseout;
}

/*
float PseudoRNG1b_next_float()
{
	double b_noiselast = b_noise;
	b_noise = b_noise + 19;
	b_noise = b_noise * b_noise;
	b_noise = (b_noise + b_noiselast) * 0.5;
	b_noise = b_noise - (int)b_noise;

	return b_noise - 0.5;
}

uint32_t PseudoRNG2_next_int32()
{
	//http://musicdsp.org/showone.php?id=59
	//Type : Linear Congruential, 32bit
	//References : Hal Chamberlain, "Musical Applications of Microprocessors" (Posted by Phil Burk)
	//Notes :
	//This can be used to generate random numeric sequences or to synthesise a white noise audio signal.
	//If you only use some of the bits, use the most significant bits by shifting right.
	//Do not just mask off the low bits.

	//Calculate pseudo-random 32 bit number based on linear congruential method.

	//Change this for different random sequences.
	static unsigned long randSeed = 22222;
	randSeed = (randSeed * 196314165) + 907633515;
	return randSeed;
}
*/

void new_random_value()
{
	float r = PseudoRNG1a_next_float();
	memcpy(&random_value, &r, sizeof(random_value));
}

int fill_with_random_value(char *buffer)
{
	float r = PseudoRNG1a_next_float();
	memcpy(buffer, &r, sizeof(r));
	return sizeof(r);
}

//this seems to be NOT a more optimal way of passing the value
void PseudoRNG_next_value(uint32_t *buffer) //load next random value to the variable
{
	b_noise = b_noise * b_noise;
	int i_noise = b_noise;
	b_noise = b_noise - i_noise;
	float b_noiseout = b_noise - 0.5;
	b_noise = b_noise + NOISE_SEED;
	memcpy(buffer, &b_noiseout, sizeof(b_noiseout));
}

void init_echo_buffer()
{
	echo_buffer = (int16_t*)malloc(ECHO_BUFFER_LENGTH);

	if(echo_buffer==NULL)
	{
		echo_buffer = (int16_t*)malloc(ECHO_BUFFER_LENGTH_LOW_MEM);

		if(echo_buffer==NULL)
		{
			while(1)
			{
				LED_R8_set_byte(0x55);
				Delay(100);
				LED_R8_set_byte(0xaa);
				Delay(100);
			}
		}
		else
		{
			memset(echo_buffer,0,ECHO_BUFFER_LENGTH_LOW_MEM);
			echo_buffer_low_memory = 1;
		}
	}
	else
	{
		memset(echo_buffer,0,ECHO_BUFFER_LENGTH);
	}
	echo_buffer_ptr0 = 0;

	//this is required in case user has pre-set larger echo buffer (e.g. in EEPROM settings from running previous channel)
	if(echo_buffer_low_memory && (echo_dynamic_loop_length > (ECHO_BUFFER_SIZE_LIMIT_LOW_MEM)))
	{
		echo_dynamic_loop_length = ECHO_BUFFER_SIZE_LIMIT_LOW_MEM;
	}

}

int get_echo_length(int step)
{
	int length = echo_dynamic_loop_steps[echo_dynamic_loop_current_step];
	if(length<0)
	{
		if(TEMPO_BY_SAMPLE==0)
		{
			TEMPO_BY_SAMPLE = get_tempo_by_BPM(tempo_bpm);
		}

		if(length==-11) { length = TEMPO_BY_SAMPLE; }
		if(length==-12) { length = TEMPO_BY_SAMPLE / 2; }
		if(length==-13) { length = TEMPO_BY_SAMPLE / 3; }
		if(length==-14) { length = TEMPO_BY_SAMPLE / 4; }
		if(length==-34) { length = TEMPO_BY_SAMPLE * 3 / 4; }

		while(length > (echo_buffer_low_memory?ECHO_BUFFER_SIZE_LIMIT_LOW_MEM:ECHO_BUFFER_SIZE_LIMIT)) //if longer than available memory, halve it
		{
			length /= 2;
		}
	}

	//this is required in case user has pre-set larger echo buffer (e.g. in EEPROM settings from running previous channel)
	if(echo_buffer_low_memory && (length > (ECHO_BUFFER_SIZE_LIMIT_LOW_MEM/2)))
	{
		length = ECHO_BUFFER_SIZE_LIMIT_LOW_MEM/2;
	}

	return length;
}

int find_minimum(float *array, int size)
{
	float min = FLT_MAX;
	int min_pos = -1;

	for(int i=0;i<size;i++)
	{
		if(array[i]<min)
		{
			min = array[i];
			min_pos = i;
		}
	}
	return min_pos;
}

int find_minimum_with_threshold(float *array, int size, int threshold)
{
	float min = FLT_MAX;
	int min_pos = -1;

	for(int i=0;i<size;i++)
	{
		if(array[i]>threshold) //below threshold values
		{
			return -2;
		}

		if(array[i]==0) //bad values
		{
			return -3;
		}

		if(array[i]<min)
		{
			min = array[i];
			min_pos = i;
		}
	}
	return min_pos;
}

int find_minimum_with_threshold_f(float *array, int size, float threshold)
{
	float min = FLT_MAX;
	int min_pos = -1;
	int any_above_threshold = 0;

	for(int i=0;i<size;i++)
	{
		if(array[i]>threshold) //count above threshold values
		{
			any_above_threshold++;
		}

		/*
		if(array[i]==0) //bad values
		{
			return -3;
		}
		*/

		if(array[i]<min)
		{
			min = array[i];
			min_pos = i;
		}
	}

	if(!any_above_threshold)
	{
		return -2;
	}

	return min_pos;
}
