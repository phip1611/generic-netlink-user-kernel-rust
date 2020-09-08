# Generic netlink message transfer between Linux kernel module and user programm (written in Rust)

This is a stripped down example how to implement and use a simple protocol on top of the generic part of netlink.
In this example a string is passed from userland to a linux kernel module and echo'ed back.

_When using generic netlink it is best to think about implementing a protocol yourself on top of generic netlink. 
The protocol describes operations you want to trigger on the receiving side as well as payload you want to send._

**Linux kernel module needs at Linux kernel (5.x) to compile.**.

I had to figure this out for an uni project and it was quite tough. So I'd like to share my findings
with the open source world!

This repository consists of:
0) an `exmpl-protocol-nl.h`-file which describes common properties/operations the example protocol
1) linux kernel module (written in C) _(originally not my work; see below - but I adjusted some parts)_
2) userland program written in Rust _(my work)_
3) userland program written in C using raw sockets  _(originally not my work; see below - but I adjusted some parts)_
3) userland program written in C using [libnl](https://www.infradead.org/~tgr/libnl/) _(my work)_
4) shell script that builds and starts everything.

The userland programs in C are for just for comparison. The work for the linux kernel part and userland (3.)
program was originally published here: 
http://www.electronicsfaq.com/2014/02/generic-netlink-sockets-example-code.html

I don't know the author but from the comments I guess the author is *Anurag Chugh* ([Blogger](https://www.blogger.com/profile/15390575283968794206), [Website](http://www.lithiumhead.com/)). I want to give a big shoutout to him! 
**The C userland program (3) here is the same as in the link but the linux code was adjusted to work with Linux 5.4 (5.x)!**

## What it does
- the linux kernel module supports a single operation: receives a string, prints it to kernel log and send a string back (echo)
- the user program sends a string to the kernel via generic netlink and receives the returned message

## How does generic netlink work?
Although netlinks is one of the nicer interfaces for communicating between user and kernel in Linux it's still
quite tough. My understanding isn't perfect either. But my findings are:
1) linux kernel module registers a new family using generic netlink protocol:
   a new family number is temporarily assigned (additional to the families [here](https://github.com/torvalds/linux/blob/master/include/uapi/linux/netlink.h))
2) user program asks for the family number of %FAMILY_NAME% using generic netlink (handled by netlink manager inside Linux);
   receives number
3) the user program uses the family number and sends generic netlink messages to this number and receives data

## How to run
- `$ sudo apt install build-essential linux-headers-$(uname -r) libnl-3 libnl-genl-3` 
- make sure rustup/cargo is installed
- `$ sh ./build_and_run.sh`
```
rust user programm:
Generic family number is 32
Sending 'Hello from userland (Rust)' via netlink
Hello World from kernel space

c user programm:
extracted family id is: 32
Sent to kernel: Hello World from C user program!
Kernel replied: Hello World from kernel space

output of kernel log:
[ 1528.389775] Generic Netlink Example Module inserted.
[ 1528.421753] hello-world-nl: doc_exmpl_echo() invoked
[ 1528.421756] received: Hello from userland (Rust)
[ 1528.489141] hello-world-nl: doc_exmpl_echo() invoked
[ 1528.489142] received: Hello World from C user program!
```