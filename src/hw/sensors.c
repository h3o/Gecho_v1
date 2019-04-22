/*
 * sensors.c
 *
 *  Created on: Jun 21, 2016
 *      Author: mayo
 */

#include <board_config.h>
#include "sensors.h"
#include "gpio.h"

//distance sensors
int adc3_val1, adc3_val2, adc3_diff, adc3_sum=0, adc3_cnt=0;

volatile uint16_t ADCConvertedValues[ADC_SENSORS];

uint16_t ADC_measured_vals[2*ADC_SENSORS];
int ADC_last_result[ADC_SENSORS]={0,0,0,0}; //static dim, not futureproof
int ADC_result_ready = 0;
//int ADC_measured_vals_updated = 0;

int process_sensors_phase = 0;
int sensors_loop = 0;


//#define ADC_timing_factor 3 //30ms, 60ms
#define ADC_timing_factor 2

int ADC_process_sensors()
{
	int s;
	if(process_sensors_phase==0)
	{
		for(s=0;s<ADC_SENSORS;s++)
		{
			ADC_measured_vals[s] = ADCConvertedValues[s];
		}
		//ADC_measured_vals_updated = 1;

#ifdef BOARD_GECHO_V001
		#ifdef IR_PAIRS_CONTROLLED_BY_TWO_LINES
			LED_RDY_OFF;
		#endif

		LED_IR_DETECTORS_ON;
#endif

#ifdef BOARD_GECHO_V002
		LED_RDY_ON;
		LED_IR_DETECTORS_OFF;
#endif

		//LED_B5_0_ON //rattling test #1
		//LED_B5_1_ON
		//LED_B5_2_ON
		//LED_B5_3_ON
		//LED_B5_4_ON
		//LED_W8_0_ON
		//LED_W8_1_ON
		//LED_W8_2_ON
		//LED_W8_3_ON
		//LED_W8_4_ON
		//GPIOA->BSRRL = GPIO_Pin_0; //rattling test #2
	}
	else if(process_sensors_phase==ADC_timing_factor)
	{
		for(s=0;s<ADC_SENSORS;s++)
		{
			ADC_measured_vals[s+ADC_SENSORS] = ADCConvertedValues[s];
		}
		//ADC_measured_vals_updated = 1;

#ifdef BOARD_GECHO_V001
		LED_IR_DETECTORS_OFF;

		#ifdef IR_PAIRS_CONTROLLED_BY_TWO_LINES
			LED_RDY_ON;
		#endif
#endif

#ifdef BOARD_GECHO_V002
		LED_RDY_OFF;
		LED_IR_DETECTORS_ON;
#endif
		//LED_B5_0_OFF //rattling test #1
		//LED_B5_1_OFF
		//LED_B5_2_OFF
		//LED_B5_3_OFF
		//LED_B5_4_OFF
		//LED_W8_0_OFF
		//LED_W8_1_OFF
		//LED_W8_2_OFF
		//LED_W8_3_OFF
		//LED_W8_4_OFF
		//GPIOA->BSRRH = GPIO_Pin_0; //rattling test #2

		for(s=0;s<ADC_SENSORS;s++)
		{
			ADC_last_result[s] = ADC_measured_vals[s] - ADC_measured_vals[s+ADC_SENSORS];
		}
		ADC_result_ready = 1;
	}

	process_sensors_phase++;
	if(process_sensors_phase==ADC_timing_factor*2)
	{
		process_sensors_phase = 0;
	}
	return ADC_result_ready;
}

void ADC_test_SENSORS()    //test timer and ADC3
{
	//int adc3_val1, adc3_val2;

	while(1)
	{
		//LED_BLUE_OFF;
		if (ADCConvertedValues[1] != 0xFFFF)
	    {
			//LED_BLUE_ON;
			ADCConvertedValues[1] = 0xFFFF;
	    }
	}

	/*
	while(1)
    {
		LED_BLUE_ON;
		LED_IR_DETECTORS_ON;
		Delay(200);
		adc3_val1 = ADC3_read();
		//adc3_val2 = ADC3_read();
		LED_BLUE_OFF;
		LED_IR_DETECTORS_OFF;
		Delay(800);
		//adc3_val1 = ADC3_read();
		adc3_val2 = ADC3_read();
    }
    */

}
