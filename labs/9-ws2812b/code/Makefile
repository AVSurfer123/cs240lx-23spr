# list out the driver program source 
PROGS = 2-neopix.c 1-blink.c  0-timing-check.c # simple.c
PROGS = 2-neopix.c 


# your source, shared by driver code.
#   if you want to use our staff-hc-sr04.o,
#   comment SRC out and uncomment STAFF_OBJS
COMMON_SRC := neopixel.c

OPT_LEVEL = -O3

RUN = 1

DEPS = ./Makefile

USE_FP = 1
LIBS += $(LIBM)

include $(CS240LX_2023_PATH)/libpi/mk/Makefile.template
