insmod ./lkm_example.ko
mknod /dev/lkm_example c 249 0
#./test1 &
echo 1 > /dev/lkm_example