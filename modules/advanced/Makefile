obj-m += advanced.o

module = advanced

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

unload:
	rmmod $(module)	

test: clean all
	insmod $(module).ko
	
