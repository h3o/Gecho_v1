/*
 * leds.h
 *
 *  Created on: Jun 21, 2016
 *      Author: mayo
 *
 *  This file is part of the Gecho Loopsynth Firmware Development Framework.
 *  It can be used within the terms of CC-BY-NC-SA license.
 *  It must not be distributed separately.
 *
 *  Find more information at: http://gechologic.com/gechologists/
 */

#ifndef LEDS_H_
#define LEDS_H_

#include <stdint.h>

extern int LEDs_RED_seq;
extern int sensor_active[];

#define SENSOR_THRESHOLD_ORANGE_4 (sensor_values[1] > IR_sensors_THRESHOLD_7)
#define SENSOR_THRESHOLD_ORANGE_3 (sensor_values[1] > IR_sensors_THRESHOLD_5) //was 500, could be IR_sensors_THRESHOLD_4(400) or IR_sensors_THRESHOLD_5(600)
#define SENSOR_THRESHOLD_ORANGE_2 (sensor_values[1] > IR_sensors_THRESHOLD_3)
#define SENSOR_THRESHOLD_ORANGE_1 (sensor_values[1] > IR_sensors_THRESHOLD_1)

#define SENSOR_THRESHOLD_RED_8 (sensor_values[0] > IR_sensors_THRESHOLD_8)
#define SENSOR_THRESHOLD_RED_7 (sensor_values[0] > IR_sensors_THRESHOLD_7)
#define SENSOR_THRESHOLD_RED_6 (sensor_values[0] > IR_sensors_THRESHOLD_6)
#define SENSOR_THRESHOLD_RED_5 (sensor_values[0] > IR_sensors_THRESHOLD_5)
#define SENSOR_THRESHOLD_RED_4 (sensor_values[0] > IR_sensors_THRESHOLD_4)
#define SENSOR_THRESHOLD_RED_3 (sensor_values[0] > IR_sensors_THRESHOLD_3)
#define SENSOR_THRESHOLD_RED_2 (sensor_values[0] > IR_sensors_THRESHOLD_2)
#define SENSOR_THRESHOLD_RED_1 (sensor_values[0] > IR_sensors_THRESHOLD_1)

#define SENSOR_THRESHOLD_BLUE_1 (sensor_values[2] > IR_sensors_THRESHOLD_8)
#define SENSOR_THRESHOLD_BLUE_2 (sensor_values[2] > IR_sensors_THRESHOLD_6)
#define SENSOR_THRESHOLD_BLUE_3 (sensor_values[2] > IR_sensors_THRESHOLD_4)
#define SENSOR_THRESHOLD_BLUE_4 (sensor_values[2] > IR_sensors_THRESHOLD_2)
#define SENSOR_THRESHOLD_BLUE_5 (sensor_values[2] > IR_sensors_THRESHOLD_1)

#define SENSOR_THRESHOLD_WHITE_1 (sensor_values[3] > IR_sensors_THRESHOLD_8)
#define SENSOR_THRESHOLD_WHITE_2 (sensor_values[3] > IR_sensors_THRESHOLD_7)
#define SENSOR_THRESHOLD_WHITE_3 (sensor_values[3] > IR_sensors_THRESHOLD_6)
#define SENSOR_THRESHOLD_WHITE_4 (sensor_values[3] > IR_sensors_THRESHOLD_5)
#define SENSOR_THRESHOLD_WHITE_5 (sensor_values[3] > IR_sensors_THRESHOLD_4)
#define SENSOR_THRESHOLD_WHITE_6 (sensor_values[3] > IR_sensors_THRESHOLD_3)
#define SENSOR_THRESHOLD_WHITE_7 (sensor_values[3] > IR_sensors_THRESHOLD_2)
#define SENSOR_THRESHOLD_WHITE_8 (sensor_values[3] > IR_sensors_THRESHOLD_1)

#ifdef __cplusplus
 extern "C" {
#endif

void IR_sensors_LED_indicators(int *sensor_values);
void LED_sequencer_indicators(int current_chord, int total_chords);
void display_chord(int *chord_LEDs, int total_leds);

void LEDs_RED_next(int limit, int update_LEDs);
void LEDs_RED_off();

void LEDs_ORANGE_next(int limit);
void LEDs_ORANGE_off();
void LEDs_ORANGE_reset();

void LED_W8_all_ON();
void LED_W8_all_OFF();
void LED_B5_all_OFF();
void LED_O4_all_OFF();
void LED_R8_all_OFF();

void LED_R8_set(int led, int status);
void LED_O4_set(int led, int status);

void LED_R8_set_byte(int value); //default: left to right
void LED_O4_set_byte(int value);
void LED_B5_set_byte(int value);
void LED_W8_set_byte(int value);

void LED_R8_set_byte_RL(int value); //reversed: right to left
void LED_O4_set_byte_RL(int value);
void LED_B5_set_byte_RL(int value);
void LED_W8_set_byte_RL(int value);

void all_LEDs_test();
void all_LEDs_test_seq1();
void all_LEDs_test_seq2();

void KEY_LED_on(int note);
void KEY_LED_all_off();

void display_number_of_chords(int row1, int row2);
void ack_by_signal_LED();
void display_code_challenge(uint32_t *code);

void display_volume_level_indicator_f(float value, float min, float max);
void display_volume_level_indicator_i(int value, int min, int max);
void display_tempo_indicator(int bpm);

void display_BCD_numbers(char *digits, int length);

#ifdef __cplusplus
}
#endif

#endif /* LEDS_H_ */
