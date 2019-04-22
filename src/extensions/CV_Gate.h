/*
 * CV_Gate.h
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

#ifndef CV_GATE_H_
#define CV_GATE_H_

#define CV_GATE_PB0_PB1 1
#define CV_GATE_PC0_PB0 2

#define GATE_PB0 (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_0)==1)


#ifdef __cplusplus
 extern "C" {
#endif

void ADC_configure_CV_GATE(int signals);
int Calibrate_CV();
int Calculate_CV_values(int *reference_notes, int *cv_values_buffer);
int find_nearest_note_freq(int CV_ADC_value);

#ifdef __cplusplus
}
#endif

#endif /* CV_GATE_H_ */
