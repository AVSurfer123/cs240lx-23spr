# list out the driver program source 
PROGS := 2-thread-driver.c
PROGS += 1-keypress.c
PROGS += 0-dump-driver.c 
PROGS += 3-ir-uart.c

# COMMON_SRC := rpi-thread.c thread-asm.S

STAFF_OBJS += staff-rpi-thread.o
STAFF_OBJS += staff-rpi-thread-asm.o

# set RUN = 1 if you want the code to automatically run after building.
RUN = 0
OPT_LEVEL = -O2

DEPS = ./Makefile
include $(CS140E_2023_PATH)/libpi/mk/Makefile.template-v2
