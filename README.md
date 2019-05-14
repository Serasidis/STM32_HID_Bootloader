STM32_HID_bootloader
=============

## Notice

This software is experimental and a work in progress. Under no circumstances should these files be used in relation to any critical system(s). Use of these files is at your own risk.

## THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


## Summary
This is a driverless (no USB drivers needed, even on Windows) USB HID bootloader
for **STM32F10x** and **STM32F4xx** devices. 

It doesn't use ST libraries since they are bloated and buggy. Only CMSIS and
some required system files and macros have been used from ST provided SDK.

This allowed for a very small bootloader ~~4 KB~~ **2 KB** on STM32F10x devices. On STM32F4xx devices there is no point to make the bootloader much smaller than 16 KB because the first flash page is already 16 KB.


This repo is based on **bootsector's**  [stm32-hid-bootloader](https://github.com/bootsector/stm32-hid-bootloader) repository but is customized to follows the [STM32duino](https://github.com/rogerclarkmelbourne/Arduino_STM32) ecosystem requirements. The source files (Bootloader and CLI) can be compiled on ***Windows***, ***Linux*** or ***Mac***

## Installing the HID bootloader to STM32 devices
###(ST-Link version)

1. Download the HID binaries.  [STM32F103 binaries](https://github.com/Serasidis/STM32_HID_Bootloader/tree/master/bootloader_F1/bootloader_only_binaries) , [STM32F4xx binaries](https://github.com/Serasidis/STM32_HID_Bootloader/tree/master/bootloader_F4/bootloader_only_binaries)
2. Download the [texane stlink](https://github.com/texane/stlink/releases/tag/1.3.0) according to your operating system (Windows, MacOSX, Linux)
3. Extract the texane stlink to your hard disk. You will need the file ```st-flash``` (it is into the bin folder).
4. Copy into that folder the HID Bootloader file (xxx.bin) according to your board. ``` 	hid_generic_pc13.bin``` The on-board LED is connectet to the PC13 pin.
5. Connect ```BOOT-0``` and ```BOOT-1``` pins or (on-board jumpers) to GND ('0' on BluePill board) 
6. Type on Windows CMD (or Linux terminal) ```st-flash.exe write hid_generic_pc13.bin 0x8000000``` for programming the HID Bootloader firmware to a **BluePill** board.
7. Apply the ```Arduino_STM32_patch``` from [here](https://github.com/Serasidis/STM32_HID_Bootloader) in case of using Roger's Core (select the zip file according to your Operating System). 
8. ***STM Official Core will be supported soon***.
9. Select from Arduino IDE ```Tools > Board > [your_stm32_board]```
10. Select  ```Tools > Upload method > HID Bootloader 2.1``` or newer 
11. You are ready !

Normally, both ```BOOT-0``` and ```BOOT-1``` must be connected to '0'. If you connect ```BOOT-1``` pin to 3.3V (or '1' on BluePill boards), the board will stay in HID Bootloader mode.  
  

### (Serial Dongle version)

1. Download the HID binaries.  [STM32F103 binaries](https://github.com/Serasidis/STM32_HID_Bootloader/tree/master/bootloader_F1/bootloader_only_binaries) , [STM32F4xx binaries](https://github.com/Serasidis/STM32_HID_Bootloader/tree/master/bootloader_F4/bootloader_only_binaries)
2. Download the [stm32flash](https://github.com/rogerclarkmelbourne/Arduino_STM32/tree/master/tools) from Roger's Clark Github repo.
3. Extract the stm32flash to your hard disk.
4. Copy into that folder the HID Bootloader file (xxx.bin) according to your board. ``` 	hid_generic_pc13.bin``` The on-board LED is connectet to the PC13 pin.
5. Set ```BOOT-0``` pin to '1' (3.3V) and reset the board
6. Type on Windows CMD (or Linux terminal) ```stm32flash.exe -g 0x8000000 -b 115200 -w hid_generic_pc13.bin COM2 ``` or ```stm32flash -g 0x8000000 -b 115200 -w hid_generic_pc13.bin /dev/ttyS0``` for programming the HID Bootloader firmware to a **BluePill** board.
7. Apply the ```Arduino_STM32_patch``` from [here](https://github.com/Serasidis/STM32_HID_Bootloader) in case of using Roger's Core (select the zip file according to your Operating System). 
8. ***STM Official Core will be supported soon***.
9. Select from Arduino IDE ```Tools > Board > [your_stm32_board]```
10. Select  ```Tools > Upload method > HID Bootloader 2.1``` or newer 
11. You are ready !

## CLI folder

`cli` folder contains the source code for creating the command line tool **hid-flash** tool. 
This bootloader should't have any compiler restrictions, so it should work with
any GCC ARM toolchain version (latest is always recommended!). Just run 'make' on that folder.

### Windows examples:

```D:\STM32_HID_bootloader\cli>make clean``` Clears the previous generated files
```D:\STM32_HID_bootloader\cli>make``` Creates the **hid-flash.exe** file


## Bootloader folder
`bootloader` folder contains the source code for creating the **hid_bootloader.bin** file that is burned into the STM32F103 flash memory. Currently, only **STM32F103** MCU is supported. Making the ***hid_bootloader.bin***

### Examples:
***STM32F10x***

```D:\STM32_HID_bootloader\bootloader_F1\bootloader>make clean``` Clears the previous generated files
```D:\STM32_HID_bootloader\bootloader_F1\bootloader>make``` Creates the **hid_bootloader.bin** file



***STM32F4xx***

```D:\STM32_HID_bootloader\bootloader_F4\bootloader>make clean``` Clears the previous generated files
```D:\STM32_HID_bootloader\bootloader_F4\bootloader>make``` Creates the **hid_bootloader.bin** file

The binary file can be found in:

```D:\STM32_HID_bootloader\bootloader_F4\bootloader\build\HID_Bootloader_F4.bin```

### Screenshot

<p align="center">
<img src="pictures/Arduino_IDE_1_8_5.PNG">
</p>
