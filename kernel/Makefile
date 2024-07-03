obj-m += Lian_li_hub.o

PWD := $(CURDIR)

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

install:
	make -C /lib/modules/$(shell uname -r)/build M=/home/emil/Programming/c/Ll-connect/kernel modules_install

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
