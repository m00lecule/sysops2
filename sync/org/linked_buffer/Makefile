obj-m := linked_buffer.o

KDIR  := /lib/modules/$(shell uname -r)/build
PWD   := $(shell pwd)

all:
	make -C $(KDIR) M=$(PWD) modules

clean:
	make -C $(KDIR) M=$(PWD) clean

insert:
	insmod linked_buffer.ko

remove: 
	rmmod linked_buffer