# clear kernel log
sudo rmmod generic-netlink-demo-km
sudo dmesg -c > /dev/null

make clean
make
sudo insmod generic-netlink-demo-km.ko
echo "inserted generic-netlink-demo-km.ko"
