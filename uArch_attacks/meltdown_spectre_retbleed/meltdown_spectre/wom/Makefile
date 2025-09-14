obj-m += wom.o
wom-objs += main.o
PWD := $(shell pwd)

KPATH :=/lib/modules/$(shell uname -r)/build

all: wom.ko

wom.ko:
	make -C $(KPATH) M=$(PWD) modules

clean:
	make -C $(KPATH) M=$(PWD) clean

insmod: all
	# (re-)enable
	-rmmod wom.ko
	insmod wom.ko

install: all
	# (re-)install globally and (re-)enable
	-rmmod wom
	make -C $(KPATH) M=$(PWD) modules_install
	insmod wom.ko
