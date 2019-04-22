/*
 * system_fn.c
 *
 *  Created on: Oct 31, 2016
 *      Author: mayo
 *
 *  This file is part of the Gecho Loopsynth Firmware Development Framework.
 *  It can be used within the terms of CC-BY-NC-SA license.
 *  It must not be distributed separately.
 *
 *  Find more information at: http://gechologic.com/gechologists/
 */

#include "system_fn.h"

void FlashAcceleratorInit()
{
	//Disable prefetch buffer
	//FLASH->ACR &= ~FLASH_ACR_PRFTEN;
	//Enable prefetch buffer
	FLASH->ACR |= FLASH_ACR_PRFTEN;

	//Enable flash instruction cache
	FLASH->ACR |= FLASH_ACR_ICEN;
	//Disable flash instruction cache
	//FLASH->ACR &= ~FLASH_ACR_ICEN;

	//Enable flash data cache
	FLASH->ACR |= FLASH_ACR_DCEN;
	//Disable flash data cache
	//FLASH->ACR &= ~FLASH_ACR_DCEN;
}
