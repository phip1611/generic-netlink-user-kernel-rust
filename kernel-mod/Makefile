# This Makefile builds "gnl_foobar_xmpl.ko" from "gnl_foobar_xmpl.c".
# That's the way to build Linux kernel modules (out of tree).

obj-m = gnl_foobar_xmpl.o

# add our custom header file
EXTRA_CFLAGS=-I$(PWD)/../include/

all:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules
	
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
