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
*/



#include <stm32f10x.h>
#include <stdbool.h>
#include "usb.h"
#include "config.h"
#include "hid.h"
#include "bitwise.h"
#include "led.h"

// HID Bootloader takes 4 kb flash.
#define USER_PROGRAM 0x08001000
#define USER_CODE_FLASH0X8001000    ((u32)0x08001000)

typedef void (*funct_ptr)(void);
void delay(uint32_t tmr);

void blink_led(uint16_t times);
volatile uint32_t timeout = 0;
bool checkUserCode(u32 usrAddr);
bool check_flash_complete(void);
bool uploadStarted;
bool uploadFinished;
bool send_next_data = false;  

static uint16_t get_and_clear_magic_word(void) {
	bit_set(RCC->APB1ENR, RCC_APB1ENR_BKPEN | RCC_APB1ENR_PWREN); //Enable the power and backup interface clocks by setting the PWREN and BKPEN bitsin the RCC_APB1ENR register

	uint16_t value = BKP->DR10;
	if(value) {
		bit_set(PWR->CR, PWR_CR_DBP); //Enable access to the backup registers and the RTC.
		BKP->DR10 = 0x0000;
		bit_clear(PWR->CR, PWR_CR_DBP);
	}
	
	bit_clear(RCC->APB1ENR, RCC_APB1ENR_BKPEN | RCC_APB1ENR_PWREN);

	return value;
}

int main(int argc, char *argv[]) {

	(void) argc;
	(void) argv;

	pins_init();
  
  // Wait 1uS so the pull-up settles...
	delay(72);
  
#if defined HAS_LED2_PIN
	led2_off();
#endif
		
	uploadStarted = false;
	uploadFinished = false;
	uint32_t userProgramAddress = *(volatile uint32_t *)(USER_PROGRAM + 0x04);
	funct_ptr userProgram = (funct_ptr) userProgramAddress;

	// If PB2 (BOOT 1 pin) is HIGH enter HID bootloader or no User Code is uploaded to the MCU ...
	if((GPIOB->IDR & GPIO_IDR_IDR2)||(checkUserCode(USER_CODE_FLASH0X8001000) == false)) {

		USB_Init(HIDUSB_EPHandler, HIDUSB_Reset);
	
		while(check_flash_complete() == false){
			delay(400L);
		};
		
		USB_Shutdown(); 		//Reset USB
		NVIC_SystemReset();		//Reset STM32
		
		for(;;);
	}
	
 /**
	* If  <Battery Backed RAM Register> was set from Arduino IDE
	* exit from USB Serial mode and go to HID mode.
	*/
	if(get_and_clear_magic_word() == 0x424C) {
		
#if defined HAS_LED2_PIN
		led2_on();
#endif

		USB_Shutdown();
		delay(4000000L);
		USB_Init(HIDUSB_EPHandler, HIDUSB_Reset);
		//delay(400000000L);
		while(check_flash_complete() == false){
			delay(400L);
		};
		
		USB_Shutdown(); 			//Reset USB
		NVIC_SystemReset();		//Reset STM32
		
		for(;;);
	
	}
	
#if defined HAS_LED2_PIN
	led2_on();
#endif

	// Turn GPIOA clock off
	bit_clear(RCC->APB2ENR, RCC_APB2ENR_IOPAEN);

	// Turn GPIOB clock off
	LED1_CLOCK_DIS;
	//bit_clear(RCC->APB2ENR, RCC_APB2ENR_IOPBEN);
	SCB->VTOR = USER_PROGRAM;
	asm volatile("msr msp, %0"::"g"(*(volatile u32 *) USER_PROGRAM));
	userProgram();

	for(;;);
}

bool check_flash_complete(void){
	if(uploadFinished == true){
		return true;
	}
	
	if(uploadStarted == false){
#if defined HAS_LED1_PIN
		led_on();
		delay(200000L);
		led_off();
#endif
		delay(200000L);
	}
	return false;
}

bool checkUserCode(u32 usrAddr) {
	u32 sp = *(vu32 *) usrAddr;

	if ((sp & 0x2FFE0000) == 0x20000000) {
		return (true);
	} else {
			return (false);
	}
}

void blink_led(uint16_t times){
	for(int i=0;i<times;i++){
#if defined HAS_LED1_PIN
		led_on();
		delay(200000L);
		led_off();
#endif
		delay(200000L);
	}
}
	
void delay(uint32_t tmr){
	for(uint32_t i = 0; i < tmr; i++) {
		asm volatile ("nop\n");
	}
}
