# Generic netlink message transfer between Linux kernel module and user programm (written in Rust)

This is a stripped down example how to use the generic protocol inside the netlink protocol to 
pass data between userland (written in Rust) and Linux kernel module (written in C).

I had to figure this out for an uni project and it was quite tough. So I'd like to share my findings
with the open source world!

This repository consists of:
1) linux kernel module (written in C)
2) userland program (written in Rust)
3) userland program (written in C)
4) shell script that builds and starts everything.

The userland program in C is just for comparision. The work for the linux kernel part and the userland
program in C was originally published here: http://www.electronicsfaq.com/2014/02/generic-netlink-sockets-example-code.html
I don't know the name of the author but I want to give a big shoutout to him/her!
**The user program (C) is the same but the linux code was adjusted to work with Linux 5.4 (5.x)!**

## What it does
- the linux kernel module supports a single operation: receives a string, prints it to kernel log and send a string back
- the user program sends a string to the kernel via netlink and receives the returned message

## How does generic netlink work?
Although netlinks is one of the nicer interfaces for communicating between user and kernel in Linux it's still
quite tough. My understanding isn't perfect either. But my findings are:
1) linux kernel module registers a new family using generic netlink protocol:
   a new number family is temporarily assigned
2) user program asks for the family number using generic netlink (handled by netlink manager inside Linux);
   receives number
3) the user program uses the family number and sends generic netlink messages to this number

## How to run
- `$ sudo apt install build-essential linux-headers-$(uname -r)` 
- make sure rustup/cargo is installed
- `$ sh ./build_and_run`
```
rust user programm:
Generic family number is 32
Sending 'Hello from userland (Rust)' via netlink
Hello World from kernel space

c user programm:
rm -rf user
gcc -Wall -Werror -o user user.c
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