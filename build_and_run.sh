# clear kernel log
sudo dmesg -c > /dev/null

cd kernel-mod
make clean
make
sudo rmmod hello-world-nl
sudo insmod hello-world-nl.ko
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
echo "c user programm:"
make clean
make
./user
cd ..

echo
echo "output of kernel log:"
sudo dmesg

# unload it at the end
# sudo rmmod hello-world-nl
