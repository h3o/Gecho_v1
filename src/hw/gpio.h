/*
 * gpio.h
 *
 *  Created on: Jun 21, 2016
 *      Author: mayo
 */

#ifndef GPIO_H_
#define GPIO_H_

#include <stdbool.h>
#include <board_config.h>
#include <hw/stm32f4xx_it.h>
#include "stm32f4xx.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"

#define GPIO_SPEED_LEDS_BUTTONS		GPIO_Speed_2MHz
#define GPIO_SPEED_USART			GPIO_Speed_50MHz

#define LED_R8_0_ON   GPIOB->BSRRL = GPIO_Pin_0
#define LED_R8_0_OFF  GPIOB->BSRRH = GPIO_Pin_0
#define LED_R8_1_ON   GPIOB->BSRRL = GPIO_Pin_1
#define LED_R8_1_OFF  GPIOB->BSRRH = GPIO_Pin_1
#define LED_R8_2_ON   GPIOB->BSRRL = GPIO_Pin_2
#define LED_R8_2_OFF  GPIOB->BSRRH = GPIO_Pin_2
#define LED_R8_3_ON   GPIOB->BSRRL = GPIO_Pin_3
#define LED_R8_3_OFF  GPIOB->BSRRH = GPIO_Pin_3
#ifdef GECHO_V2
#define LED_R8_4_ON   GPIOA->BSRRL = GPIO_Pin_7
#define LED_R8_4_OFF  GPIOA->BSRRH = GPIO_Pin_7
#else
#define LED_R8_4_ON   GPIOB->BSRRL = GPIO_Pin_4
#define LED_R8_4_OFF  GPIOB->BSRRH = GPIO_Pin_4
#endif
#define LED_R8_5_ON   GPIOB->BSRRL = GPIO_Pin_5
#define LED_R8_5_OFF  GPIOB->BSRRH = GPIO_Pin_5
#define LED_R8_6_ON   GPIOB->BSRRL = GPIO_Pin_6
#define LED_R8_6_OFF  GPIOB->BSRRH = GPIO_Pin_6
#define LED_R8_7_ON   GPIOB->BSRRL = GPIO_Pin_7
#define LED_R8_7_OFF  GPIOB->BSRRH = GPIO_Pin_7

#define LED_O4_0_ON   GPIOB->BSRRL = GPIO_Pin_10
#define LED_O4_0_OFF  GPIOB->BSRRH = GPIO_Pin_10
#define LED_O4_1_ON   GPIOB->BSRRL = GPIO_Pin_11
#define LED_O4_1_OFF  GPIOB->BSRRH = GPIO_Pin_11
#define LED_O4_2_ON   GPIOB->BSRRL = GPIO_Pin_12
#define LED_O4_2_OFF  GPIOB->BSRRH = GPIO_Pin_12
#define LED_O4_3_ON   GPIOB->BSRRL = GPIO_Pin_13
#define LED_O4_3_OFF  GPIOB->BSRRH = GPIO_Pin_13

#define LED_RDY_ON   GPIOB->BSRRL = GPIO_Pin_14
#define LED_RDY_OFF  GPIOB->BSRRH = GPIO_Pin_14

#ifdef BOARD_GECHO_V001
#define LED_SIG_ON   GPIOB->BSRRL = GPIO_Pin_15
#define LED_SIG_OFF  GPIOB->BSRRH = GPIO_Pin_15
#endif
#ifdef BOARD_GECHO_V002
#define LED_SIG_ON   GPIOA->BSRRL = GPIO_Pin_5
#define LED_SIG_OFF  GPIOA->BSRRH = GPIO_Pin_5
#endif

#ifdef LEDS_WHITE_BLUE_INVERTED
#define LED_B5_0_ON   GPIOC->BSRRH = GPIO_Pin_4
#define LED_B5_0_OFF  GPIOC->BSRRL = GPIO_Pin_4
#define LED_B5_1_ON   GPIOC->BSRRH = GPIO_Pin_5
#define LED_B5_1_OFF  GPIOC->BSRRL = GPIO_Pin_5
#define LED_B5_2_ON   GPIOC->BSRRH = GPIO_Pin_6
#define LED_B5_2_OFF  GPIOC->BSRRL = GPIO_Pin_6
#define LED_B5_3_ON   GPIOC->BSRRH = GPIO_Pin_8
#define LED_B5_3_OFF  GPIOC->BSRRL = GPIO_Pin_8
#define LED_B5_4_ON   GPIOC->BSRRH = GPIO_Pin_9
#define LED_B5_4_OFF  GPIOC->BSRRL = GPIO_Pin_9
#else
#define LED_B5_0_ON   GPIOC->BSRRL = GPIO_Pin_4
#define LED_B5_0_OFF  GPIOC->BSRRH = GPIO_Pin_4
#define LED_B5_1_ON   GPIOC->BSRRL = GPIO_Pin_5
#define LED_B5_1_OFF  GPIOC->BSRRH = GPIO_Pin_5
#define LED_B5_2_ON   GPIOC->BSRRL = GPIO_Pin_6
#define LED_B5_2_OFF  GPIOC->BSRRH = GPIO_Pin_6
#define LED_B5_3_ON   GPIOC->BSRRL = GPIO_Pin_8
#define LED_B5_3_OFF  GPIOC->BSRRH = GPIO_Pin_8
#define LED_B5_4_ON   GPIOC->BSRRL = GPIO_Pin_9
#define LED_B5_4_OFF  GPIOC->BSRRH = GPIO_Pin_9
#endif

#ifdef BYPASS_W8_0
#define LED_W8_0_ON
#define LED_W8_0_OFF
#else
#ifdef LEDS_WHITE_BLUE_INVERTED
#define LED_W8_0_ON   GPIOA->BSRRH = GPIO_Pin_8
#define LED_W8_0_OFF  GPIOA->BSRRL = GPIO_Pin_8
#else
#define LED_W8_0_ON   GPIOA->BSRRL = GPIO_Pin_8
#define LED_W8_0_OFF  GPIOA->BSRRH = GPIO_Pin_8
#endif
#endif

#ifdef CAN_USE_CH340G_LEDS
#ifdef LEDS_WHITE_BLUE_INVERTED
	#define LED_W8_1_ON   GPIOA->BSRRH = GPIO_Pin_9
	#define LED_W8_1_OFF  GPIOA->BSRRL = GPIO_Pin_9
	#define LED_W8_2_ON   GPIOA->BSRRH = GPIO_Pin_10
	#define LED_W8_2_OFF  GPIOA->BSRRL = GPIO_Pin_10
#else
	#define LED_W8_1_ON   GPIOA->BSRRL = GPIO_Pin_9
	#define LED_W8_1_OFF  GPIOA->BSRRH = GPIO_Pin_9
	#define LED_W8_2_ON   GPIOA->BSRRL = GPIO_Pin_10
	#define LED_W8_2_OFF  GPIOA->BSRRH = GPIO_Pin_10
#endif
#endif

#ifdef LEDS_WHITE_BLUE_INVERTED
#define LED_W8_3_ON   GPIOA->BSRRH = GPIO_Pin_11
#define LED_W8_3_OFF  GPIOA->BSRRL = GPIO_Pin_11
#define LED_W8_4_ON   GPIOA->BSRRH = GPIO_Pin_12
#define LED_W8_4_OFF  GPIOA->BSRRL = GPIO_Pin_12
#else
#define LED_W8_3_ON   GPIOA->BSRRL = GPIO_Pin_11
#define LED_W8_3_OFF  GPIOA->BSRRH = GPIO_Pin_11
#define LED_W8_4_ON   GPIOA->BSRRL = GPIO_Pin_12
#define LED_W8_4_OFF  GPIOA->BSRRH = GPIO_Pin_12
#endif

#ifdef CAN_BLOCK_SWD_DEBUG
#ifdef LEDS_WHITE_BLUE_INVERTED
	#define LED_W8_5_ON   GPIOA->BSRRH = GPIO_Pin_13
	#define LED_W8_5_OFF  GPIOA->BSRRL = GPIO_Pin_13
	#define LED_W8_6_ON   GPIOA->BSRRH = GPIO_Pin_14
	#define LED_W8_6_OFF  GPIOA->BSRRL = GPIO_Pin_14
#else
	#define LED_W8_5_ON   GPIOA->BSRRL = GPIO_Pin_13
	#define LED_W8_5_OFF  GPIOA->BSRRH = GPIO_Pin_13
	#define LED_W8_6_ON   GPIOA->BSRRL = GPIO_Pin_14
	#define LED_W8_6_OFF  GPIOA->BSRRH = GPIO_Pin_14
#endif
#else
	#define LED_W8_5_ON		//these macros will do nothing if SWD debug enabled
	#define LED_W8_5_OFF
	#define LED_W8_6_ON
	#define LED_W8_6_OFF
#endif

#ifdef LEDS_WHITE_BLUE_INVERTED
#define LED_W8_7_ON   GPIOA->BSRRH = GPIO_Pin_15
#define LED_W8_7_OFF  GPIOA->BSRRL = GPIO_Pin_15
#else
#define LED_W8_7_ON   GPIOA->BSRRL = GPIO_Pin_15
#define LED_W8_7_OFF  GPIOA->BSRRH = GPIO_Pin_15
#endif

#define KEY_C_ON LED_W8_0_ON
#define KEY_C_OFF LED_W8_0_OFF
#ifdef CAN_USE_CH340G_LEDS
	#define KEY_D_ON LED_W8_1_ON
	#define KEY_D_OFF LED_W8_1_OFF
	#define KEY_E_ON LED_W8_2_ON
	#define KEY_E_OFF LED_W8_2_OFF
#else
	#define KEY_D_ON
	#define KEY_D_OFF

#define KEY_E_ON
	#define KEY_E_OFF
#endif
#define KEY_F_ON LED_W8_3_ON
#define KEY_F_OFF LED_W8_3_OFF
#define KEY_G_ON LED_W8_4_ON
#define KEY_G_OFF LED_W8_4_OFF
#ifdef CAN_BLOCK_SWD_DEBUG
	#define KEY_A_ON LED_W8_5_ON
	#define KEY_A_OFF LED_W8_5_OFF
	#define KEY_H_ON LED_W8_6_ON
	#define KEY_H_OFF LED_W8_6_OFF
#else
	#define KEY_A_ON LED_O4_2_ON
	#define KEY_A_OFF LED_O4_2_OFF
	#define KEY_H_ON LED_O4_3_ON
	#define KEY_H_OFF LED_O4_3_OFF
#endif
#define KEY_C2_ON LED_W8_7_ON
#define KEY_C2_OFF LED_W8_7_OFF

#define KEY_Cis_ON LED_B5_0_ON
#define KEY_Cis_OFF LED_B5_0_OFF
#define KEY_Dis_ON LED_B5_1_ON
#define KEY_Dis_OFF LED_B5_1_OFF
#define KEY_Fis_ON LED_B5_2_ON
#define KEY_Fis_OFF LED_B5_2_OFF
#define KEY_Gis_ON LED_B5_3_ON
#define KEY_Gis_OFF LED_B5_3_OFF
#define KEY_Ais_ON LED_B5_4_ON
#define KEY_Ais_OFF LED_B5_4_OFF

#define KEY_ALL_OFF KEY_C_OFF;KEY_Cis_OFF;KEY_D_OFF;KEY_Dis_OFF;KEY_E_OFF;KEY_F_OFF;KEY_Fis_OFF;KEY_G_OFF;KEY_Gis_OFF;KEY_A_OFF;KEY_Ais_OFF;KEY_H_OFF;KEY_C2_OFF;

#define BUTTON_SET_ON	(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0)==1)	//PWR button: PA0
#define BUTTON_U1_ON	(GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_2)==1)	//USer Button 1: PD2
#define BUTTON_U2_ON	(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_13)==0)	//USer Buttons 2-4: PC13..PC15
#define BUTTON_U3_ON	(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_14)==0)
#define BUTTON_U4_ON	(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_15)==0)

#define ANY_USER_BUTTON_ON (BUTTON_U1_ON || BUTTON_U2_ON || BUTTON_U3_ON || BUTTON_U4_ON || BUTTON_SET_ON)

#define USART3_RX	(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_11)==1)	//PB11

#ifdef __cplusplus
 extern "C" {
#endif

/* Exported variables ------------------------------------------------------- */
extern __IO uint32_t sys_timer;
extern __IO uint32_t sys_clock;

extern int run_program;

#define USART_COMMAND_BUFFER_LENGTH 2048 //TODO: implement overflow indication
extern int usart_command_length, usart_command_received;
extern char usart_command[USART_COMMAND_BUFFER_LENGTH];

/* Exported functions ------------------------------------------------------- */
void GPIO_Init_all(bool can_block_SWD_debug);
void GPIO_Init_USART1(int baud_rate);
void GPIO_Deinit_USART1();

void GPIO_Init_USART3RX_direct_input();
void GPIO_Init_USART3(int baud_rate, int interrupt_enabled);
#define USART_MIDI_BAUD_RATE 31250

void GPIO_LEDs_Buttons_Reset();

void BUTTONS_test();
void pwr_button_shutdown();
void mcu_and_codec_shutdown();
void stop_program();
int user_button_status(int button);
int get_button_pressed();
int get_button_combination();
void reset_button_pressed_timings();
void wait_till_all_buttons_released();
int any_button_held();
void check_for_reset();

void GPIO_init_keboard_as_inputs(bool can_block_SWD_debug);
void Stylus_test();

void Delay(__IO uint32_t nTime);

void USART_puts(USART_TypeDef* USARTx, volatile char *s);

#ifdef __cplusplus
}
#endif

#endif /* GPIO_H_ */
