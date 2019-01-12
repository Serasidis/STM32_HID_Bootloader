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
#include <string.h>
#include <stdbool.h>
#include "config.h"
#include "usb.h"
#include "hid.h"
#include "led.h"
#include "flash.h"

/* This should be <= MAX_EP_NUM defined in usb.h */
#define EP_NUM 			2

/* Flash memory base address */
#define FLASH_BASE_ADDRESS	0x08000000

/* This should be the last page taken by the bootloader */
#define MIN_PAGE		4

/* Maximum packet size */
#define MAX_PACKET_SIZE		8

/* Command size */
#define COMMAND_SIZE		64

/* Page size */
#define PAGE_SIZE		1024

/* Buffer table base address */
#define BTABLE_ADDRESS		(0x00)

/* EP0  */
/* RX/TX buffer base address */
#define ENDP0_RXADDR		(0x18)
#define ENDP0_TXADDR		(0x58)

/* EP1  */
/* TX buffer base address */
#define ENDP1_TXADDR		(0x100)

/* Upload started flag */
volatile bool UploadStarted;

/* Upload finished flag */
volatile bool UploadFinished;

/* Received command */
static const uint8_t CommandSignature[] = {'B', 'T', 'L', 'D', 'C', 'M', 'D'};

/* Sent command */
static const uint8_t CommandSendNextData[] = {'B', 'T', 'L', 'D', 'C', 'M', 'D', 2};

/* Flash page buffer */
static uint8_t PageData[PAGE_SIZE];

/* Current page number (starts right after bootloader's end */
static volatile uint8_t CurrentPage = 0;

/* Byte offset in flash page */
static volatile uint16_t CurrentPageOffset = 0;

/* USB Descriptors */
static const uint8_t USB_DeviceDescriptor[] = {
	0x12,			// bLength
	0x01,			// bDescriptorType (Device)
	0x10, 0x01,		// bcdUSB 1.10
	0x00,			// bDeviceClass (Use class information in the Interface Descriptors)
	0x00,			// bDeviceSubClass
	0x00,			// bDeviceProtocol
	MAX_PACKET_SIZE,	// bMaxPacketSize0 8
	0x09, 0x12,		// idVendor 0x1209
	0xBA, 0xBE,		// idProduct 0xBEBA
	0x02, 0x00,		// bcdDevice 0.02
	0x01,			// iManufacturer (String Index)
	0x02,			// iProduct (String Index)
	0x00,			// iSerialNumber (String Index)
	0x01 			// bNumConfigurations 1
};

static const uint8_t USB_ConfigurationDescriptor[] = {
	0x09,			// bLength
	0x02,			// bDescriptorType (Configuration)
	0x22, 0x00,		// wTotalLength 34
	0x01,			// bNumInterfaces 1
	0x01,			// bConfigurationValue
	0x00,			// iConfiguration (String Index)
	0xC0,			// bmAttributes Self Powered
	0x32,			// bMaxPower 100mA

	0x09,			// bLength
	0x04,			// bDescriptorType (Interface)
	0x00,			// bInterfaceNumber 0
	0x00,			// bAlternateSetting
	0x01,			// bNumEndpoints 1
	0x03,			// bInterfaceClass
	0x00,			// bInterfaceSubClass
	0x00,			// bInterfaceProtocol
	0x00,			// iInterface (String Index)

	0x09,			// bLength
	0x21,			// bDescriptorType (HID)
	0x11, 0x01,		// bcdHID 1.11
	0x00,			// bCountryCode
	0x01,			// bNumDescriptors
	0x22,			// bDescriptorType[0] (HID)
	0x20, 0x00,		// wDescriptorLength[0] 32

	0x07,			// bLength
	0x05,			// bDescriptorType (Endpoint)
	0x81,			// bEndpointAddress (IN/D2H)
	0x03,			// bmAttributes (Interrupt)
	MAX_PACKET_SIZE, 0x00,	// wMaxPacketSize 8
	0x05 			// bInterval 5 (2^(5-1)=16 micro-frames)
};

static const uint8_t USB_ReportDescriptor[32] = {
	0x06, 0x00, 0xFF,	// Usage Page (Vendor Defined 0xFF00)
	0x09, 0x01,		// Usage (0x01)
	0xA1, 0x01,		// Collection (Application)
	0x09, 0x02,		// 	Usage (0x02)
	0x15, 0x00,		// 	Logical Minimum (0)
	0x25, 0xFF,		// 	Logical Maximum (255)
	0x75, 0x08,		// 	Report Size (8)
	0x95, 0x08,		// 	Report Count (8)
	0x81, 0x02,		// 	Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
	0x09, 0x03,		// 	Usage (0x03)
	0x15, 0x00,		// 	Logical Minimum (0)
	0x25, 0xFF,		// 	Logical Maximum (255)
	0x75, 0x08,		// 	Report Size (8)
	0x95, 0x40,		// 	Report Count (64)
	0x91, 0x02,		// 	Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
	0xC0 			// End Collection
};

/* USB String Descriptors */
static const uint8_t USB_LangIDStringDescriptor[] = {
	0x04,			// bLength
	0x03,			// bDescriptorType (String)
	0x09, 0x04		// English (United States)
};

static const uint8_t USB_VendorStringDescriptor[] = {
	0x22,			// bLength
	0x03,			// bDescriptorType (String)
	'w', 0, 'w', 0, 'w', 0, '.', 0, 's', 0, 'e', 0, 'r', 0, 'a', 0, 's', 0,
	'i', 0, 'd', 0, 'i', 0, 's', 0, '.', 0, 'g', 0, 'r', 0
};

static const uint8_t USB_ProductStringDescriptor[] = {
	0x2C,			// bLength
	0x03,			// bDescriptorType (String)
	'S', 0, 'T', 0, 'M', 0, '3', 0, '2', 0, 'F', 0, ' ', 0, 'H', 0, 'I', 0,
	'D', 0, ' ', 0, 'B', 0, 'o', 0, 'o', 0, 't', 0, 'l', 0, 'o', 0, 'a', 0,
	'd', 0, 'e', 0, 'r', 0
};

static void HIDUSB_GetDescriptor(USB_SetupPacket *SPacket) {

	switch (SPacket->wValue.H) {
	case USB_DEVICE_DESC_TYPE:
		USB_SendData(0, (uint16_t *) USB_DeviceDescriptor,
			SPacket->wLength > sizeof (USB_DeviceDescriptor) ?
				sizeof (USB_DeviceDescriptor) : SPacket->wLength);
		break;

	case USB_CFG_DESC_TYPE:
		USB_SendData(0, (uint16_t *) USB_ConfigurationDescriptor,
			SPacket->wLength > sizeof (USB_ConfigurationDescriptor) ?
				sizeof (USB_ConfigurationDescriptor) : SPacket->wLength);
		break;

	case USB_REPORT_DESC_TYPE:
		USB_SendData(0, (uint16_t *) USB_ReportDescriptor,
			SPacket->wLength > sizeof (USB_ReportDescriptor) ?
				sizeof (USB_ReportDescriptor) : SPacket->wLength);
		break;

	case USB_STR_DESC_TYPE:
		switch (SPacket->wValue.L) {
		case 0x00:
			USB_SendData(0, (uint16_t *)USB_LangIDStringDescriptor,
				SPacket->wLength > sizeof (USB_LangIDStringDescriptor) ?
					sizeof (USB_LangIDStringDescriptor) : SPacket->wLength);
			break;

		case 0x01:
			USB_SendData(0, (uint16_t *) USB_VendorStringDescriptor,
				SPacket->wLength > sizeof (USB_VendorStringDescriptor) ?
					sizeof (USB_VendorStringDescriptor) : SPacket->wLength);
			break;

		case 0x02:
			USB_SendData(0, (uint16_t *) USB_ProductStringDescriptor,
				SPacket->wLength > sizeof (USB_ProductStringDescriptor) ?
					sizeof (USB_ProductStringDescriptor) : SPacket->wLength);
			break;

		default:
			USB_SendData(0, 0, 0);
		}
		break;

	default:
		USB_SendData(0, 0, 0);
		break;
	}
}

static uint8_t HIDUSB_PacketIsCommand(void) {
	uint8_t hasdata = 0;

	for (int i = sizeof (CommandSignature) + 1; i < COMMAND_SIZE; i++) {
		hasdata |= PageData[i];
	}
	if (hasdata) {
		return 0;
	}
	if (memcmp(PageData, CommandSignature, sizeof (CommandSignature)) == 0) {
		return 1;
	}
	return 0;
}

static void HIDUSB_HandleData(uint8_t *data) {
	uint32_t PageAddress;

	memcpy(PageData + CurrentPageOffset, data, MAX_PACKET_SIZE);
	CurrentPageOffset += MAX_PACKET_SIZE;
	if (CurrentPageOffset == COMMAND_SIZE) {
		if (HIDUSB_PacketIsCommand()) {
			switch (PageData[sizeof (CommandSignature)]) {

			case 0x00:

				/* Reset Page Command */
				UploadStarted = true;
				CurrentPage = MIN_PAGE;
				CurrentPageOffset = 0;
			break;

			case 0x01:

				/* Reboot MCU Command */
				UploadFinished = true;
			break;

			default:
				break;
			}
		}
	} else {
		if (CurrentPageOffset >= PAGE_SIZE) {
			led_on();
			PageAddress = FLASH_BASE_ADDRESS + (CurrentPage * PAGE_SIZE);
			FLASH_WritePage(PageAddress, PageData, PAGE_SIZE);
			CurrentPage++;
			CurrentPageOffset = 0;
			USB_SendData(ENDP1, (uint16_t *) CommandSendNextData,
				sizeof (CommandSendNextData));
			led_off();
		}
	}
}

void HIDUSB_Reset(void) {

	/* Initialize Flash Page Settings */
	CurrentPage = MIN_PAGE;
	CurrentPageOffset = 0;
	_SetBTABLE(BTABLE_ADDRESS);

	/* Initialize Endpoint 0 */
	_SetEPType(ENDP0, EP_CONTROL);
	_SetEPRxAddr(ENDP0, ENDP0_RXADDR);
	_SetEPTxAddr(ENDP0, ENDP0_TXADDR);
	_ClearEP_KIND(ENDP0);
	_SetEPRxValid(ENDP0);

	/* Initialize Endpoint 1 */
	_SetEPType(ENDP1, EP_INTERRUPT);
	_SetEPTxAddr(ENDP1, ENDP1_TXADDR);
	_SetEPTxCount(ENDP1, MAX_PACKET_SIZE);
	_SetEPRxStatus(ENDP1, EP_RX_DIS);
	_SetEPTxStatus(ENDP1, EP_TX_NAK);

	/* Set address in every used endpoint */
	for (int i = 0; i < EP_NUM; i++) {
		_SetEPAddress((uint8_t ) i, (uint8_t ) i);
		RxTxBuffer[i].MaxPacketSize = MAX_PACKET_SIZE;
	}

	/* Set device address and enable function */
	_SetDADDR(0 | DADDR_EF);
}

void HIDUSB_EPHandler(uint16_t Status) {
	uint8_t EPn = Status & USB_ISTR_EP_ID;
	uint16_t EP = _GetENDPOINT(EPn);
	USB_SetupPacket *SetupPacket;

	/* OUT and SETUP packets (data reception) */
	if (EP & EP_CTR_RX) {

		/* Copy from packet area to user buffer */
		USB_PMA2Buffer(EPn);
		if (EPn == 0) {

			/* If control endpoint */
			if (EP & USB_EP0R_SETUP) {
				SetupPacket = (USB_SetupPacket *) RxTxBuffer[EPn].RXB;
				switch (SetupPacket->bRequest) {

				case USB_REQUEST_SET_ADDRESS:
					DeviceAddress = SetupPacket->wValue.L;
					USB_SendData(0, 0, 0);
					break;

				case USB_REQUEST_GET_DESCRIPTOR:
					HIDUSB_GetDescriptor(SetupPacket);
					break;

				case USB_REQUEST_GET_STATUS:
					USB_SendData(0, (uint16_t *) &DeviceStatus, 2);
					break;

				case USB_REQUEST_GET_CONFIGURATION:
					USB_SendData(0, (uint16_t *) &DeviceConfigured, 1);
					break;

				case USB_REQUEST_SET_CONFIGURATION:
					DeviceConfigured = 1;
					USB_SendData(0, 0, 0);
					break;

				case USB_REQUEST_GET_INTERFACE:
					USB_SendData(0, 0, 0);
					break;

				default:
					USB_SendData(0, 0, 0);
					_SetEPTxStatus(0, EP_TX_STALL);
					break;
				}
			} else {

				/* OUT packet */
				if (RxTxBuffer[EPn].RXL) {
					HIDUSB_HandleData((uint8_t *) RxTxBuffer[EPn].RXB);
				}
			}

		}
		_ClearEP_CTR_RX(EPn);
		_SetEPRxValid(EPn);
	}
	if (EP & EP_CTR_TX) {

		/* Something transmitted */
		if (DeviceAddress) {

			/* Set device address and enable function */
			_SetDADDR(DeviceAddress | DADDR_EF);
			DeviceAddress = 0;
		}
		USB_Buffer2PMA(EPn);
		_SetEPTxValid(EPn);
		_ClearEP_CTR_TX(EPn);
		if (EPn == ENDP1) {
			_SetEPTxStatus(ENDP1, EP_TX_NAK);
		}
	}
}
