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

This allowed for a very small bootloader ~~~4 KB~~~ **2 KB** on STM32F10x devices. On STM32F4xx devices there is no point to make the bootloader much smaller than 16 KB because the first flash page is already 16 KB.


This repo is based on **bootsector's**  [stm32-hid-bootloader](https://github.com/bootsector/stm32-hid-bootloader) repository but is customized to follows the [STM32duino](https://github.com/rogerclarkmelbourne/Arduino_STM32) ecosystem requirements. The source files (Bootloader and CLI) can be compiled on ***Windows***, ***Linux*** or ***Mac***


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