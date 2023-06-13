# list out the driver program source

PROGS = stepper-test.c national_anthem.c
COMMON_SRC := stepper.c stepper-int.c

# use floating point.
USE_FP=1

LIB_POST := $(CS240LX_2023_PATH)/lib/libgcc.a
LIBS += $(LIB_POST)
LIBS += $(LIBM)

RUN = 0

DEPS = ./Makefile
include $(CS240LX_2023_PATH)/libpi/mk/Makefile.template-v2
