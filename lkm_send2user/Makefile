LKM = lkm_example
PWD := $(shell pwd)

obj-m += $(LKM).o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean


unload:
	sudo rmmod $(LKM).ko || true

load: unload all
	sudo insmod $(LKM).ko
	dmesg | tail