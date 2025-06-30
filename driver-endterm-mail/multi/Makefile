obj-m := mailbox_driver.o


KERNEL_DIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	make -C $(KERNEL_DIR) M=$(PWD) modules

# Clean target
clean:
	make -C $(KERNEL_DIR) M=$(PWD) clean

# Install module
install:
	sudo insmod mailbox_driver.ko
	sudo chmod 666 /dev/mailbox0
	sudo chmod 666 /dev/mailbox1

# Remove module
remove:
	sudo rmmod mailbox_driver

# Show device info
info:
	ls -la /dev/mailbox*
	lsmod | grep mailbox
reload:
	sudo rmmod mailbox_driver || true
	make clean
	make
	sudo /usr/src/linux-headers-$(shell uname -r)/scripts/sign-file sha256 ~/Downloads/VMWARE17.priv ~/Downloads/VMWARE17.der ./mailbox_driver.ko
	sudo insmod mailbox_driver.ko
	sudo chmod 666 /dev/mailbox0
	sudo chmod 666 /dev/mailbox1

.PHONY: all clean install remove info