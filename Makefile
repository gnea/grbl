# detect what shell is used
ifeq ($(findstring cmd.exe,$(SHELL)),cmd.exe)
$(info "Info : Running on Windows shell cmd.exe")
DEVNUL := NUL
WHICH := where
else
$(info "Info : Running on Unix shell")
DEVNUL := /dev/null
WHICH := which
endif


# Find first cross compilation tools
XTOOLS_DIR = $(abspath $(shell dirname `${WHICH} arm-none-eabi-gcc`)/..)

# Set tool & Lib paths
X_LIBC_DIR = $(XTOOLS_DIR)/arm-none-eabi/lib/armv7e-m/fpu
X_CC = $(XTOOLS_DIR)/bin/arm-none-eabi-gcc
X_OBJCOPY = $(XTOOLS_DIR)/bin/arm-none-eabi-objcopy
X_AR = $(XTOOLS_DIR)/bin/arm-none-eabi-ar
X_LD = $(XTOOLS_DIR)/bin/arm-none-eabi-ld
X_GDB = $(XTOOLS_DIR)/bin/arm-none-eabi-gdb

OUT_DIR = debug

OUTPUT = grbl-stm32

SRC= ./cmsis_boot/startup/startup_stm32f10x_md.c \
     ./cmsis_boot/system_stm32f10x.c \
     ./grbl/coolant_control.c \
     ./grbl/eeprom.c \
     ./grbl/gcode.c \
     ./grbl/jog.c \
     ./grbl/limits.c \
     ./grbl/main.c \
     ./grbl/motion_control.c \
     ./grbl/nuts_bolts.c \
     ./grbl/planner.c \
     ./grbl/print.c \
     ./grbl/probe.c \
     ./grbl/protocol.c \
     ./grbl/report.c \
     ./grbl/serial.c \
     ./grbl/settings.c \
     ./grbl/spindle_control.c \
     ./grbl/stepper.c \
     ./grbl/system.c \
     ./stm_lib/src/misc.c \
     ./stm_lib/src/stm32f10x_exti.c \
     ./stm_lib/src/stm32f10x_flash.c \
     ./stm_lib/src/stm32f10x_gpio.c \
     ./stm_lib/src/stm32f10x_rcc.c \
     ./stm_lib/src/stm32f10x_tim.c \
     ./stm_lib/src/stm32f10x_usart.c \
     ./stm_usb_fs_lib/src/usb_core.c \
     ./stm_usb_fs_lib/src/usb_init.c \
     ./stm_usb_fs_lib/src/usb_int.c \
     ./stm_usb_fs_lib/src/usb_mem.c \
     ./stm_usb_fs_lib/src/usb_regs.c \
     ./stm_usb_fs_lib/src/usb_sil.c \
     ./usb/hw_config.c \
     ./usb/usb_desc.c \
     ./usb/usb_endp.c \
     ./usb/usb_istr.c \
     ./usb/usb_prop.c \
     ./usb/usb_pwr.c \
     ./util/stm32f10x_it.c

OBJ = $(patsubst %.c, %.o, $(SRC))
#OBJ += $(BOARD_DIR)/start.o

# include files
INC = .
INC += cmsis
INC += cmsis_boot
INC += cmsis_boot/startup
INC += grbl
INC += stm_lib/inc
INC += stm_lib/src
INC += stm_usb_fs_lib/inc
INC += stm_usb_fs_lib/src
INC += usb
INC += util

INCLUDE = $(addprefix -I,$(INC))

# compiler flags
# as in coocox
# -mcpu=cortex-m3 ; -mthumb ; -g2 ; -Wall ; -Os ; -DUSE_STDPERIPH_DRIVER ; -D__ASSEMBLY__ ; -D_GRBL_ ; -DSTM32F103C8 ; -DSTM32F10X_MD ; -DLEDBLINK ; -DUSEUSB ; -I. ; -Iusb ; -Iutil ; -Iuseusb ; -Igrbl ;

CFLAGS = -Wall
CFLAGS += -Os
CFLAGS += -g2
CFLAGS += -mthumb 
CFLAGS += -mcpu=cortex-m3

# linker flags
LDSCRIPT = STM32F103C8T6.ld
LDFLAGS = -T$(LDSCRIPT) -Wl,-Map,$(OUT_DIR)/$(OUTPUT).map -Wl,--gc-sections

# defines
DEFINES = -DUSE_STDPERIPH_DRIVER -D__ASSEMBLY__ -D_GRBL_ -DSTM32F103C8 -DSTM32F10X_MD -DLEDBLINK -DUSEUSB

.S.o:
	$(X_CC) $(INCLUDE) $(DEFINES) $(CFLAGS) -c $< -o $@
.c.o:
	$(X_CC) $(INCLUDE) $(DEFINES) $(CFLAGS) -c $< -o $@

.PHONY: all flash grbl_src clean

all:  $(OBJ)
	$(X_CC) $(CFLAGS) $(LDFLAGS) $(OBJ) -lm -o $(OUT_DIR)/$(OUTPUT)
	mv $(OUT_DIR)/$(OUTPUT) $(OUT_DIR)/$(OUTPUT).elf
	$(X_OBJCOPY) -O binary $(OUT_DIR)/$(OUTPUT).elf $(OUT_DIR)/$(OUTPUT).bin

flash:
	make all
	st-flash write $(OUT_DIR)/$(OUTPUT).bin 0x08000000

grbl_src:
	make  all

clean:
	-rm $(OBJ)
	-rm $(OUT_DIR)/$(OUTPUT).map
	-rm $(OUT_DIR)/$(OUTPUT).bin
	-rm $(OUT_DIR)/$(OUTPUT).elf
