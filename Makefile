
# Project Name
PROJECT_NAME = BombaAgua

# Project versión
VERSION = 1_0

# Microcontroller Model
CPU = stm8s103f3

# CPU speed in Hz
CPU_FREQ =  16000000

# Source files to compile
SOURCE_FILES = main.c
#SOURCE_FILES += other source.c

# stm8happy framework installation PATH
STM8HAPPY_PATH = ./../../stm8happy

# Path for extra include files
EXTRA_INCLUDE_PATH = ./

# stm8happy internals.
include $(STM8HAPPY_PATH)/tools/Makefile.common
