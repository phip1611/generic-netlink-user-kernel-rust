# this builds "hello-world-nl.ko" from "hello-world-nl.c"
# that's the way to build linux kernel modules
obj-m = hello-world-nl.o 

# add our custom header file
EXTRA_CFLAGS=-I$(PWD)/../include/

all:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules
	
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
