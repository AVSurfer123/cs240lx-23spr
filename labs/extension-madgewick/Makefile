PROGS = read_imu.c test_stepper.c

COMMON_SRC = quaternion.c stepper.c

USE_FP=1

# define this if you need to give the device for your pi
TTYUSB = 
BOOTLOADER = my-install

# uncomment if you want it to automatically run.
RUN=0

LIBS += $(LIBM)

# LIB_POST = $(CS240LX_2023_PATH)/lib/libgcc.a
#LIB_POST := ./libgcc.a

include $(CS240LX_2023_PATH)/libpi/mk/Makefile.template-v2
