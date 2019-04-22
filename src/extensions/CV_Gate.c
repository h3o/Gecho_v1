/*
* CV_Gate.c
 *
 *  Created on: May 28, 2017
 *      Author: mayo
 *
 *  This file is part of the Gecho Loopsynth Firmware Development Framework.
 *  It can be used within the terms of CC-BY-NC-SA license.
 *  It must not be distributed separately.
 *
 *  Find more information at: http://gechologic.com/gechologists/
 */

#include "CV_Gate.h"
#include <hw/gpio.h>
#include <hw/sensors.h>
#include <stdlib.h>

int *calibrated_cv_values;

void ADC_configure_CV_GATE(int signals)
{
	// struct to initialize GPIO pins
	GPIO_InitTypeDef GPIO_InitStructure;

	if (signals == CV_GATE_PB0_PB1)
	{

	}
	else if (signals == CV_GATE_PC0_PB0)
	{
		//configure PB0 as input
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN; //Input mode
		GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //Output type push-pull
		GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL; //Without pull up/down resistors
		GPIO_InitStructure.GPIO_Speed = GPIO_SPEED_LEDS_BUTTONS;
		GPIO_Init(GPIOB, &GPIO_InitStructure);
	}
}

int Calibrate_CV()
{
	//turn off IR sensor LEDs
	LED_RDY_OFF;
	LED_IR_DETECTORS_OFF;

	while(GATE_PB0); //wait till gate level goes down

	while(!GATE_PB0) { //indication repeats until gate goes back up

		int delay = 500 / 8;

		LED_W8_0_ON;
		Delay(delay);
		LED_W8_0_OFF;
		Delay(delay);

		LED_W8_4_ON;
		Delay(delay);
		LED_W8_4_OFF;
		Delay(delay);

		LED_W8_7_ON;
		Delay(delay);
		LED_W8_7_OFF;
		Delay(delay);

		Delay(500 - 6 * delay);
		//ADC_process_sensors();
	}

	int notes[3];
	int gate_level, gate_stable; //de-bouncing
	uint32_t gate_avg = 0;
	int gate_avg_samples = 0;
	#define GATE_THRESHOLD 50000

	for(int note = 0; note < 3; note++)
	{
		//CLEAR_ADC_RESULT_RDY_FLAG;
		//Delay(100);
		gate_stable = 0;
		gate_level = 0;
		while(!gate_level || notes[note]>3000) { //wait till gate level goes up
			//ADC_process_sensors();
			//notes[note] = ADCConvertedValues[0];
			if(GATE_PB0)
			{
				gate_stable++;
				if(gate_stable > GATE_THRESHOLD / 4) //at 25% of threshold, start collecting values
				{
					gate_avg += ADCConvertedValues[0];
					gate_avg_samples++;
				}
				if(gate_stable==GATE_THRESHOLD)
				{
					gate_level = 1;
					notes[note] = gate_avg / gate_avg_samples;
				}
			}
			else
			{
				gate_stable = 0;
			}
		}
		//while(!ADC_process_sensors()); //wait till signal is measured
		//notes[note] = ADC_measured_vals[0];
		//notes[note] = ADCConvertedValues[0];
		//CLEAR_ADC_RESULT_RDY_FLAG;

		if(note == 0)
		{
			LED_W8_0_ON;
		}
		else if(note == 1)
		{
			LED_W8_4_ON;
		}
		else if(note == 2)
		{
			LED_W8_7_ON;
		}

		//Delay(100);
		//while(GATE_PB0) { //wait till gate level goes down again
			//ADC_process_sensors();
			//Delay(50);
		//}

		gate_stable = 0;
		gate_level = 1;
		while(gate_level) { //wait till gate level goes down again
			if(!GATE_PB0)
			{
				gate_stable++;
				if(gate_stable==GATE_THRESHOLD)
				{
					gate_level = 0;
				}
			}
			else
			{
				gate_stable = 0;
			}
		}
	}

	//after 3 notes captured, analyze values and extrapolate the rest of the voltage scale
	calibrated_cv_values = (int*)malloc(128); //full scale, 10 and half octaves
	return Calculate_CV_values(notes, calibrated_cv_values); //return success
}

int Calculate_CV_values(int *reference_notes, int *cv_values_buffer)
{
	//notes will be numbered according to http://www.electronics.dit.ie/staff/tscarff/Music_technology/midi/midi_note_numbers_for_octaves.htm

	//we assume reference notes to be C4, G4 and C5
	cv_values_buffer[48] = reference_notes[0];
	cv_values_buffer[55] = reference_notes[1];
	cv_values_buffer[60] = reference_notes[2];

	float one_octave = cv_values_buffer[60] - cv_values_buffer[48];
	float one_note = one_octave / 12;
	float g4 = cv_values_buffer[48] + 7 * one_note;
	float err = g4 - (float)cv_values_buffer[55];
	if(abs(err) > one_note / 2)
	{
		//too much imprecision
		return (int)err;
	}

	for(int n=0;n<128;n++)
	{
		cv_values_buffer[n] = reference_notes[0] + one_note * (n-48);
	}
	return 0; //error is low enough
}

int find_nearest_note_freq(int CV_ADC_value)
{
	//*calibrated_cv_values;
}
