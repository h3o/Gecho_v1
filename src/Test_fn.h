/*
 * test_fn.h
 *
 *  Created on: Apr 27, 2016
 *      Author: mayo
 */

#ifndef TEST_FN_H_
#define TEST_FN_H_

#include <dsp/Detectors.h>
#include <dsp/Filters.h>
#include <string.h>

extern Filters *fil;
extern volatile uint32_t sampleCounter;
extern uint32_t random_value;
extern float sample_f[2],sample_synth[2],sample_mix;
extern float volume2;
extern volatile int16_t sample_i16;
extern float noise_volume, noise_volume_max, noise_boost_by_sensor;

#ifdef __cplusplus
 extern "C" {
#endif

void MAGNETIC_sensors_test(int sensor_value);

#ifdef GORS
void Goertzel_detectors_capture(int tones_to_find, int *found_tones, char *goertzel_octaves);
void AutoCorrelation_capture(int tones_to_find, int *found_tones, int preamp_boost);
void MagneticRing_capture(int tones_to_find, int *found_tones);
#endif

void Stylus_test();
int IR_remote_capture();

void check_battery_level();

void show_board_serial_no();
void show_firmware_version();

void acoustic_location_test();
void direct_buttons_LEDs_test();

#ifdef __cplusplus
}
#endif

#endif /* TEST_FN_H_ */
