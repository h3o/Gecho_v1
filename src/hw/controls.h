/*
 * controls.h
 *
 *  Created on: Sep 24, 2016
 *      Author: mayo
 *
 *  This file is part of the Gecho Loopsynth Firmware Development Framework.
 *  It can be used within the terms of CC-BY-NC-SA license.
 *  It must not be distributed separately.
 *
 *  Find more information at: http://gechologic.com/gechologists/
 */

#ifndef CONTROLS_H_
#define CONTROLS_H_

#include "codec.h"
#include "stdbool.h"

//click
#define ADC_SENSOR_THRESHOLD 400
#define ADC_SENSOR_CLICK_LOOP_DELAY 5

//gesture
#define ADC_SENSOR_THRESHOLD_DIFF 10
#define ADC_SENSOR_GESTURE_LOOP_DELAY 20
#define ADC_SENSOR_GESTURE_INIT_DELAY 5

#define USER_BUTTONS_N 4
#define USER_BUTTONS_DEBOUNCE 5 //limit for "pressed" state counter, until button is considered pressed
#define SCAN_BUTTONS_LOOP_DELAY 10 //buttons are scanned each 10ms
#define PWR_BUTTON_THRESHOLD_1 10 //SCAN_BUTTONS_LOOP_DELAY * x ms -> 100ms
#define PWR_BUTTON_THRESHOLD_2 100 //SCAN_BUTTONS_LOOP_DELAY * x ms -> 1 sec
#define PWR_BUTTON_THRESHOLD_3 300 //SCAN_BUTTONS_LOOP_DELAY * x ms -> 3 sec

#define PRESS_ORDER_MAX 19 //maximum lenght of main-menu buttons sequence to parse

extern const char *BINARY_ID;
extern const char *FW_VERSION;

#define UID_BASE_ADDRESS 0x1FFF7A10 //location of MCU's factory-programmed unique ID
#define CODE_CHALLENGE_KEY "ouroboros_of_the_sound"

extern int mode_set_flag;
extern bool PROG_wavetable_sample;
extern bool PROG_audio_input_microphones;
extern bool PROG_add_OpAmp_ADC12_signal;
extern bool PROG_mix_input_signal;


#define FILTER_PAIRS 8 //default number of filter pairs
extern int ACTIVE_FILTERS_PAIRS;

#ifdef __cplusplus
 extern "C" {
#endif

uint64_t select_channel();
uint64_t get_user_buttons_sequence(int *press_order_ptr, int *press_order);

int preview_composition(char *data);

int scan_buttons();
void user_button_pressed(int button, int *binary_status_O4, int *press_order_ptr, int *press_order);
void buttons_controls_during_play();

//int wait_for_gesture();
//int wait_for_sensor_click();

void l33tsp34k(char *buffer, int buf_length);
void string_to_14(char *buffer, int buf_length);
void sha1_to_hex(char *code_sha1_hex, uint8_t *code_sha1);
int verify_activation_unlock_code(char *code);
void send_activation_info_USART();

#ifdef __cplusplus
}
#endif

#endif /* CONTROLS_H_ */
