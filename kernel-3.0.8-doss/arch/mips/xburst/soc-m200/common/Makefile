## WARNING! DO NOT change this lists order because power sleep order depends on it!! ##

obj-y += setup.o
obj-y += reset.o
obj-y += timer.o

obj-y += serial.o
obj-y += tcu.o

obj-y += gpio.o
obj-y += ddr.o
# only for debug
#obj-y += pmmon.o
obj-y += irq.o

obj-y += platform.o

obj-y += clk/

obj-y += pm_p0.o

obj-y += libdmmu.o

obj-$(CONFIG_USB_SUPPORT) += cpm_usb.o

obj-$(CONFIG_CPU_FREQ) += cpufreq.o

obj-y += jz_big_little_switch.o cpu_switch.o cpu_core_voltage.o
obj-$(SW-DEBUG) +=  cpu_switch_debug.o

obj-$(CONFIG_DWC_LM_INTERFACE)  += lm.o lm-device.o
