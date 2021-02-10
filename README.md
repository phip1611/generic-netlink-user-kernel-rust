# Generic Netlink data transfer between Linux kernel module and user program (written in Rust)

This is a stripped down example about how to implement and use a simple protocol on top of the generic part of 
Netlink, an IPC channel in Linux kernel. When you use Generic Netlink it is best to imagine to implement a 
protocol yourself on top of Netlink. The protocol describes operations you want to trigger on the 
receiving side as well as payload to transfer. In this example a string is passed from userland to a Linux
kernel module and echo'ed back via Netlink.

This repository consists of a Linux Kernel Module (**developed with Linux 5.4**), that is written in C, and three 
**independent** userland components, that all act as standalone binaries and talk to the kernel module via 
Netlink.

The userland components are:
1) a Rust program using [neli](https://crates.io/crate/neli) as abstraction/library for (Generic) Netlink
2) a C program using [libnl](https://www.infradead.org/~tgr/libnl/) as abstraction/library for (Generic) Netlink
3) a pure C program using raw sockets and no library for abstraction _(originally not my work; see below - but I 
   adjusted some parts)_

*The kernel code and 3) are inspired by (I don't know the author but from the comments I guess) **Anurag Chugh** 
([Blogger](https://www.blogger.com/profile/15390575283968794206), [Website](http://www.lithiumhead.com/)).*

## Netlink vs Generic Netlink
Netlink knows about *families* and each family has an ID statically assigned in the Kernel code, 
see `<linux/netlink.h>`. Generic Netlink is one of these families and it can be used to create new families
on the fly/during runtime. A new family ID gets assigned temporarily for the name and with this ID you can
start to send or listen for messages on the given ID (in Kernel and in userland).

Generic Netlink (`<linux/genetlink.h>`) also helps you in the kernel code with some convenient functions for 
receiving and replying, and it offers `struct genlmsghdr`. The struct `struct genlmsghdr` offers the two 
important properties `cmd` and `version`. `cmd` is used to trigger an specific action on the receiving side 
whereas `version` can be used to cope with different versions (e.g. older code, newer versions of your app).

A ***Generic Netlink*** message is a message that has the following structure:
![Overview Generic Netlink message](Generic%20Netlink%20Message%20Overview.png "Overview Generic Netlink message")

So in fact, in this demo, we are transferring Generic Netlink messages, which are Netlink messages with a 
Generic Netlink header.

## Let's talk about the code

In this example we create a Netlink family on the fly called 
`gnl_foobar_xmpl` using Generic Netlink. The common Netlink properties shared by the C projects (Kernel and 
the two C userland components) are specified in `include/gnl_foobar_xmpl.h`. The very first step is the 
kernel module which needs to be loaded and register the Netlink family using Generic Netlink. Afterwards 
the userland components can talk to it.

## How to run
- this needs at least Linux 5.4
- `$ sudo apt install build-essential`
- `$ sudo apt install libnl-3 libnl-genl-3`
- `$ sudo apt install linux-headers-$(uname -r)` ()
- make sure rustup/cargo is installed
- `$ sh ./build_and_run.sh`

#### Example output
```
[User-Rust]: Generic family number is 34
[User-Rust]: Sending 'Some data that has `Nl` trait implemented, like &str' via netlink
[User-Rust]: Received from kernel: 'Some data that has `Nl` trait implemented, like &str'

[User-C-Pure] extracted family id is: 34
[User-C-Pure] Sent to kernel: Hello World from C user program (using raw sockets)!
[User-C-Pure] Kernel replied: Hello World from C user program (using raw sockets)!

[User-C-libnl] Family-ID of generic netlink family 'gnl_foobar_xmpl' is: 34
[User-C-libnl] Sent to kernel: 'Hello World from Userland with libnl & libnl-genl'
[User-C-libnl] Kernel replied: Hello World from Userland with libnl & libnl-genl

output of kernel log:
[10144.393103] Generic Netlink Example Module inserted.
[10144.461217] generic-netlink-demo-km: gnl_foobar_xmpl_cb_echo() invoked
[10144.461218] received: 'Some data that has `Nl` trait implemented, like &str'
[10144.542496] generic-netlink-demo-km: gnl_foobar_xmpl_cb_echo() invoked
[10144.542497] received: 'Hello World from C user program (using raw sockets)!'
[10144.543104] generic-netlink-demo-km: gnl_foobar_xmpl_cb_echo() invoked
[10144.543105] received: 'Hello World from Userland with libnl & libnl-genl'

```

## Trivia
I had to figure this out for an uni project and it was quite tough in the beginning, so I'd like to
share my findings with the open source world! Netlink documentation is not good (especially in the 
kernel) and perhaps not all my information are right. I just want to show what worked for me.
