/*
 * sensors.h
 *
 *  Created on: Jun 21, 2016
 *      Author: mayo
 */

#ifndef SENSORS_H_
#define SENSORS_H_

#include <board_config.h>
#include "stm32f4xx_adc.h"

#define USE_IR_SENSORS

#define IR_sensors_THRESHOLD_1 150
#define IR_sensors_THRESHOLD_2 250
#define IR_sensors_THRESHOLD_3 350
#define IR_sensors_THRESHOLD_4 450
#define IR_sensors_THRESHOLD_5 600
#define IR_sensors_THRESHOLD_6 800
#define IR_sensors_THRESHOLD_7 1000
#define IR_sensors_THRESHOLD_8 1200
#define IR_sensors_THRESHOLD_9 2000

//STM32F4_DISC1
//#define LED_IR_DETECTORS_ON   GPIOA->BSRRL = GPIO_Pin_3;
//#define LED_IR_DETECTORS_OFF  GPIOA->BSRRH = GPIO_Pin_3;

#ifdef BOARD_GECHO_V001

	//Gecho V0.01 - PA1 - problematic
	#define LED_IR_DETECTORS_ON   GPIOA->BSRRL = GPIO_Pin_1;
	#define LED_IR_DETECTORS_OFF  GPIOA->BSRRH = GPIO_Pin_1;

	//PA0 - rattling test #2
	//#define LED_IR_DETECTORS_ON   GPIOA->BSRRL = GPIO_Pin_0;
	//#define LED_IR_DETECTORS_OFF  GPIOA->BSRRH = GPIO_Pin_0;

	//Rattling test #3 via PB14
	//#define LED_IR_DETECTORS_ON   GPIOB->BSRRL = GPIO_Pin_14;
	//#define LED_IR_DETECTORS_OFF  GPIOB->BSRRH = GPIO_Pin_14;

	//PD2 - alternative test - wired to But2
	//#define LED_IR_DETECTORS_ON   GPIOD->BSRRL = GPIO_Pin_2;
	//#define LED_IR_DETECTORS_OFF  GPIOD->BSRRH = GPIO_Pin_2;

#endif

#ifdef BOARD_GECHO_V002
	#define LED_IR_DETECTORS_ON		GPIOB->BSRRL = GPIO_Pin_15;
	#define LED_IR_DETECTORS_OFF	GPIOB->BSRRH = GPIO_Pin_15;
#endif

#define CLEAR_ADC_RESULT_RDY_FLAG	ADC_result_ready = 0;

#ifdef __cplusplus
 extern "C" {
#endif

//distance sensors
extern int adc3_val1, adc3_val2, adc3_diff, adc3_sum, adc3_cnt;
extern volatile uint16_t ADCConvertedValues[];

extern uint16_t ADC_measured_vals[];
extern int ADC_last_result[];
extern int ADC_result_ready;
extern int ADC_measured_vals_updated;

extern int sensors_loop;

// --------------------------------------

int ADC_process_sensors();
void ADC_test_SENSORS();

#ifdef __cplusplus
}
#endif

#endif /* SENSORS_H_ */
