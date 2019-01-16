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
* Modified January 2019
*	by Michel Stempin <michel.stempin@wanadoo.fr>
*	Cleanup and optimizations
*
*/

#include <stm32f10x.h>
#include <stdlib.h>
#include <stdbool.h>

#include "usb.h"
#include "hid.h"

#define CNTR_MASK	(CNTR_RESETM | CNTR_SUSPM | CNTR_WKUPM)
#define ISTR_MASK	(ISTR_CTR | ISTR_RESET | ISTR_SUSP | ISTR_WKUP)

USB_RxTxBuf_t RxTxBuffer[MAX_EP_NUM];

volatile uint8_t DeviceAddress = 0;
volatile uint16_t DeviceConfigured = 0;
const uint16_t DeviceStatus = 0;

void USB_PMA2Buffer(uint8_t EPn) {
	volatile uint32_t *BTableAddress = BTABLE_ADDR(EPn);
	uint32_t Count = RxTxBuffer[EPn].RXL = BTableAddress[USB_COUNTn_RX] & 0x3ff;
	uint32_t *Address = (uint32_t *) (PMAAddr + BTableAddress[USB_ADDRn_RX] * 2);
	uint16_t *Destination = (uint16_t *) RxTxBuffer[EPn].RXB;

	for (uint32_t i = 0; i < Count; i++) {
		*Destination++ = *Address++;
	}
}

void USB_Buffer2PMA(uint8_t EPn) {
	volatile uint32_t *BTableAddress = BTABLE_ADDR(EPn);
	uint32_t Count = RxTxBuffer[EPn].TXL <= RxTxBuffer[EPn].MaxPacketSize ?
		RxTxBuffer[EPn].TXL : RxTxBuffer[EPn].MaxPacketSize;
	uint16_t *Address = RxTxBuffer[EPn].TXB;
	uint32_t *Destination = (uint32_t *) (PMAAddr + BTableAddress[USB_ADDRn_TX] * 2);

	/* Set transmission byte count in buffer descriptor table */
	BTableAddress[USB_COUNTn_TX] = Count;
	for (uint32_t i = (Count + 1) / 2; i; i--) {
		*Destination++ = *Address++;
	}
	RxTxBuffer[EPn].TXL -= Count;
	RxTxBuffer[EPn].TXB = Address;
}

void USB_SendData(uint8_t EPn, uint16_t *Data, uint16_t Length) {
	if (EPn > 0 && !DeviceConfigured) {
		return;
	}
	RxTxBuffer[EPn].TXL = Length;
	RxTxBuffer[EPn].TXB = Data;
	USB_Buffer2PMA(EPn);
	TOGGLE_REG(EP0REG[EPn],
		   EP_DTOG_RX | EPRX_STAT | EP_DTOG_TX,
		   0,
		   EP_TX_VALID);
}

void USB_Shutdown(void) {

	/* Disable USB IRQ */
	NVIC_DisableIRQ(USB_LP_CAN1_RX0_IRQn);
	_SetISTR(0);
	DeviceConfigured = 0;

	/* Turn USB Macrocell off */
	_SetCNTR(CNTR_FRES|CNTR_PDWN);

	/* PA12: General purpose output 50 MHz open drain */
	SET_BIT(RCC->APB2ENR, RCC_APB2ENR_IOPAEN);
	MODIFY_REG(GPIOA->CRH,
		GPIO_CRH_CNF12 | GPIO_CRH_MODE12,
		GPIO_CRH_CNF12_0 | GPIO_CRH_MODE12);

	/* Sinks PA12 to GND */
	GPIOA->BRR = GPIO_BRR_BR12;

	/* Disable USB Clock on APB1 */
	CLEAR_BIT(RCC->APB1ENR, RCC_APB1ENR_USBEN);
}

void USB_Init(void) {

	/* Reset RX and TX lengths inside RxTxBuffer struct for all
	 * endpoints
	 */
	for (int i = 0; i < MAX_EP_NUM; i++) {
		RxTxBuffer[i].RXL = RxTxBuffer[i].TXL = 0;
	}

	/* PA12: General purpose Input Float */
	SET_BIT(RCC->APB2ENR, RCC_APB2ENR_IOPAEN);
	MODIFY_REG(GPIOA->CRH,
		GPIO_CRH_CNF12 | GPIO_CRH_MODE12,
		GPIO_CRH_CNF12_0);
	DeviceConfigured = 0;
	SET_BIT(RCC->APB1ENR, RCC_APB1ENR_USBEN);

	/* Enable USB IRQ in core */
	NVIC_EnableIRQ(USB_LP_CAN1_RX0_IRQn);

	/* CNTR_FRES = 1, CNTR_PWDN = 0 */
	_SetCNTR(CNTR_FRES);

	/* The following sequence is recommended:
	 * 1- FRES = 0
	 * 2- Wait until RESET flag = 1 (polling)
	 * 3- clear ISTR register
	 */

	/* CNTR_FRES = 0 */
	_SetCNTR(0);

	/* Wait until RESET flag = 1 (polling) */
	while (!READ_BIT(_GetISTR(), ISTR_RESET)) {
		;
	}

	/* Clear pending interrupts */
	_SetISTR(0);

	/* Set interrupt mask */
	_SetCNTR(CNTR_MASK);
}

void USB_LP_CAN1_RX0_IRQHandler(void) {
	volatile uint16_t istr;

	while ((istr = _GetISTR() & ISTR_MASK) != 0) {

		/* Handle EP data */
		if (READ_BIT(istr, ISTR_CTR)) {

			/* Handle data on EP */
			_SetISTR((uint16_t) CLR_CTR);
			USB_EPHandler(_GetISTR());
		}

		/* Handle Reset */
		if (READ_BIT(istr, ISTR_RESET)) {
			_SetISTR((uint16_t) CLR_RESET);
			USB_Reset();
		}

		/* Handle Suspend */
		if (READ_BIT(istr, ISTR_SUSP)) {
			_SetISTR((uint16_t) CLR_SUSP);

			/* If device address is assigned, then reset it */
			if (_GetDADDR() & 0x007f) {
				_SetDADDR(0);
				_SetCNTR(_GetCNTR() & ~CNTR_SUSPM);
			}
		}

		/* Handle Wakeup */
		if (READ_BIT(istr, ISTR_WKUP)) {
			_SetISTR((uint16_t) CLR_WKUP);
		}
	}

	/* Default to clear all interrupt flags */
	_SetISTR(0);
}
