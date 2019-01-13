/*
* STM32 HID Bootloader - USB HID bootloader for STM32F10X
* Copyright (c) 2018 Bruno Freitas - bruno@brunofreitas.com
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
* Modified 20 April 2018
*	by Vassilis Serasidis <info@serasidis.gr>
*	This HID bootloader works with STM32F103 + STM32duino + Arduino IDE <http://www.stm32duino.com/>
*
* Modified January 2019
*	by Michel Stempin <michel.stempin@wanadoo.fr>
*	Cleanup and optimizations
*
*/

#include <stm32f10x.h>
#include <stdbool.h>
#include "usb.h"
#include "config.h"
#include "hid.h"
#include "led.h"

/* Bootloader size */
#define BOOTLOADER_SIZE			(4 * 1024)

/* SRAM size */
#define SRAM_SIZE			(20 * 1024)

/* HID Bootloader takes 4 kb flash. */
#define USER_PROGRAM			(FLASH_BASE + BOOTLOADER_SIZE)

typedef void (*funct_ptr)(void);

extern void Reset_Handler(void);

void _init(void);
void __libc_init_array(void);

uint32_t RamVectors[37] = {1};

static void delay(uint32_t tmr) {
	for (uint32_t i = 0; i < tmr; i++) {
		__NOP();
	}
}

static bool check_flash_complete(void) {
	if (UploadFinished == true) {
		return true;
	}
	if (UploadStarted == false) {

#if defined HAS_LED1_PIN
		led_on();
		delay(200000L);
		led_off();
#endif

		delay(200000L);
	}
	return false;
}

static bool check_user_code(u32 usrAddr) {
	u32 sp = *(vu32 *) usrAddr;

	/* Check if the stack pointer in the vector table points in
	   RAM */
	if ((sp & 0x2FFE0000) == SRAM_BASE) {
		return true;
	} else {
		return false;
	}
}

static uint16_t get_and_clear_magic_word(void) {

	/* Enable the power and backup interface clocks by setting the
	 * PWREN and BKPEN bitsin the RCC_APB1ENR register
	 */
	SET_BIT(RCC->APB1ENR, RCC_APB1ENR_BKPEN | RCC_APB1ENR_PWREN);
	uint16_t value = BKP->DR10;
	if (value) {

		/* Enable access to the backup registers and the RTC. */
		SET_BIT(PWR->CR, PWR_CR_DBP);
		BKP->DR10 = 0x0000;
		CLEAR_BIT(PWR->CR, PWR_CR_DBP);
	}
	CLEAR_BIT(RCC->APB1ENR, RCC_APB1ENR_BKPEN | RCC_APB1ENR_PWREN);
	return value;
}

void _init(void) {
}

void __libc_init_array(void) {
}

int main(int argc, char *argv[]) {
	(void) argc;
	(void) argv;

	RamVectors[0] = SRAM_BASE + SRAM_SIZE;
	RamVectors[1] = (uint32_t) Reset_Handler;
	RamVectors[36] = (uint32_t) USB_LP_CAN1_RX0_IRQHandler;
	SCB->VTOR = (volatile uint32_t) RamVectors;

	uint16_t magic_word = get_and_clear_magic_word();

	pins_init();

	/* Wait 1uS so the pull-up settles... */
	delay(72);

#if defined HAS_LED2_PIN
	led2_off();
#endif
		
	UploadStarted = false;
	UploadFinished = false;
	uint32_t userProgramAddress = *(volatile uint32_t *) (USER_PROGRAM + 0x04);
	funct_ptr userProgram = (funct_ptr) userProgramAddress;

	/* If PB2 (BOOT 1 pin) is HIGH enter HID bootloader or no User
	 * Code is uploaded to the MCU or <Battery Backed RAM
	 * Register> was set from Arduino IDE exit from USB Serial
	 * mode and go to HID mode ...
	 */
	if ((magic_word == 0x424C) ||
		(GPIOB->IDR & GPIO_IDR_IDR2) ||
		(check_user_code(USER_PROGRAM) == false)) {
		if (magic_word == 0x424C) {
		
#if defined HAS_LED2_PIN
			led2_on();
#endif

			USB_Shutdown();
			delay(4000000L);
		}
		USB_Init(HIDUSB_EPHandler, HIDUSB_Reset);
		while (check_flash_complete() == false) {
			delay(400L);
		};

 		/* Reset USB */
		USB_Shutdown();

		/* Reset STM32 */
		NVIC_SystemReset();
		for (;;) {
			;
		}
	}
	
#if defined HAS_LED2_PIN
	led2_on();
#endif

	/* Turn GPIOA clock off */
	CLEAR_BIT(RCC->APB2ENR, RCC_APB2ENR_IOPAEN);

	/* Turn GPIOB clock off */
	LED1_CLOCK_DIS;
	//CLEAR_BIT(RCC->APB2ENR, RCC_APB2ENR_IOPBEN);
	SCB->VTOR = USER_PROGRAM;
	__set_MSP((*(volatile uint32_t *) USER_PROGRAM));
	userProgram();
	for (;;) {
		;
	}
}
