TARGET = hid_bootloader
FLASH  = st-flash
DEVICE = STM32F10X_MD

VECTOR_TABLE_OFFSET = 0x0000

SRCS = main.c usb.c hid.c led.c

DEVICE_SRCS += CMSIS/Device/ST/STM32F10x/Source/Templates/gcc/startup_stm32f10x_md.s
DEVICE_SRCS += system_stm32f10x.c

INCLUDE_DIRS += -I .
INCLUDE_DIRS += -I CMSIS/Device/ST/STM32F10x/Include 
INCLUDE_DIRS += -I CMSIS/Include

#LINKER_SCRIPT = STM32F103C8T6.ld
#TARGETFLAGS= -DTARGET_GENERIC_F103_PC13
#LINKER_SCRIPT = STM32F103CBT6.ld
#TARGETFLAGS= -DTARGET_MAPLE_MINI

SRCS += $(DEVICE_SRCS)
OBJS = $(addsuffix .o,  $(basename $(SRCS)))

ASFLAGS += -mcpu=cortex-m3 -mthumb

CFLAGS += -mcpu=cortex-m3 -mthumb -Wall -Os
CFLAGS += -std=gnu99
CFLAGS += -fno-common -static
CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += -specs=nano.specs -specs=nosys.specs
CFLAGS += $(INCLUDE_DIRS)
CFLAGS += -D$(DEVICE)
CFLAGS += -DVECTOR_TABLE_OFFSET=$(VECTOR_TABLE_OFFSET)
CFLAGS += $(TARGETFLAGS)

LDFLAGS += -Wl,-Map=$(TARGET).map,--cref
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -Wl,--print-memory-usage
LDFLAGS += -T$(LINKER_SCRIPT)

CC = arm-none-eabi-gcc
AS = arm-none-eabi-as
LD = arm-none-eabi-ld
OBJDUMP = arm-none-eabi-objdump
OBJCOPY = arm-none-eabi-objcopy
SIZE = arm-none-eabi-size
GDB = arm-none-eabi-gdb

ECHO = echo
RM = rm

program: $(TARGET).hex
	$(FLASH) write $(TARGET).bin 0x8000000
.PHONY: all build output info size clean flash

all: $(SRCS) clean gccversion build output info size

maple-mini: $(SRCS) clean gccversion build_maple-mini copy_maple_mini info size
generic-pc13: $(SRCS) clean gccversion build_generic-pc13 copy_generic-pc13 info size
generic-pd2: $(SRCS) clean gccversion build_generic-pd2 copy_generic-pd2 info size
generic-pd1: $(SRCS) clean gccversion build_generic-pd1 copy_generic-pd1 info size
generic-pa1: $(SRCS) clean gccversion build_generic-pa1 copy_generic-pa1 info size
generic-pb9: $(SRCS) clean gccversion build_generic-pb9 copy_generic-pb9 info size
generic-pe2: $(SRCS) clean gccversion build_generic-pe2 copy_generic-pe2 info size
generic-pa9: $(SRCS) clean gccversion build_generic-pa9 copy_generic-pa9 info size
generic-pe5: $(SRCS) clean gccversion build_generic-pe5 copy_generic-pe5 info size
generic-pb7: $(SRCS) clean gccversion build_generic-pb7 copy_generic-pb7 info size
generic-pb0: $(SRCS) clean gccversion build_generic-pb0 copy_generic-pb0 info size
generic-pb12: $(SRCS) clean gccversion build_generic-pb12 copy_generic-pb12 info size

build: $(TARGET).elf

build_maple-mini: TARGETFLAGS= -DTARGET_MAPLE_MINI
build_maple-mini: LINKER_SCRIPT=STM32F103CBT6.ld
build_maple-mini: $(TARGET).elf $(TARGET).bin
copy_maple_mini:
	cp $(TARGET).bin bootloader_only_binaries/hid_maple_mini.bin

build_generic-pc13: TARGETFLAGS= -DTARGET_GENERIC_F103_PC13
build_generic-pc13: LINKER_SCRIPT=STM32F103C8T6.ld
build_generic-pc13: $(TARGET).elf $(TARGET).bin
copy_generic-pc13:
	cp $(TARGET).bin bootloader_only_binaries/hid_generic_pc13.bin

build_generic-pd2: TARGETFLAGS= -DTARGET_GENERIC_F103_PD2
build_generic-pd2: LINKER_SCRIPT=STM32F103C8T6.ld
build_generic-pd2: $(TARGET).elf $(TARGET).bin
copy_generic-pd2:
	cp $(TARGET).bin bootloader_only_binaries/hid_generic_pd2.bin

build_generic-pd1: TARGETFLAGS= -DTARGET_GENERIC_F103_PD1
build_generic-pd1: LINKER_SCRIPT=STM32F103C8T6.ld
build_generic-pd1: $(TARGET).elf $(TARGET).bin
copy_generic-pd1:
	cp $(TARGET).bin bootloader_only_binaries/hid_generic_pd1.bin

build_generic-pa1: TARGETFLAGS= -DTARGET_GENERIC_F103_PA1
build_generic-pa1: LINKER_SCRIPT=STM32F103C8T6.ld
build_generic-pa1: $(TARGET).elf $(TARGET).bin
copy_generic-pa1:
	cp $(TARGET).bin bootloader_only_binaries/hid_generic_pa1.bin

build_generic-pb9: TARGETFLAGS= -DTARGET_GENERIC_F103_PB9
build_generic-pb9: LINKER_SCRIPT=STM32F103C8T6.ld
build_generic-pb9: $(TARGET).elf $(TARGET).bin
copy_generic-pb9:
	cp $(TARGET).bin bootloader_only_binaries/hid_generic_pb9.bin

build_generic-pe2: TARGETFLAGS= -DTARGET_GENERIC_F103_PE2
build_generic-pe2: LINKER_SCRIPT=STM32F103C8T6.ld
build_generic-pe2: $(TARGET).elf $(TARGET).bin
copy_generic-pe2:
	cp $(TARGET).bin bootloader_only_binaries/hid_generic_pe2.bin

build_generic-pa9: TARGETFLAGS= -DTARGET_GENERIC_F103_PA9
build_generic-pa9: LINKER_SCRIPT=STM32F103C8T6.ld
build_generic-pa9: $(TARGET).elf $(TARGET).bin
copy_generic-pa9:
	cp $(TARGET).bin bootloader_only_binaries/hid_generic_pa9.bin

build_generic-pe5: TARGETFLAGS= -DTARGET_GENERIC_F103_PE5
build_generic-pe5: LINKER_SCRIPT=STM32F103C8T6.ld
build_generic-pe5: $(TARGET).elf $(TARGET).bin
copy_generic-pe5:
	cp $(TARGET).bin bootloader_only_binaries/hid_generic_pe5.bin

build_generic-pb7: TARGETFLAGS= -DTARGET_GENERIC_F103_PB7
build_generic-pb7: LINKER_SCRIPT=STM32F103C8T6.ld
build_generic-pb7: $(TARGET).elf $(TARGET).bin
copy_generic-pb7:
	cp $(TARGET).bin bootloader_only_binaries/hid_generic_pb7.bin

build_generic-pb0: TARGETFLAGS= -DTARGET_GENERIC_F103_PB0
build_generic-pb0: LINKER_SCRIPT=STM32F103C8T6.ld
build_generic-pb0: $(TARGET).elf $(TARGET).bin
copy_generic-pb0:
	cp $(TARGET).bin bootloader_only_binaries/hid_generic_pb0.bin

build_generic-pb12: TARGETFLAGS= -DTARGET_GENERIC_F103_PB12
build_generic-pb12: LINKER_SCRIPT=STM32F103C8T6.ld
build_generic-pb12: $(TARGET).elf $(TARGET).bin
copy_generic-pb12:
	cp $(TARGET).bin bootloader_only_binaries/hid_generic_pb12.bin


$(TARGET).elf: $(OBJS)
	$(CC) $(LDFLAGS) $(CFLAGS) $(OBJS) -o "$@"

.SECONDARY: $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c "$<" -o "$@"

%.o: %.s
	$(AS) $(ASFLAGS) -c "$<" -o "$@"

%.s : %.c
	$(CC) -S $(CFLAGS) $< -o $@

output: $(TARGET).hex $(TARGET).bin

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

gccversion:
	@$(CC) --version

info: $(TARGET).elf
	$(OBJDUMP) -x -S $(TARGET).elf > $(TARGET).lst
	$(OBJDUMP) -D $(TARGET).elf > $(TARGET).dis
	$(SIZE) $(TARGET).elf > $(TARGET).size

size: $(TARGET).elf
	$(SIZE) $(TARGET).elf

clean:
	$(RM) -f $(TARGET).elf
	$(RM) -f $(TARGET).bin
	$(RM) -f $(TARGET).hex
	$(RM) -f $(TARGET).size
	$(RM) -f $(TARGET).lst
	$(RM) -f $(TARGET).dis
	$(RM) -f $(TARGET).map
	$(RM) -f $(OBJS)

