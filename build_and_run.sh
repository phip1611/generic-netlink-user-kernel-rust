#!/bin/sh

cd kernel-mod
sh build_and_insert_km.sh
cd ..

# rust user program:
# should communicate with kernel module via netlink 
cd user-rust
# main binary
cargo run --bin echo
cd ..

# c user program:
# should communicate with kernel module via netlink 
cd user-c
echo
make clean
make

echo
./user-pure
echo
./user-libnl
cd ..

echo
echo "output of kernel log:"
sudo dmesg

echo "----------------------- more examples"
cd user-rust
# main binary
cargo run --bin echo_with_dump_flag
cargo run --bin reply_with_error
cd ..

# unload it at the end
# sudo rmmod gnl_foobar_xmpl
