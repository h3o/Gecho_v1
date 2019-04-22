/*
 * gpio.c
 *
 *  Created on: Jun 21, 2016
 *      Author: mayo
 */

#include <hw/codec.h>
#include <hw/gpio.h>
#include <hw/leds.h>
#include <string.h>

int BT_PWR_pressed = 0; //de-bouncing counter for power button
int usart_command_length = 0, usart_command_received = 0;
char usart_command[USART_COMMAND_BUFFER_LENGTH];

__IO uint32_t sys_timer = 0;
__IO uint32_t sys_clock = 0;

void GPIO_Init_all(bool can_block_SWD_debug)
{
	// struct to initialize GPIO pins
	GPIO_InitTypeDef GPIO_InitStructure;

	//enables GPIO clock for PortA, PortB, PortC and PortD
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);

	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL; //Without pull up & pull down resistors, as default

	//8x Red LEDs: PB0..PB7, if board v2 then 5th LED is PA7
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_SPEED_LEDS_BUTTONS;
#ifdef GECHO_V2
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
#else
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
#endif
	LED_R8_all_OFF();

#ifdef BOARD_GECHO_V001
	//4x Orange LEDs: PB10..PB13 + Red LED: PB14 + Yellow LED: PB15
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_SPEED_LEDS_BUTTONS;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
#endif

#ifdef BOARD_GECHO_V002
	//4x Orange LEDs: PB10..PB13 + Red LED: PB14
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_SPEED_LEDS_BUTTONS;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	//1x Signal LED: PA5
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
#endif

	LED_O4_all_OFF();
	LED_RDY_OFF;
	LED_SIG_OFF;

	//5x Blue LED: PC4, PC5, PC6, PC8, PC9
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_8 | GPIO_Pin_9;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	LED_B5_all_OFF();

	//8x White LEDs: PA8..PA15
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_SPEED_LEDS_BUTTONS;

	if(can_block_SWD_debug)
	{
		#ifdef BYPASS_W8_0
		GPIO_InitStructure.GPIO_Pin = /*GPIO_Pin_8 |*/ GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
		#else
			#ifdef CAN_USE_CH340G_LEDS
				GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
			#else
				GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | /*GPIO_Pin_9 | GPIO_Pin_10 |*/ GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
			#endif
		#endif
	}
	else
	{
		#ifdef BYPASS_W8_0
			GPIO_InitStructure.GPIO_Pin = /*GPIO_Pin_8 |*/ GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12 /*| GPIO_Pin_13 | GPIO_Pin_14*/ | GPIO_Pin_15;
		#else
			#ifdef CAN_USE_CH340G_LEDS
				GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12 /*| GPIO_Pin_13 | GPIO_Pin_14*/ | GPIO_Pin_15;
			#else
				GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | /*GPIO_Pin_9 | GPIO_Pin_10 |*/ GPIO_Pin_11 | GPIO_Pin_12 /*| GPIO_Pin_13 | GPIO_Pin_14*/ | GPIO_Pin_15;
			#endif
		#endif
		//rattling test #2: PA0 driven by alternating signal, causing similar rattle
		//GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12 /*| GPIO_Pin_13 | GPIO_Pin_14*/ | GPIO_Pin_15;
	}

	GPIO_Init(GPIOA, &GPIO_InitStructure);
	LED_W8_all_OFF();

#ifdef BOARD_GECHO_V001
	/*
	//still rattling when PA8 pin flipped
	#ifdef BYPASS_W8_0
	//1st White LED: PA8
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
	GPIO_InitStructure.GPIO_Speed = GPIO_SPEED_LEDS_BUTTONS;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	#endif
	*/

	//PA3 is IR LED Driver - STM32F4_DISC1
	//GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;

	//IR LED Driver PA1 - Gecho V0.01 - causing rattling sound
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_SPEED_LEDS_BUTTONS;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
#endif


#ifdef BOARD_GECHO_V002
	//IR LED Drivers PB14,PB15
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14 | GPIO_Pin_15;

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_SPEED_LEDS_BUTTONS;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
#endif


	//----------------- BUTTONS ---------------------------------------------------

	//PWR button: PA0
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN; //Input mode
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //Output type push-pull
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL; //Without pull down resistor, there is one on board
	GPIO_InitStructure.GPIO_Speed = GPIO_SPEED_LEDS_BUTTONS;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	//USer Button 1: PD2
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN; //Input mode
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //Output type push-pull
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN; //With pull down resistor
	GPIO_InitStructure.GPIO_Speed = GPIO_SPEED_LEDS_BUTTONS;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	//USer Buttons 2-4: PC13..PC15
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN; //Input mode
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //Output type push-pull
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //With pull down resistor
	GPIO_InitStructure.GPIO_Speed = GPIO_SPEED_LEDS_BUTTONS;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	/*
	//IR LED Driver alternative test PD2 (wired to But2 resistor)
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_SPEED_LEDS_BUTTONS;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
	*/

	/*
	//Rattling test #3 via PB14
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_SPEED_LEDS_BUTTONS;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	*/

	/*
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15 | GPIO_Pin_14 | GPIO_Pin_13 | GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_SPEED_LEDS_BUTTONS;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	//PA0 is blue user button (but later may be overriden for External Analog Input)
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	//Input mode
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	//Output type push-pull
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	//With pull down resistor
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
	//50MHz pin speed
	GPIO_InitStructure.GPIO_Speed = GPIO_SPEED_LEDS_BUTTONS;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	//on-shield Blue and Red LEDs: PA15,PC11,PA9,PA8,PC6,PA10,PC8,PC9...
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_SPEED_LEDS_BUTTONS;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15 | GPIO_Pin_10 | GPIO_Pin_9 | GPIO_Pin_8;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_9 | GPIO_Pin_8 | GPIO_Pin_6;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	LED_SB1_OFF;
	LED_SB2_OFF;
	LED_SB3_OFF;
	LED_SB4_OFF;
	LED_SR1_OFF;
	LED_SR2_OFF;
	LED_SR3_OFF;
	LED_SR4_OFF;
	*/
}

void GPIO_Init_USART3RX_direct_input()
{
	// struct to initialize GPIO pins
	GPIO_InitTypeDef GPIO_InitStructure;

	//direct init as input pin for quick test

	//2nd Orange LED: PB11
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	//GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_SPEED_LEDS_BUTTONS;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
}

void GPIO_Init_USART3(int baud_rate, int interrupt_enabled)
{
	//proper init as alternate function pin

	// Enable clock for GPIOB
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	// Enable clock for USART3
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);

	// Connect PB10 to USART3_Tx
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource10, GPIO_AF_USART3);
	// Connect PB11 to USART3_Rx
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource11, GPIO_AF_USART3);

	// Initialization of GPIOB
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStruct.GPIO_Speed = GPIO_SPEED_USART;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOB, &GPIO_InitStruct);

	// Initialization of USART3
	USART_InitTypeDef USART_InitStruct;
	USART_InitStruct.USART_BaudRate = baud_rate;//31250;
	USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStruct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_InitStruct.USART_Parity = USART_Parity_No;
	USART_InitStruct.USART_StopBits = USART_StopBits_1;
	USART_InitStruct.USART_WordLength = USART_WordLength_8b;
	USART_Init(USART3, &USART_InitStruct);

	if(interrupt_enabled)
	{
		// Enable the USART3 receive interrupt and configure the interrupt controller
		// to jump to the USART3_IRQHandler() function if the USART3 receive interrupt occurs
		USART_ITConfig(USART3, USART_IT_RXNE, ENABLE); // enable the USART3 receive interrupt

		// Configure the NVIC (nested vector interrupt controller)
		NVIC_InitTypeDef NVIC_InitStructure;
		NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;		 // we want to configure the USART3 interrupts
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;// this sets the priority group of the USART3 interrupts
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;		 // this sets the subpriority inside the group
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			 // the USART3 interrupts are globally enabled
		NVIC_Init(&NVIC_InitStructure);							 // the properties are passed to the NVIC_Init function which takes care of the low level stuff
	}

	// Enable the USART3 peripheral
	USART_Cmd(USART3, ENABLE);
}

void GPIO_Init_USART1(int baud_rate)
{
	//proper init as alternate function pin

	// Enable clock for GPIOA
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	// Enable clock for USART1 and AFIO
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

	// Connect PA9 to USART1_Tx
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_USART1);
	// Connect PA10 to USART1_Rx
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_USART1);

	// Initialization of GPIOA
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStruct.GPIO_Speed = GPIO_SPEED_USART;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP; //GPIO_OType_OD;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP; //GPIO_PuPd_NOPULL; //GPIO_PuPd_UP;
	GPIO_Init(GPIOA, &GPIO_InitStruct);

	// Initialization of USART1
	USART_InitTypeDef USART_InitStruct;
	USART_InitStruct.USART_BaudRate = baud_rate;//31250; //9600;//115200;
	USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStruct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_InitStruct.USART_Parity = USART_Parity_No;
	USART_InitStruct.USART_StopBits = USART_StopBits_1;
	USART_InitStruct.USART_WordLength = USART_WordLength_8b;
	USART_Init(USART1, &USART_InitStruct);

	// Enable the USART1 receive interrupt and configure the interrupt controller
	// to jump to the USART1_IRQHandler() function if the USART1 receive interrupt occurs
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE); // enable the USART1 receive interrupt

	// Configure the NVIC (nested vector interrupt controller)
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;		 // we want to configure the USART1 interrupts
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;// this sets the priority group of the USART1 interrupts
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;		 // this sets the subpriority inside the group
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			 // the USART1 interrupts are globally enabled
	NVIC_Init(&NVIC_InitStructure);							 // the properties are passed to the NVIC_Init function which takes care of the low level stuff

	// Enable the USART1 peripheral
	USART_Cmd(USART1, ENABLE);
}


// Source: https://github.com/g4lvanix/STM32F4-examples/blob/master/USART/main.c

/* This function is used to transmit a string of characters via
 * the USART specified in USARTx.
 *
 * It takes two arguments: USARTx --> can be any of the USARTs e.g. USART1, USART2 etc.
 * 						   (volatile) char *s is the string you want to send
 *
 * Note: The string has to be passed to the function as a pointer because
 * 		 the compiler doesn't know the 'string' data type. In standard
 * 		 C a string is just an array of characters
 *
 * Note 2: At the moment it takes a volatile char because the received_string variable
 * 		   declared as volatile char --> otherwise the compiler will spit out warnings
 * */
void USART_puts(USART_TypeDef* USARTx, volatile char *s){

	while(*s){
		// wait until data register is empty
		while( !(USARTx->SR & 0x00000040) );
		USART_SendData(USARTx, *s);
		*s++;
	}
}

//#define MAX_STRLEN 12 // this is the maximum string length of our string in characters
//volatile char received_string[MAX_STRLEN+1]; // this will hold the recieved string

// This is the interrupt request handler (IRQ) for ALL USART1 interrupts
void USART1_IRQHandler(void){

	// check if the USART1 receive interrupt flag was set
	if(USART_GetITStatus(USART1, USART_IT_RXNE))
	{
		char c = USART1->DR; // the character from the USART1 data register is saved in c

		if(!usart_command_received) //do nothing if previous command not parsed yet
		{
			//check if the received character is not the LF character (used to determine end of string)
			//or the if the maximum string length has been been reached
			if( (c != '\n') && (usart_command_length < USART_COMMAND_BUFFER_LENGTH) )
			{
				usart_command[usart_command_length] = c;
				usart_command_length++;
			}
			else
			{
				//unless buffer is overflown, set the received flag, otherwise we will the command as it is most likely incomplete
				if(usart_command_length < USART_COMMAND_BUFFER_LENGTH)
				{
					usart_command_received = 1;
				}

				usart_command[usart_command_length] = 0; //terminate the string
				usart_command_length = 0; //reset the pointer so it is prepared for the next command
			}
		}
		/*
		else
		{
			usart_command_received++;
		}

		if(usart_command_length >= USART_COMMAND_BUFFER_LENGTH)
		{
			usart_command_received++;
		}
		*/
	}
}

void GPIO_Deinit_USART1()
{
	USART_Cmd(USART1, DISABLE); // Disable the USART1 peripheral
	USART_ITConfig(USART1, USART_IT_RXNE, DISABLE); // Disable the USART1 receive interrupt
	USART_DeInit(USART1); // Deinitialize the USART peripheral registers to their default reset values

	// Revert GPIO pins back to normal mode
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStruct.GPIO_Speed = GPIO_SPEED_LEDS_BUTTONS;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP; //GPIO_OType_OD;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP; //GPIO_PuPd_NOPULL; //GPIO_PuPd_UP;
	GPIO_Init(GPIOA, &GPIO_InitStruct);
}

void GPIO_LEDs_Buttons_Reset()
{
	BT_PWR_pressed = 0; //restart counter
	LED_R8_all_OFF();
	LED_O4_all_OFF();
	LED_B5_all_OFF();
	LED_W8_all_OFF();
}

/**
  * @brief  Inserts a delay time.
  * @param  nTime: specifies the delay time length, in 10 ms.
  * @retval None
  */
void Delay(__IO uint32_t nTime)
{
  //TimingDelay = nTime;
	sys_timer = nTime;

  //while(TimingDelay != 0);
  while(sys_timer != 0);
}

void BUTTONS_test()
{
	//Returs pin state (1 if HIGH, 0 if LOW)
	if(BUTTON_U1_ON)
	{
		LED_B5_0_ON;
	}
	if(BUTTON_U2_ON)
	{
		LED_B5_1_ON;
	}
	if(BUTTON_U3_ON)
	{
		LED_B5_2_ON;
	}
	if(BUTTON_U4_ON)
	{
		LED_B5_3_ON;
	}
#ifndef IGNORE_PA0_IF_PULLDOWN_NOT_ASSEMBLED
	if(BUTTON_SET_ON)
	{
		BT_PWR_pressed++;
		if(BT_PWR_pressed==I2S_AUDIOFREQ) //1 second
		{
			stop_program();
		}
		if(BT_PWR_pressed==I2S_AUDIOFREQ*3) //3 seconds
		{
			mcu_and_codec_shutdown();
		}
	}
#endif
	else
	{
		BT_PWR_pressed = 0;
	}
}

void mcu_and_codec_shutdown()
{
	codec_reset();
	PWR_WakeUpPinCmd(ENABLE); //Enable WKUP pin
	PWR_EnterSTANDBYMode();	//Enter Stand-by mode
}

void stop_program()
{
	codec_reset();
	run_program = 0;
}

void pwr_button_shutdown()
{
	if(BUTTON_SET_ON)
	{
		BT_PWR_pressed++;
		if(BT_PWR_pressed==I2S_AUDIOFREQ*3) //3 seconds
		{
			mcu_and_codec_shutdown();
		}
	}
	else
	{
		BT_PWR_pressed = 0;
	}
}

void GPIO_init_keboard_as_inputs(bool can_block_SWD_debug)
{
	// struct to initialize GPIO pins
	GPIO_InitTypeDef GPIO_InitStructure;

	LED_W8_all_OFF();
	LED_B5_all_OFF();

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //Output type push-pull
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	//GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_InitStructure.GPIO_Speed = GPIO_SPEED_LEDS_BUTTONS;

	//5x Blue LED: PC4, PC5, PC6, PC8, PC9
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_8 | GPIO_Pin_9;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	if(can_block_SWD_debug)
	{
		//8x White LEDs: PA8..PA15
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
		GPIO_Init(GPIOA, &GPIO_InitStructure);
	}
	else
	{
		//6x White LEDs: PA8..PA15 except PA13 and PA14
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12 | /*GPIO_Pin_13 | GPIO_Pin_14 |*/ GPIO_Pin_15;
		GPIO_Init(GPIOA, &GPIO_InitStructure);
	}
}

int user_button_status(int button)
{
	if(button==0)
	{
		return BUTTON_U1_ON;
	}
	if(button==1)
	{
		return BUTTON_U2_ON;
	}
	if(button==2)
	{
		return BUTTON_U3_ON;
	}
	if(button==3)
	{
		return BUTTON_U4_ON;
	}
	if(button==4)
	{
		return BUTTON_SET_ON;
	}
	return 0;
}

int button_pressed_timing[5] = {0,0,0,0,0};
#define BUTTON_PRESSED_THRESHOLD 100
int get_button_pressed()
{
	for(int i=0;i<5;i++)
	{
		if(user_button_status(i))
		{
			if(button_pressed_timing[i] > -1)
			{
				button_pressed_timing[i]++;
				if(button_pressed_timing[i] == BUTTON_PRESSED_THRESHOLD)
				{
					button_pressed_timing[i] = -1; //blocked until released
					return i+1; //return button number (1-4)
				}
			}
		}
		else
		{
			button_pressed_timing[i] = 0;
		}
	}
	return 0;
}

int button_combination_already_returned = 0;

int get_button_combination()
{
	int i,j;

	for(i=0;i<5;i++)
	{
		if(button_pressed_timing[i] == -1) //already held
		{
			//check other buttons for combo
			for(j=0;j<5;j++)
			{
				if(i!=j)
				{
					if(user_button_status(j)) //other button pressed simultaneously
					{
						//button_pressed_timing[i] = 0;
						button_pressed_timing[j] = 0;
						button_combination_already_returned = 1;
						return 10*(i+1) + j+1;
					}
				}
			}

			//check if button just released
			{
				if(!user_button_status(i)) //not pressed anymore
				{
					button_pressed_timing[i] = 0;
					if(button_combination_already_returned)
					{
						button_combination_already_returned = 0;
						return 0;
					}

					return i+1; //return button number (1-4)
				}
			}
		}
		else if(user_button_status(i))
		{
			if(button_pressed_timing[i] > -1)
			{
				button_pressed_timing[i]++;
				if(button_pressed_timing[i] == BUTTON_PRESSED_THRESHOLD)
				{
					button_pressed_timing[i] = -1; //blocked until released
					return 0;
				}
			}
		}
		else //???
		{
			button_pressed_timing[i] = 0;
		}
	}
	return 0;
}

void reset_button_pressed_timings()
{
	for(int i=0;i<5;i++)
	{
		button_pressed_timing[i] = 0;
	}
}

void wait_till_all_buttons_released()
{
	int wait = 1;
	while(wait)
	{
		wait = 0;
		for(int i=0;i<5;i++)
		{
			if(user_button_status(i))
			{
				wait = 1;
			}
		}
	}
	reset_button_pressed_timings();
}

int any_button_held()
{
	for(int i=0;i<5;i++)
	{
		if(user_button_status(i))
		{
			return 1;
		}
	}
	return 0;
}

void check_for_reset()
{
	if(usart_command_received)
	{
		if(0==strncmp(usart_command,"EXIT",4))
		{
			run_program = 0;
		}

		else if(0==strncmp(usart_command,"BTN=",4))
		{
			if(0==strncmp(usart_command+4,"RST",3))
			{
				//mcu_and_codec_shutdown();
				NVIC_SystemReset();
			}
		}
		usart_command_received = 0; //clear flag if any other command received, so it won't block the MCU
	}
}
