# Ambiq SDK specific Makefile includes

TARGET := OPENLOG_ARTEMIS
# arm-none-eabi is only compiler tested
COMPILERNAME := gcc

# Specify paths to the SDK and/or the BSP
SDKPATH			?= 
BSP			?=
TOOLPATH		?=

ifeq ($(SDKPATH),)
    SDKPATH		=./AmbiqSDK
    $(info Using SDK root: $(SDKPATH))
else
   # When the SDKPATH is given export it
   export SDKPATH
endif

ifeq ($(BSP),)
    BSP=$(SDKPATH)/boards_sfe/$(BOARD)/bsp
else
    export BSP
endif

### Bootloader Tools and definitions
ifeq ($(TOOLPATH),)
    TOOLPATH=$(SDKPATH)/boards_sfe/common/tools_sfe
endif

# Sparkfun/Ambiq loader options
# You can override these values on the command line e.g. make bootload COM_PORT=/dev/ttyUSB0
# COM_PORT is the serial port to use to flash device. 
COM_PORT	?=

# Baud rate setting of the Ambiq Secue Bootloader (ASB). Defaults to 115200 if unset
ASB_UPLOAD_BAUD	?=

# Baud rate setting of the SparkFun Variable Loader (SVL). Defaults to 230400 if unset
SVL_UPLOAD_BAUD	?=

# PYTHON3 should evaluate to a call to the Python3 executable on your machine
PYTHON3		?=

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

### Bootloader Tools
ASB_UPLOADER=$(PYTHON3) $(TOOLPATH)/asb/asb.py
SVL_UPLOADER=$(PYTHON3) $(TOOLPATH)/svl/svl.py

FROZEN_MANIFEST ?= $(BOARD_DIR)/manifest.py
