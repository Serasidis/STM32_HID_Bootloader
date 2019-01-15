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

/* SRAM end (bottom of stack) */
#define SRAM_END			(SRAM_BASE + SRAM_SIZE)

/* HID Bootloader takes 4 kb flash. */
#define USER_PROGRAM			(FLASH_BASE + BOOTLOADER_SIZE)

/* Simple function pointer type to call user program */
typedef void (*funct_ptr)(void);

/* The bootloader entry point gunction prototype */
void Reset_Handler(void);

/* The SRAM vector table The initializer is to put this array in the
 * .data section at the begining of SRAM
 */
uint32_t RamVectors[37]  __attribute__((section(".data")));

/* Minimal initial Flash-based vector table */
uint32_t *VectorTable[] __attribute__((section(".isr_vector"))) = {

	/* Initial stack pointer (MSP) */
	(uint32_t *) SRAM_END,

	/* Reset handler */
	(uint32_t *) Reset_Handler
};

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

		LED1_ON;
		delay(200000L);
		LED1_OFF;

		delay(200000L);
	}
	return false;
}

static bool check_user_code(uint32_t usrAddr) {
	uint32_t sp = *(volatile uint32_t *) usrAddr;

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
	 * PWREN and BKPEN bits in the RCC_APB1ENR register
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

static void SetSysClockTo72(void)
{

	/* Enable HSE */
	SET_BIT(RCC->CR, RCC_CR_HSEON);

	/* Wait until HSE is ready */
	//while ((RCC->CR & RCC_CR_HSERDY) == 0) {
	while (READ_BIT(RCC->CR, RCC_CR_HSERDY) == 0) {
		;
	}

	/* Enable Prefetch Buffer & set Flash access to 2 wait states */
	SET_BIT(FLASH->ACR, FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY_2);

	/* SYSCLK = PCLK2 = HCLK */
	/* PCLK1 = HCLK / 2 */
	/* PLLCLK = HSE * 9 = 72 MHz */
	SET_BIT(RCC->CFGR, RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE2_DIV1 | RCC_CFGR_PPRE1_DIV2 |
		RCC_CFGR_PLLSRC_HSE | RCC_CFGR_PLLMULL9);

	/* Enable PLL */
	SET_BIT(RCC->CR, RCC_CR_PLLON);

	/* Wait until PLL is ready */
	//while ((RCC->CR & RCC_CR_PLLRDY) == 0) {
	while (READ_BIT(RCC->CR, RCC_CR_PLLRDY) == 0) {
		;
	}

	/* Select PLL as system clock source */
	SET_BIT(RCC->CFGR, RCC_CFGR_SW_PLL);

	/* Wait until PLL is used as system clock source */
	while (READ_BIT(RCC->CFGR, RCC_CFGR_SWS_1) == 0) {
		;
	}
}

void Reset_Handler(void) {

	/* Setup the system clock (System clock source, PLL Multiplier
	 * factors, AHB/APBx prescalers and Flash settings)
	 */
	SetSysClockTo72();

	/* Setup to vector table in SRAM, so we can handle USB IRQs */
	RamVectors[0] = SRAM_END;
	RamVectors[1] = (uint32_t) Reset_Handler;
	RamVectors[36] = (uint32_t) USB_LP_CAN1_RX0_IRQHandler;
	SCB->VTOR = (volatile uint32_t) RamVectors;

	/* Check for a magic word in BACKUP memory */
	uint16_t magic_word = get_and_clear_magic_word();

	/* Initialize GPIOs */
	pins_init();

	/* Wait 1uS so the pull-up settles... */
	delay(72);
	LED2_OFF;

	UploadStarted = false;
	UploadFinished = false;
	funct_ptr UserProgram = (funct_ptr) *(volatile uint32_t *) (USER_PROGRAM + 0x04);

	/* If:
	 *  - PB2 (BOOT 1 pin) is HIGH or
	 *  - no User Code is uploaded to the MCU or
	 *  - a magic word was stored in the battery-backed RAM
	 *    registers from the Arduino IDE
	 * then enter HID bootloader...
	 */
	if ((magic_word == 0x424C) ||
		(GPIOB->IDR & GPIO_IDR_IDR2) ||
		(check_user_code(USER_PROGRAM) == false)) {
		if (magic_word == 0x424C) {

			/* If a magic word was stored in the
			 * battery-backed RAM registers from the
			 * Arduino IDE, exit from USB Serial mode and
			 * go to HID mode...
			 */
			LED2_ON;
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
	LED2_ON;

	/* Turn GPIOA clock off */
	CLEAR_BIT(RCC->APB2ENR, RCC_APB2ENR_IOPAEN);

	/* Turn GPIOB clock off */
	LED1_CLOCK_DIS;
	//CLEAR_BIT(RCC->APB2ENR, RCC_APB2ENR_IOPBEN);
	SCB->VTOR = USER_PROGRAM;
	__set_MSP((*(volatile uint32_t *) USER_PROGRAM));
	UserProgram();
	for (;;) {
		;
	}
}
