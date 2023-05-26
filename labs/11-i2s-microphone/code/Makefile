# list out the driver program source 
# PROGS = 0-clock-loopback.c
PROGS = 1-mic-read.c
PROGS = $(wildcard tests/[9]*.c)

# swap if you want.
COMMON_SRC = i2s.c fft.c
# STAFF_OBJS = staff-i2s.o

# pick one of your attachment points (first or last) consistently.
# BOOTLOADER = my-install --last

OPT_LEVEL = -O3

# set RUN = 1 if you want the code to automatically run after building.
RUN = 1

USE_FP = 1

LIB_POST := $(CS240LX_2023_PATH)/lib/libgcc.a
LIBS += $(LIB_POST)
LIBS += $(LIBM)

DEPS = ./Makefile
include $(CS240LX_2023_PATH)/libpi/mk/Makefile.template-v2
