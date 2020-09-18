#******************************************************************************
#
# Makefile - Rules for building the libraries, examples and docs.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 
# 1. Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
# 
# 2. Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution.
# 
# 3. Neither the name of the copyright holder nor the names of its
# contributors may be used to endorse or promote products derived from this
# software without specific prior written permission.
# 
# Third party software included in this distribution is subject to the
# additional license terms as defined in the /docs/licenses directory.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
#
#******************************************************************************

#******************************************************************************
#
# User Options
#
#******************************************************************************

# You can override these values on the command line e.g. make bootload COM_PORT=/dev/cu***
# COM_PORT is the serial port to use for uploading. For example COM#### on Windows or /dev/cu.usbserial-#### on *nix
COM_PORT		?=
# ASB_UPLOAD_BAUD is the baud rate setting of the Ambiq Secue Bootloader (ASB) as it is configured on the Apollo3. Defautls to 115200 if unset
ASB_UPLOAD_BAUD	?=
# SVL_UPLOAD_BAUD is the baud rate setting of the SparkFun Variable Loader (SVL). Defaults to 230400 if unset
SVL_UPLOAD_BAUD	?=
# PYTHON3 should evaluate to a call to the Python3 executable on your machine
PYTHON3			?=

# *Optionally* specify absolute paths to the SDK and the BSP
# You can do this on the command line - e.g. make bootload SDKPATH=~/$AMBIQ_SDK_ROOT_PATH
# Make sure to use / instead of \ when on Windows
SDKPATH			?=
COMMONPATH		?=
BOARDPATH		?=
PROJECTPATH		?=
BOARD			?=

### Project Settings
TARGET := mpy_apollo3
COMPILERNAME := gcc
PROJECT := $(TARGET)_gcc

#******************************************************************************
#
# Warning Messages
#
#******************************************************************************
ifeq ($(SDKPATH),)
    SDKPATH			=./AmbiqSDK
    $(info Using SDK root: $(SDKPATH))
else
# When the SDKPATH is given export it
export SDKPATH
endif

ifeq ($(BOARD),)
    $(error error: no BOARD specified)
else
    BOARDPATH=$(SDKPATH)/boards_sfe/$(BOARD)
    $(info Using BOARD=$(BOARD) in $(BOARDPATH))
endif

ifeq ($(COM_PORT),)
    COM_PORT=/dev/ttyUSB0
    $(info default COM_PORT as /dev/ttyUSB0)
endif
ifeq ($(PYTHON3),)
    PYTHON3=python3
endif
ifeq ($(ASB_UPLOAD_BAUD),)
    ASB_UPLOAD_BAUD=115200
    $(info defaulting to 115200 baud for ASB)
endif
ifeq ($(SVL_UPLOAD_BAUD),)
    SVL_UPLOAD_BAUD=230400
    $(info defaulting to 230400 baud for SVL)
endif

ifeq ($(COMMONPATH),)
    COMMONPATH			=$(SDKPATH)/boards_sfe/common
    $(info Using COMMON root: $(COMMONPATH))
else
# When the COMMONPATH is given export it
export COMMONPATH
endif

ifeq ($(BOARDPATH),)
    $(error Error: BOARDPATH must be provided)
else
# Ensure that boardpath does not include a trailing '/'
ifeq ($(notdir $(BOARDPATH)),)
    override BOARDPATH:=$(patsubst %/, %,$(BOARDPATH))
    $(warning BOARDPATH had a trivial 'notdir' so we tried changing it to: $(BOARDPATH))
endif
BOARD=$(notdir $(BOARDPATH))
# When the BOARDPATH is given export it
export BOARDPATH
endif

ifeq ($(PROJECTPATH),)
    PROJECTPATH			=$(CURDIR)
    $(info Using PROJECT root: $(PROJECTPATH))
else
# When the PROJECTPATH is given export it
export PROJECTPATH
endif

BUILD ?= $(PROJECTPATH)/$(TARGET)

#******************************************************************************
#
# User Defines / Includes / Sources / Libraries
#
#******************************************************************************

include ../../py/mkenv.mk

# Optional (not currently used for Apollo3)
-include mpconfigboard.mk

# qstr definitions (must come before including py.mk)
QSTR_DEFS = qstrdefsport.h
QSTR_GLOBAL_DEPENDENCIES = $(PROJECTPATH)/mpconfigboard.h

# MicroPython feature configurations
MICROPY_ROM_TEXT_COMPRESSION ?= 1
MICROPY_PY_USSL = 0
MICROPY_SSL_AXTLS = 0
MICROPY_VFS_LFS2 = 1
#MICROPY_PY_BTREE = 1
#BTREE_DEFS_EXTRA = -DDEFPSIZE=1024 -DMINCACHE=3

BOARD_DIR ?= $(PROJECTPATH)
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
INC+= -I$(BOARDPATH)/bsp
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
       $(BOARDPATH)/bsp/gcc/bin/libam_bsp.a

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

### Bootloader Tools
AMBIQ_BIN2BOARD=$(PYTHON3) $(COMMONPATH)/tools_sfe/ambiq/ambiq_bin2board.py
ARTEMIS_SVL=$(PYTHON3) $(COMMONPATH)/tools_sfe/artemis/artemis_svl.py

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
LFLAGS+= -Wl,--gc-sections,--entry,Reset_Handler,-Map,$(BUILD)/$(TARGET).map,--cref
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

asb: directories $(BUILD)/$(TARGET)_asb.bin
svl: directories $(BUILD)/$(TARGET)_svl.bin

define write_ind_file
    temp=($(echo "$1" | tr ";" "\n"))

    #Output the split string
    for o in "${temp[@]}"
    do
        echo $o >> $(BUILD)/$(TARGET).ind
    done
endef

directories: $(BUILD)

$(BUILD):
	@mkdir -p $@

$(BUILD)/$(TARGET)_asb.axf: LINKER_FILE := apollo3_freertos_asb.ld
$(BUILD)/$(TARGET)_asb.axf: $(OBJ) $(LIBS)
	@echo " Linking $(COMPILERNAME) $@ with script $(LINKER_FILE)" ;\
	$(CC) -Wl,-T,$(LINKER_FILE) -o $@ $(OBJ) $(LFLAGS)

$(BUILD)/$(TARGET)_svl.axf: LINKER_FILE := apollo3_freertos_svl.ld
$(BUILD)/$(TARGET)_svl.axf: $(OBJ) $(LIBS)
	@echo " Linking $(COMPILERNAME) $@ with script $(LINKER_FILE)"
	@echo "$^" > $(BUILD)/$(TARGET).ind ;\
	$(Q)$(CC) -Wl,-T,$(LINKER_FILE) -o $@ $(OBJ) $(LFLAGS)

$(BUILD)/$(TARGET)_%.bin: $(BUILD)/$(TARGET)_%.axf
	@echo " Copying $(COMPILERNAME) $@..." ;\
	$(OBJCOPY) $(CPFLAGS) $< $@ ;\
	$(OBJDUMP) $(ODFLAGS) $< > $(BUILD)/$(TARGET).lst

bootload_asb: directories $(BUILD)/$(TARGET)_asb.bin
	@echo "Uploading using the Ambiq Secure Bootloader"
	$(Q)$(AMBIQ_BIN2BOARD) --bin $(BUILD)/$(TARGET)_asb.bin --load-address-blob 0x20000 \
              --magic-num 0xCB -o $(BUILD)/$(TARGET) --version 0x0 --load-address-wired 0xC000 \
              -i 6 --options 0x1 -b $(ASB_UPLOAD_BAUD) -port $(COM_PORT) -r 2 -v 

bootload_svl: directories $(BUILD)/$(TARGET)_svl.bin
	@echo "Uploading using the SparkFun Variable Loader"
	$(Q)$(ARTEMIS_SVL) $(COM_PORT) -f $(BUILD)/$(TARGET)_svl.bin -b $(SVL_UPLOAD_BAUD) -v

bootload: bootload_asb

$(SDKPATH)/mcu/apollo3/hal/gcc/bin/libam_hal.a:
	$(MAKE) -C $(SDKPATH)/mcu/apollo3/hal

$(BOARDPATH)/bsp/gcc/bin/libam_bsp.a:
	$(MAKE) -C $(BOARDPATH)/bsp

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
