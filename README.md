# Generic Netlink data transfer between userland (C & Rust) and Linux kernel using a custom Netlink family

This is a stripped down example about how to implement and use a simple protocol on top of the generic part of 
Netlink, an IPC channel in Linux kernel. I'm using a custom Netlink family that I create with Generic Netlink 
during runtime with a custom Linux kernel module/driver.

When you use Generic Netlink it is best to imagine to implement a 
protocol yourself on top of Netlink. The protocol describes operations you want to trigger on the 
receiving side as well as payload to transfer. In this example a string is passed from userland to a Linux
kernel module and echo'ed back via Netlink.

This repository consists of a Linux Kernel Module (**developed with Linux 5.4**), that is written in C, and three 
**independent** userland components, that all act as standalone binaries and talk to the kernel module via 
Netlink. This tutorial covers Netlink topics like **validation**, **error reporting** (`NMLSG_ERROR` responses),
dump calls (with `NMLSG_DONE` responses) as well as **regular data transfer**. The code is well and intensively 
documented, especially the kernel module. You can use this code as a starter template for own kernel modules with 
custom Generic Netlink families.

The (standalone, independent) userland components are:
1) a Rust program using [neli](https://crates.io/crate/neli) as abstraction/library for (Generic) Netlink
2) a C program using [libnl](https://www.infradead.org/~tgr/libnl/) as abstraction/library for (Generic) Netlink
3) a pure C program using raw sockets and no library for abstraction _(originally not my work; see below - but I 
   adjusted some parts)_

*The pure C program (3) code is inspired of the code on [electronicsfaq.com](http://www.electronicsfaq.com/2014/02/generic-netlink-sockets-example-code.html)
by (I don't know the author but from the comments I guess) **Anurag Chugh** 
([Blogger](https://www.blogger.com/profile/15390575283968794206), [Website](http://www.lithiumhead.com/)).*

## TL;DR; Where To Start?
Check out the code in `kernel-mod/gnl_foobar_xmpl.c`. Build it and load the kernel module. Afterwards you can run 
several userland components from this repository against it. 
* `$ cargo run --bin echo`
* `$ cargo run --bin echo_with_dump_flag`
* `$ cargo run --bin reply_with_error`

## Netlink vs Generic Netlink
Netlink knows about *families* and each family has an ID statically assigned in the Kernel code, 
see `<linux/netlink.h>`. Generic Netlink is one of these families and it can be used to create new families
on the fly/during runtime. A new family ID gets assigned temporarily for the name and with this ID you can
start to send or listen for messages on the given ID (in Kernel and in userland). Therefore the ID is used for adressing packets in Netlink.

Generic Netlink (`<linux/genetlink.h>`) also helps you in the kernel code with some convenient functions for 
receiving and replying, and it offers `struct genlmsghdr`. The struct `struct genlmsghdr` offers the two 
important properties `cmd` and `version`. `cmd` is used to trigger a specific action on the receiving side 
whereas `version` can be used to cope with different versions (e.g. older code, newer versions of your app).

A ***Generic Netlink*** message is a message that has the following structure:
![Overview Generic Netlink message](Generic%20Netlink%20Message%20Overview.png "Overview Generic Netlink message. I use operation and command as synonyms.")

(I use "Operation" and "Command" as synonyms.) In fact, in this demo, we are transferring Generic Netlink messages, 
which are Netlink messages with a Generic Netlink header.

## Let's talk about the code

In this example we create a Netlink family on the fly called 
`gnl_foobar_xmpl` using Generic Netlink. The common Netlink properties shared by the C projects (Kernel and 
the two C userland components) are specified in `include/gnl_foobar_xmpl.h`. The very first step is the 
kernel module which needs to be loaded and register the Netlink family using Generic Netlink. Afterwards 
the userland components can talk to it.

## How to run
- this needs at least Linux 5.*
- `$ sudo apt install build-essential`
- `$ sudo apt install libnl-3 libnl-genl-3`: for C example with `libnl`
- `$ sudo apt install linux-headers-$(uname -r)`: useful only for easier Kernel Module development; Clion IDE can find headers
- make sure rustup/cargo is installed: for Rust userland component
- `$ sh ./build_and_run.sh`: builds and starts everything. Creates an output as shown below.)

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

## Measurement and comparison the userland components (abstractions cost)
I measured the average time in all three userland components for establishing a Netlink connection,
building the message (payload), sending the ECHO request to the kernel, and receiving the reply all together.
I ran every user component in release mode (optimized) and took the measurements inside the code. These are the
average durations in microseconds. I did 100,000 iterations for each measurement.

| User Rust (`neli`) | User C (Pure) | User C (`libnl`) |
|------------------|---------------|----------------|
|             12µs |           8µs |           13µs |

Abstractions cost us a little bit of time :) Using strace we can find that the Rust program and C (with `libnl`) 
doing much more system calls. Before I measured this, I removed the loop (100,000 iterations, as mentioned above)
again and just did a single run. I executed the following statements which results in the table shown below.

- `$ strace user-rust/target/release/user-rust 2>&1 >/dev/null | wc -l`
- `$ strace ./user-pure 2>&1 >/dev/null | wc -l`
- `$ strace ./user-libnl 2>&1 >/dev/null | wc -l` \


| User Rust (`neli`) | User C (Pure) | User C (`libnl`) |
|--------------------|---------------|----------------|
|       108 syscalls |   45 syscalls |   89 syscalls |

Look into the [measurements/](https://github.com/phip1611/generic-netlink-user-kernel-rust/tree/main/measurements) 
directory of this repository, you can find the traces there. I didn't dived in deeper but you can clearly see that 
`libnl` and `neli` results in a lot more syscalls which explains the slower result.

## Trivia
I had to figure this out for an uni project and it was quite tough in the beginning, so I'd like to
share my findings with the open source world! Netlink documentation and tutorial across the web are not good
and not centralized, you have to look up many things by yourself ad different sources. Perhaps not all my 
information are 100% right, but I want to share what works for me. I hope I can help you :)
If so, let me know on Twitter ([@phip1611](https://twitter.com/phip1611)).

## Additional resources
- [Generic Netlink HOW-TO based on Jamal's original doc](https://lwn.net/Articles/208755/)
