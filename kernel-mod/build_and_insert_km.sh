# clear kernel log
sudo rmmod gnl_foobar_xmpl
sudo dmesg -c > /dev/null

make clean
make
sudo insmod gnl_foobar_xmpl.ko
echo "inserted gnl_foobar_xmpl.ko"
