obj-m += reverse.o

module = reverse

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

unload:
	rmmod $(module)	

test: clean all
	insmod $(module).ko
	echo "michal dygas dygas" > /dev/$(module)
	echo '5' > /proc/$(module)
	cat /dev/$(module)
	echo 'mich' > /dev/$(module)
	cat /dev/$(module)

