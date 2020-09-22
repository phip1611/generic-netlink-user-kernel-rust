cd kernel-mod
sh build_and_insert_km.sh
cd ..

# rust user program:
# should communicate with kernel module via netlink 
cd user-rust
echo
echo "rust user programm:"
cargo run
cd ..

# c user program:
# should communicate with kernel module via netlink 
cd user-c
echo
make clean
make

echo
echo
echo
echo "c user programm (using raw sockets):"
./user-pure
echo
echo
echo "c user programm (using libnl):"
./user-libnl
cd ..

echo
echo
echo
echo "output of kernel log:"
sudo dmesg

# unload it at the end
# sudo rmmod generic-netlink-demo-km
