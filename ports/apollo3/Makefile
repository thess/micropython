#******************************************************************************
#
# User Options
#
#******************************************************************************

PROJECTPATH		?=
BOARD			?=

ifeq ($(BOARD),)
    $(error error: no BOARD specified)
else
    $(info Using BOARD=$(BOARD))
endif

ifeq ($(PROJECTPATH),)
    PROJECTPATH = $(CURDIR)
    $(info Using PROJECT root: $(PROJECTPATH))
else
    # When the PROJECTPATH is given export it
    export PROJECTPATH
endif

# Define Ambiq SDK, BSP and Sparkfun tools paths
BOARD_DIR ?= boards/$(BOARD)
include $(BOARD_DIR)/mpconfigboard.mk

BUILD ?= build-$(TARGET)

#******************************************************************************
#
# User Defines / Includes / Sources / Libraries
#
#******************************************************************************

include ../../py/mkenv.mk

# qstr definitions (must come before including py.mk)
QSTR_DEFS = qstrdefsport.h
QSTR_GLOBAL_DEPENDENCIES = $(BOARD_DIR)/mpconfigboard.h

# MicroPython feature configurations
MICROPY_ROM_TEXT_COMPRESSION ?= 1
MICROPY_PY_USSL = 0
MICROPY_SSL_AXTLS = 0
MICROPY_VFS_LFS2 = 1
#MICROPY_PY_BTREE = 1
#BTREE_DEFS_EXTRA = -DDEFPSIZE=1024 -DMINCACHE=3

FROZEN_MANIFEST ?= manifest.py

# include py core make definitions
include $(TOP)/py/py.mk

# Global Defines
DEFINES+= -DAM_FREERTOS
DEFINES+= -DAM_CUSTOM_BDADDR
DEFINES+= -DAM_PACKAGE_BGA
# DEFINES+= -DAM_DEBUG_PRINTF
DEFINES+= -DAM_BSP_DISABLE_BUFFERED_UART
DEFINES+= -DAM_PART_$(PART)

INC+= -I$(SDKPATH)/utils
INC+= -I$(SDKPATH)/mcu/apollo3
INC+= -I$(SDKPATH)/mcu/apollo3/hal
INC+= -I$(SDKPATH)/devices
INC+= -I$(SDKPATH)/third_party/FreeRTOSv10.1.1/Source/portable/GCC/AMapollo2
INC+= -I$(SDKPATH)/third_party/FreeRTOSv10.1.1/Source/include
INC+= -I$(PROJECTPATH)
INC+= -I$(BOARD_DIR)
INC+= -I$(BSP)
INC+= -I$(SDKPATH)/CMSIS/ARM/Include
INC+= -I$(SDKPATH)/CMSIS/AmbiqMicro/Include
INC+= -I$(TOP)
INC+= -I$(BUILD)

VPATH = $(SDKPATH)/utils
VPATH+=:$(SDKPATH)/third_party/FreeRTOSv10.1.1/Source
VPATH+=:$(SDKPATH)/third_party/FreeRTOSv10.1.1/Source/portable/MemMang
VPATH+=:$(SDKPATH)/third_party/FreeRTOSv10.1.1/Source/portable/GCC/AMapollo2
VPATH+=:$(SDKPATH)/devices
VPATH+=:$(SDKPATH)/mcu/apollo3/hal

LIBS = $(SDKPATH)/mcu/apollo3/hal/gcc/bin/libam_hal.a \
       $(BSP)/gcc/bin/libam_bsp.a

SRC_C = $(STARTUP_FILE) \
	main.c \
	mp_rtos.c \
	mp_task.c \
	uart.c \
        moduos.c \
	modutime.c \
	modartemis.c \
	fatfs_port.c \
	mphalport.c \
	modmachine.c \
	machine_pin.c \
	machine_i2c.c \
	machine_spi.c \
	$(SRC_MOD)

RTOS_SRC_C = port.c \
	list.c \
	queue.c \
	tasks.c \
	timers.c \
	heap_5.c \
	event_groups.c

SDK_SRC_C = am_devices_led.c \
	am_util_delay.c \
	am_util_stdio.c \

#	am_util_debug.c \
#	am_util_faultisr.c \
#	am_util_id.c \

LIB_SRC_C = $(addprefix lib/, \
	mp-readline/readline.c \
	libc/string0.c \
	utils/gchelper_native.c \
	utils/pyexec.c \
	timeutils/timeutils.c \
	utils/interrupt_char.c \
	utils/sys_stdio_mphal.c \
	)

SRC_S = lib/utils/gchelper_m3.s

SHELL:=/bin/bash

#### Setup ####

CROSS_COMPILE ?= arm-none-eabi-
PART = APOLLO3
CPU = cortex-m4
FPU = fpv4-sp-d16
# Default to FPU hardware calling convention.  However, some customers and/or
# applications may need the software calling convention.
#FABI = softfp
FABI = hard

OBJDUMP = $(CROSS_COMPILE)objdump

STARTUP_FILE := startup_$(COMPILERNAME).c

CSTD = -std=c99 

CFLAGS = -mthumb -mcpu=$(CPU) -mfpu=$(FPU) -mfloat-abi=$(FABI)
CFLAGS+= -ffunction-sections -fdata-sections -fomit-frame-pointer

# Debugging/Optimization
ifeq ($(DEBUG), 1)
CFLAGS += -g -O0 -D_DEBUG
else
CFLAGS += -g -Os -DNDEBUG
endif
CFLAGS+= $(CSTD) -Wall
CFLAGS+= $(DEFINES)
CFLAGS+= $(INC)

LFLAGS = -mthumb -mcpu=$(CPU) -mfpu=$(FPU) -mfloat-abi=$(FABI)
LFLAGS+= -nostartfiles -static
LFLAGS+= -Wl,--gc-sections,--entry,Reset_Handler,-Map,$(BUILD)/mpy_apollo3.map,--cref
LFLAGS+= -Wl,--start-group -lm -lc -lgcc $(LIBS) -Wl,--end-group

# Additional user specified CFLAGS
CFLAGS+=$(EXTRA_CFLAGS) $(CFLAGS_MOD)

CPFLAGS = -Obinary

ODFLAGS = -S

# Options for mpy-cross
MPY_CROSS_FLAGS += -march=armv7emsp

# MicroPython mkrules expects OBJ := list of object modules
OBJ += $(PY_O)
OBJ += $(addprefix $(BUILD)/, $(SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(RTOS_SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(SDK_SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_S:.s=.o))
OBJ += $(addprefix $(BUILD)/, $(LIB_SRC_C:.c=.o))

SRC_QSTR += $(SRC_C) $(RTOS_SRC_C) $(SDK_SRC_C) $(LIB_SRC_C)

#******************************************************************************
#
# Targets / Rules
#
#******************************************************************************
all: asb

asb: directories $(BUILD)/mpy_apollo3_asb.bin
svl: directories $(BUILD)/mpy_apollo3_svl.bin

directories: $(BUILD)

$(BUILD):
	@mkdir -p $@

$(BUILD)/mpy_apollo3_asb.axf: LINKER_FILE := apollo3_freertos_asb.ld
$(BUILD)/mpy_apollo3_asb.axf: $(OBJ) $(LIBS)
	@echo " Linking $(COMPILERNAME) $@ with script $(LINKER_FILE)" ;\
	$(CC) -Wl,-T,$(LINKER_FILE) -o $@ $(OBJ) $(LFLAGS)

$(BUILD)/mpy_apollo3_svl.axf: LINKER_FILE := apollo3_freertos_svl.ld
$(BUILD)/mpy_apollo3_svl.axf: $(OBJ) $(LIBS)
	@echo " Linking $(COMPILERNAME) $@ with script $(LINKER_FILE)"
	@echo "$^" > $(BUILD)/mpy_apollo3.ind ;\
	$(Q)$(CC) -Wl,-T,$(LINKER_FILE) -o $@ $(OBJ) $(LFLAGS)

$(BUILD)/mpy_apollo3_%.bin: $(BUILD)/mpy_apollo3_%.axf
	@echo " Copying $(COMPILERNAME) $@..." ;\
	$(OBJCOPY) $(CPFLAGS) $< $@ ;\
	$(OBJDUMP) $(ODFLAGS) $< > $(BUILD)/mpy_apollo3.lst

bootload_asb: directories $(BUILD)/mpy_apollo3_asb.bin
	@echo "Uploading using the Ambiq Secure Bootloader"
	$(Q)$(AMBIQ_BIN2BOARD) --bin $(BUILD)/mpy_apollo3_asb.bin --load-address-blob 0x20000 \
              --magic-num 0xCB -o $(BUILD)/mpy_apollo3 --version 0x0 --load-address-wired 0xC000 \
              -i 6 --options 0x1 -b $(ASB_UPLOAD_BAUD) -port $(COM_PORT) -r 2 -v 

bootload_svl: directories $(BUILD)/mpy_apollo3_svl.bin
	@echo "Uploading using the SparkFun Variable Loader"
	$(Q)$(ARTEMIS_SVL) $(COM_PORT) -f $(BUILD)/mpy_apollo3_svl.bin -b $(SVL_UPLOAD_BAUD) -v

bootload: bootload_asb

$(SDKPATH)/mcu/apollo3/hal/gcc/bin/libam_hal.a:
	$(MAKE) -C $(SDKPATH)/mcu/apollo3/hal

$(BSP)/gcc/bin/libam_bsp.a:
	$(MAKE) -C $(BSP)

# Build it
include $(TOP)/py/mkrules.mk

# Generate qstr headers, etc.
genhdr: $(HEADER_BUILD)/qstrdefs.generated.h $(HEADER_BUILD)/mpversion.h $(OBJ_EXTRA_ORDER_DEPS)

# Build frozen_content.c from a manifest
genfrozen: FORCE $(HEADER_BUILD)/qstrdefs.generated.h
	$(MAKE_MANIFEST) -o $(BUILD)/frozen_content.c -v "MPY_DIR=$(TOP)" \
		-v "MPY_LIB_DIR=$(MPY_LIB_DIR)" -v "PORT_DIR=$(shell pwd)" \
		-v "BOARD_DIR=$(BOARD_DIR)" -b "$(BUILD)" \
		$(if $(MPY_CROSS_FLAGS),-f"$(MPY_CROSS_FLAGS)",) $(FROZEN_MANIFEST)
