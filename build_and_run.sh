cd kernel-mod
sh build_and_insert_km.sh
cd ..

# rust user program:
# should communicate with kernel module via netlink 
cd user-rust
cargo run
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

# unload it at the end
sudo rmmod gnl_foobar_xmpl-km
