#
# Makefile for Bosch sensor driver.
#
obj-$(CONFIG_SENSORS_BMA2X2)  += bstclass.o

obj-$(CONFIG_SENSORS_BMA2X2)    += bma2x2.o

ifeq ($(CONFIG_SENSORS_BMA2X2_ENABLE_INT1),y)
	EXTRA_CFLAGS += -DBMA2X2_ENABLE_INT1
endif

ifeq ($(CONFIG_BOSCH_BMA2X2_ENABLE_INT2),y)
	EXTRA_CFLAGS += -DBMA2X2_ENABLE_INT2
endif
