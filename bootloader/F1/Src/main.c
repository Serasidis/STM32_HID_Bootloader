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
#define BOOTLOADER_SIZE			(2 * 1024)

/* SRAM size */
#define SRAM_SIZE			(20 * 1024)

/* SRAM end (bottom of stack) */
#define SRAM_END			(SRAM_BASE + SRAM_SIZE)

/* HID Bootloader takes 2 kb flash. */
#define USER_PROGRAM			(FLASH_BASE + BOOTLOADER_SIZE)

/* Initial stack pointer index in vector table*/
#define INITIAL_MSP			0

/* Reset handler index in vector table*/
#define RESET_HANDLER			1

/* USB Low-Priority and CAN1 RX0 IRQ handler idnex in vector table */
#define USB_LP_CAN1_RX0_IRQ_HANDLER	36

/* Simple function pointer type to call user program */
typedef void (*funct_ptr)(void);

/* The bootloader entry point function prototype */
void Reset_Handler(void);

/* Minimal initial Flash-based vector table */
uint32_t *VectorTable[] __attribute__((section(".isr_vector"))) = {

	/* Initial stack pointer (MSP) */
	(uint32_t *) SRAM_END,

	/* Initial program counter (PC): Reset handler */
	(uint32_t *) Reset_Handler
};

static void delay(uint32_t timeout)
{
	for (uint32_t i = 0; i < timeout; i++) {
		__NOP();
	}
}

static bool check_flash_complete(void)
{
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

static bool check_user_code(uint32_t user_address)
{
	uint32_t sp = *(volatile uint32_t *) user_address;

	/* Check if the stack pointer in the vector table points
	   somewhere in SRAM */
	return ((sp & 0x2FFE0000) == SRAM_BASE) ? true : false;
}

static uint16_t get_and_clear_magic_word(void)
{

	/* Enable the power and backup interface clocks by setting the
	 * PWREN and BKPEN bits in the RCC_APB1ENR register
	 */
	SET_BIT(RCC->APB1ENR, RCC_APB1ENR_BKPEN | RCC_APB1ENR_PWREN);
	uint16_t value = READ_REG(BKP->DR4);
	if (value) {

		/* Enable write access to the backup registers and the
		 * RTC.
		 */
		SET_BIT(PWR->CR, PWR_CR_DBP);
		WRITE_REG(BKP->DR4, 0x0000);
		CLEAR_BIT(PWR->CR, PWR_CR_DBP);
	}
	CLEAR_BIT(RCC->APB1ENR, RCC_APB1ENR_BKPEN | RCC_APB1ENR_PWREN);
	return value;
}

static void set_sysclock_to_72_mhz(void)
{

	/* Enable HSE */
	SET_BIT(RCC->CR, RCC_CR_HSEON);

	/* Wait until HSE is ready */
	while (READ_BIT(RCC->CR, RCC_CR_HSERDY) == 0) {
		;
	}

	/* Enable Prefetch Buffer & set Flash access to 2 wait states */
	SET_BIT(FLASH->ACR, FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY_2);

	/* SYSCLK = PCLK2 = HCLK */
	/* PCLK1 = HCLK / 2 */
	/* PLLCLK = HSE * 9 = 72 MHz */
	SET_BIT(RCC->CFGR,
		RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE2_DIV1 | RCC_CFGR_PPRE1_DIV2 |
		RCC_CFGR_PLLSRC_HSE | RCC_CFGR_PLLMULL9);

	/* Enable PLL */
	SET_BIT(RCC->CR, RCC_CR_PLLON);

	/* Wait until PLL is ready */
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

void Reset_Handler(void)
{
	volatile uint32_t *const ram_vectors =
		(volatile uint32_t *const) SRAM_BASE;

	/* Setup the system clock (System clock source, PLL Multiplier
	 * factors, AHB/APBx prescalers and Flash settings)
	 */
	set_sysclock_to_72_mhz();

	/* Setup a temporary vector table into SRAM, so we can handle
	 * USB IRQs
	 */
	ram_vectors[INITIAL_MSP] = SRAM_END;
	ram_vectors[RESET_HANDLER] = (uint32_t) Reset_Handler;
	ram_vectors[USB_LP_CAN1_RX0_IRQ_HANDLER] =
		(uint32_t) USB_LP_CAN1_RX0_IRQHandler;
	WRITE_REG(SCB->VTOR, (volatile uint32_t) ram_vectors);

	/* Check for a magic word in BACKUP memory */
	uint16_t magic_word = get_and_clear_magic_word();

	/* Initialize GPIOs */
	pins_init();

	/* Wait 1us so the pull-up settles... */
	delay(72);
	LED2_OFF;

	UploadStarted = false;
	UploadFinished = false;
	funct_ptr UserProgram =
		(funct_ptr) *(volatile uint32_t *) (USER_PROGRAM + 0x04);

	/* If:
	 *  - PB2 (BOOT 1 pin) is HIGH or
	 *  - no User Code is uploaded to the MCU or
	 *  - a magic word was stored in the battery-backed RAM
	 *    registers from the Arduino IDE
	 * then enter HID bootloader...
	 */
	if ((magic_word == 0x424C) ||
		READ_BIT(GPIOB->IDR, GPIO_IDR_IDR2) ||
		(check_user_code(USER_PROGRAM) == false)) {
		/* Reinitialize USB (never trust the previous state) */
		LED2_ON;
		USB_Shutdown();
		delay(4000000L);
		USB_Init();
		while (check_flash_complete() == false) {
			delay(400L);
		};

		/* Reset the USB */
		USB_Shutdown();

		/* Reset the STM32 */
		NVIC_SystemReset();

		/* Never reached */
		for (;;) {
			;
		}
	}
	LED2_ON;

	/* Turn GPIO clocks off */
	CLEAR_BIT(RCC->APB2ENR,
		LED1_CLOCK | LED2_CLOCK | DISC_CLOCK/* | RCC_APB2ENR_IOPBEN*/);

	/* Setup the vector table to the final user-defined one in Flash
	 * memory
	 */
	WRITE_REG(SCB->VTOR, USER_PROGRAM);

	/* Setup the stack pointer to the user-defined one */
	__set_MSP((*(volatile uint32_t *) USER_PROGRAM));

	/* Jump to the user firmware entry point */
	UserProgram();

	/* Never reached */
	for (;;) {
		;
	}
}
