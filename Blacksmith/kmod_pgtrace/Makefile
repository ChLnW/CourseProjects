mod-name := pgtrace
obj-m += $(mod-name).o
ccflags-y := -O1 -DDEBUG -std=gnu99 -Wall $(CCFLAGS)
PWD := $(shell pwd)
KDIR = /lib/modules/$(shell uname -r)/build

all:
	make -C $(KDIR) M=$(PWD) modules

clean:
	make -C $(KDIR) M=$(PWD) clean

install: all
	sudo rmmod $(mod-name) | true
	make -C $(KDIR) M=$(PWD) modules_install
	sudo insmod $(mod-name).ko

.PHONY: insert 
insert: all
	cp $(mod-name).ko /tmp/ && sudo insmod /tmp/$(mod-name).ko

.PHONY: remove
remove:
	sudo rmmod $(mod-name)
