This is sample block device driver example compiled and tested
on 3.10 kernel.

This is based out of LWN article: https://lwn.net/Articles/58719/
which was based on 2.6 kernel.

Makefile:
obj-y	+= slos_bd.o

Commands on Terminal:
1. ls /dev/block/slos0

2. ./data/busybox/busybox fdisk /dev/block/slos0

3. Create partition
Command (m for help): n
Command action
   e   extended
   p   primary partition (1-4)
p
Partition number (1-4): 1
First cylinder (1-16, default 1):
Using default value 1
Last cylinder, +cylinders or +size{K,M,G} (1-16, default 16):
Using default value 16

Command (m for help): w
The partition table has been altered!

Calling ioctl() to re-read partition table.

4. ./data/busybox/busybox mkfs.vfat /dev/block/slos0p1

5. mount -t vfat /dev/block/slos0p1 /mnt

6. echo "slos sample block device" > /mnt/file

7. cat /mnt/file.

8. umount /mnt

-Ritesh <ritesh.list@gmail.com>
