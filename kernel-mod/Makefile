# this builds "generic-netlink-demo-km.ko" from "generic-netlink-demo-km.c"
# that's the way to build linux kernel modules
obj-m = gnl_foobar_xmpl-km.o

# add our custom header file
EXTRA_CFLAGS=-I$(PWD)/../include/

all:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules
	
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
