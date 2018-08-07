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
*	This HID bootloader work with bluepill + STM32duino + Arduino IDE <http://www.stm32duino.com/>
*
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include "rs232.h"
#include "hidapi.h"

#define SECTOR_SIZE 1024
#define HID_TX_SIZE    65
#define HID_RX_SIZE     9


int serial_init(char *argument, uint8_t __timer);


static int usb_write(hid_device *device, uint8_t *buffer, int len) {
	int retries = 20;
	int retval;

	while(((retval = hid_write(device, buffer, len)) < len) && --retries) {
		if(retval < 0) {
			usleep(100 * 1000); // No data has been sent here. Delay and retry.
			//sleep(1);
		} else {
			return 0; // Partial data has been sent. Firmware will be corrupted. Abort process.
		}
	}

	if(retries <= 0) {
		return 0;
	}

	return 1;
}

int main(int argc, char *argv[]) {
	uint8_t page_data[SECTOR_SIZE];
	uint8_t hid_tx_buf[HID_TX_SIZE];
  uint8_t hid_rx_buf[HID_RX_SIZE];
	uint8_t CMD_RESET_PAGES[8] = {'B','T','L','D','C','M','D', 0x00};
	uint8_t CMD_REBOOT_MCU[8] = {'B','T','L','D','C','M','D', 0x01};
	hid_device *handle = NULL;
	size_t read_bytes;
	FILE *firmware_file = NULL;
	int error = 0;
	uint32_t n_bytes = 0;
  int i;
	setbuf(stdout, NULL);
  uint8_t _timer = 0;
	
	printf("\n+----------------------------------------------------------------------+\n");
	printf  ("|         HID-Flash v2.0 - STM32 HID Bootloader Flash Tool            |\n");
	printf  ("|     (c) 04/2018 - Bruno Freitas - http://www.brunofreitas.com/       |\n");
	printf  ("|     (c) 04/2018 - Vassilis Serasidis - http://www.serasidis.gr/      |\n");
	printf  ("|   Customized for STM32duino ecosystem - http://www.stm32duino.com/   |\n");
	printf  ("+----------------------------------------------------------------------+\n\n");
	
	if(argc < 3) {
		printf("Usage: hid-flash <bin_firmware_file> <comport> <delay (optional)>\n");
		return 1;
	}else if(argc == 4){
    _timer = atol(argv[3]);
  }
  
	serial_init(argv[2], _timer); //Setting up Serial port
  hid_init();
  
  printf("> Searching for 1209:BEBA HID device...\n");
  
   for(i=0;i<10;i++){ //Trying 10 times to open the HID device.
    printf("#");
    handle = hid_open(0x1209, 0xBEBA, NULL);
    if (handle){
      break;
    }
    sleep(1);
  }
  
  if(i == 10){
    printf("\n> Unable to open the HID device.\n");
		error = 1;
		goto exit;
  }  
  
  printf("\n> 1209:BEBA device is found !\n");
  
	firmware_file = fopen(argv[1], "rb");
	if(!firmware_file) {
		printf("> Error opening firmware file: %s\n", argv[1]);
		error = 1;
		goto exit;
	}

	// Send RESET PAGES command to put HID bootloader in initial stage...
	memset(hid_tx_buf, 0, sizeof(hid_tx_buf)); //Fill the hid_tx_buf with zeros.
	memcpy(&hid_tx_buf[1], CMD_RESET_PAGES, sizeof(CMD_RESET_PAGES));

	printf("> Sending <reset pages> command...\n");

	// Flash is unavailable when writing to it, so USB interrupt may fail here
	if(!usb_write(handle, hid_tx_buf, HID_TX_SIZE)) {
		printf("> Error while sending <reset pages> command.\n");
		error = 1;
		goto exit;
	}
	memset(hid_tx_buf, 0, sizeof(hid_tx_buf));

	// Send Firmware File data
	printf("> Flashing firmware...\n");

	memset(page_data, 0, sizeof(page_data));
	read_bytes = fread(page_data, 1, sizeof(page_data), firmware_file);

	while(read_bytes > 0) {

		for(int i = 0; i < SECTOR_SIZE; i += HID_TX_SIZE - 1) {
			memcpy(&hid_tx_buf[1], page_data + i, HID_TX_SIZE - 1);

      if((i % 1024) == 0){
        printf(".");
      }
			
			// Flash is unavailable when writing to it, so USB interrupt may fail here
			if(!usb_write(handle, hid_tx_buf, HID_TX_SIZE)) {
				printf("> Error while flashing firmware data.\n");
				error = 1;
				goto exit;
			}
      n_bytes += (HID_TX_SIZE - 1);
      usleep(500);
		}
    
    printf(" %d Bytes\n", n_bytes);

    do{
      hid_read(handle, hid_rx_buf, 9);
      usleep(500);
    }while(hid_rx_buf[7] != 0x02);
    
		memset(page_data, 0, sizeof(page_data));
		read_bytes = fread(page_data, 1, sizeof(page_data), firmware_file);
	}

	printf("\n> Done!\n");
	
 	// Send CMD_REBOOT_MCU command to reboot the microcontroller...
	memset(hid_tx_buf, 0, sizeof(hid_tx_buf));
	memcpy(&hid_tx_buf[1], CMD_REBOOT_MCU, sizeof(CMD_REBOOT_MCU));

	printf("> Sending <reboot mcu> command...\n");

	// Flash is unavailable when writing to it, so USB interrupt may fail here
	if(!usb_write(handle, hid_tx_buf, HID_TX_SIZE)) {
		printf("> Error while sending <reboot mcu> command.\n");
	}
	
	exit:

	if(handle) {
		hid_close(handle);
	}

	hid_exit();

	if(firmware_file) {
		fclose(firmware_file);
	}

	return error;
}

int serial_init(char *argument, uint8_t __timer) {

  printf("> Trying to open the comport...\n");
  if(RS232_OpenComport(argument)){
    return(0);
  }
	printf("> Toggling DTR...\n");
  
  RS232_disableRTS();
 	RS232_enableDTR();
	usleep(200000L);
  RS232_disableDTR();
	usleep(200000L);
	RS232_enableDTR();
	usleep(200000L);
  RS232_disableDTR();
	usleep(200000L);
	RS232_send_magic();
	usleep(200000L);
	RS232_CloseComport();
  
  //printf("A %i\n",__timer);
  if (__timer > 0) {
    sleep(__timer);
	}
	return 0;
}
