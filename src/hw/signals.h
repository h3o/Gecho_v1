/*
 * signals.h
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

#ifndef SIGNALS_H_
#define SIGNALS_H_

#include <stdbool.h>
#include <hw/codec.h>
#include "stm32f4xx_adc.h"

#define ADC_PA0_PC0 10
#define ADC_PA2_PC2 20
#define ADC_PA6_PA7 30

extern uint32_t random_value;
extern float rnd_f;

#ifdef __cplusplus
 extern "C" {
#endif

void ADC_configure_MIC(int pins_channels);
void ADC_configure_PICKUPS();
void ADC_configure_ADC12_mute();
void ADC_set_input_multiplexer(int select);

void ADC_configure_IR_sensors_as_audio();
void ADC_configure_MAGNETIC_SENSOR();
void ADC_configure_SENSORS(volatile uint16_t *converted_values);
void ADC_configure_VBAT_channel();

int ADC1_read();
int ADC2_read();
//int ADC3_read();

void RNG_Config (void);

void reset_pseudo_random_seed();
void set_pseudo_random_seed(double new_value);

float PseudoRNG1a_next_float();
//float PseudoRNG1b_next_float();
//uint32_t PseudoRNG2_next_int32();

void new_random_value();
int fill_with_random_value(char *buffer);
void PseudoRNG_next_value(uint32_t *buffer);

void DMA2_Stream1_IRQHandler(void);

void init_echo_buffer();
int get_echo_length(int step);

int find_minimum(float *array, int size);
int find_minimum_with_threshold(float *array, int size, int threshold);
int find_minimum_with_threshold_f(float *array, int size, float threshold);

extern int magnetic_sensor_calibration;
extern int magnetic_sensor_latest_value;

//-------------------------- sample and echo buffers -------------------

#define ECHO_DYNAMIC_LOOP_STEPS 15
//the length is now controlled dynamically (as defined in signals.c)

//#define ECHO_BUFFER_LENGTH (I2S_AUDIOFREQ * 5 / 2)	//longest delay currently used
#define ECHO_BUFFER_LENGTH (I2S_AUDIOFREQ * 5)	//longest delay currently used, now allocating as bytes so *2
#define ECHO_BUFFER_LENGTH_LOW_MEM (I2S_AUDIOFREQ * 4)	//fallback buffer length when not enough memory for default (larger)

//#define ECHO_BUFFER_LENGTH (I2S_AUDIOFREQ * 5 / 4)	//longest delay that can be used at 44.1kHz

//extern int16_t echo_buffer[ECHO_BUFFER_LENGTH];
extern int16_t *echo_buffer; //the buffer is allocated dynamically

extern int echo_buffer_ptr0, echo_buffer_ptr; //pointers for echo buffer
extern int echo_dynamic_loop_length;
extern int echo_dynamic_loop_current_step;
extern int echo_buffer_low_memory;
#define ECHO_BUFFER_LOW_MEM_SKIP 3
extern float ECHO_MIXING_GAIN_MUL, ECHO_MIXING_GAIN_DIV;
extern float echo_mix_f;
extern const int echo_dynamic_loop_steps[ECHO_DYNAMIC_LOOP_STEPS];
extern bool PROG_add_echo;

#define REVERB_BUFFER_LENGTH (I2S_AUDIOFREQ / 21)	//longest reverb currently used (1050 samples @ 22.5k)

extern int16_t reverb_buffer[REVERB_BUFFER_LENGTH+8];	//the buffer is allocated statically
extern int reverb_buffer_ptr0, reverb_buffer_ptr;		//pointers for reverb buffer
extern int reverb_dynamic_loop_length;
extern float REVERB_MIXING_GAIN_MUL, REVERB_MIXING_GAIN_DIV;
extern float reverb_mix_f;

//-------------------------- inputs switching during play -------------------

#define INPUT_MUX_MICS 0
#define INPUT_MUX_PICKUPS 1
#define INPUT_MUX_OFF 2

#define INPUT_MUX_STEPS 3
extern int input_mux_current_step;

//-------------------------- limits and gain multipliers --------------------

#define OPAMP_ADC12_CONVERSION_FACTOR_DEFAULT	0.00292f //by ref version, 12/4096... previous value: 0.00500f
#define OPAMP_ADC12_CONVERSION_FACTOR_MIN		0.00010f
#define OPAMP_ADC12_CONVERSION_FACTOR_MAX		0.02000f
#define OPAMP_ADC12_CONVERSION_FACTOR_STEP		0.00010f
extern float OpAmp_ADC12_signal_conversion_factor;

#define OPAMP_ADC12_CONVERSION_FACTOR_BOOST_LOW_PASS 4.0f

#define COMPUTED_SAMPLE_MIXING_LIMIT_UPPER 32000.0f
#define COMPUTED_SAMPLE_MIXING_LIMIT_LOWER -32000.0f
extern float computed_sample_dynamic_limit;

#define PREAMP_BOOST 9		//multiplier for signal from ADC1/ADC2 (microphones and piezo pickups input)
#define MAIN_VOLUME 64.0f	//multiplier for samples mixing volume (float to int)

#define UNFILTERED_SIGNAL_VOLUME 64.0f			//for mixing white noise in
#define FLASH_SAMPLES_MIXING_VOLUME 0.5f		//for mixing the samples from flash
#define COMPUTED_SAMPLES_MIXING_VOLUME 2.0f	//used to adjust volume when converting from float to int
//TODO: 4 was used while recording signal fed to pickup inputs

//#define NOISE_BOOST_BY_SENSOR_LIMIT 3.0f
//#define NOISE_BOOST_BY_SENSOR_LIMIT 4.0f
//#define NOISE_BOOST_BY_SENSOR_LIMIT 6.0f
#define NOISE_BOOST_BY_SENSOR_LIMIT 9.0f
#define NOISE_BOOST_BY_SENSOR_DEFAULT 2.0f

#define AUTOCORRELATION_PREAMP_BOOST_MIC 12
#define AUTOCORRELATION_PREAMP_BOOST_LINEIN 19

#ifdef __cplusplus
}
#endif

#endif /* SIGNALS_H_ */
