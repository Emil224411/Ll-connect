VERSION = 0.0.1
SOURCE = /usr/src/Lian_Li_UNI_HUB_ALv2-$(VERSION)
#obj-m += Lian_Li_UNI_HUB_ALv2.o
#make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules_install
#make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
PWD := $(CURDIR)

all:
	echo "type \"make modules\" to compile modules\ntype \"make install\" to install the driver"
module:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

install:
	mkdir -p $(SOURCE)
	cp $(PWD)/Lian_Li_UNI_HUB_ALv2.c $(SOURCE)
	cp $(PWD)/Kbuild $(SOURCE)
	cp $(PWD)/dkms.conf $(SOURCE)
	cp $(PWD)/Makefile $(SOURCE)
	dkms install -m Lian_Li_UNI_HUB_ALv2 -v $(VERSION)

clean:
	modprobe -r Lian_Li_UNI_HUB_ALv2
	dkms remove -m Lian_Li_UNI_HUB_ALv2 -v $(VERSION) --all
	rm -r $(SOURCE)
