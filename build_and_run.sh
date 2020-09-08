# clear kernel log
sudo rmmod hello-world-nl
sudo dmesg -c > /dev/null

cd kernel-mod
make clean
make
sudo insmod hello-world-nl.ko
echo "inserted hello-world-nl.ko"
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

echo "c user programm (using raw sockets):"
./user-pure
echo "c user programm (using libnl):"
./user-libnl
cd ..

echo
echo "output of kernel log:"
sudo dmesg

# unload it at the end
# sudo rmmod hello-world-nl
