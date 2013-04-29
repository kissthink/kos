#!/bin/bash
#
# Written by Martin Karsten (mkarsten@uwaterloo.ca)
#
# also see http://wiki.osdev.org/Loopback_Device for other ideas
#
source $(dirname $0)/../config

[ $(id -u) -eq 0 ] || {
	echo -n "Login as root - "
	exec su -l root -c "$0 $*"
}

LOOPDEV0=/dev/loop0
LOOPDEV1=/dev/loop1

/sbin/modprobe loop

for ld in $LOOPDEV1 $LOOPDEV0; do
	for mnt in $(mount|fgrep $ld|cut -f3 -d' '); do
		umount -d $mnt || exit 1
		rmdir $mnt
	done
	losetup -a|fgrep -q $ld && losetup -d $ld
done

[ "$2" = "clean" ] && exit 0

MNTDIR=$(mktemp -d /tmp/osimg.XXXXXXXXXX)

# create partition table with one active, bootable FAT16 partition
# printf "n\np\n\n\n\na\n1\nt\n6\nw\n" | fdisk $1 >/dev/null 2>&1
printf "o\nn\n\n\n\n\na\n1\nw\n" | fdisk -u -C64 -S63 -H16 $1 >/dev/null 2>&1

# compute byte offset of partition
start=$(fdisk -l $1|tail -1|awk '{print $3}')
size=$(fdisk -l $1|fgrep "Sector size"|awk '{print $7}')
offset=$(expr $start \* $size)

# set up loop devices for disk and partition
losetup $LOOPDEV0 $1
losetup -o $offset $LOOPDEV1 $LOOPDEV0

# create filesystem on partition
mkfs -t ext2 $LOOPDEV1 >/dev/null 2>&1

# mount filesystem
mount $LOOPDEV1 $MNTDIR

mkdir -p $MNTDIR/$TARGET-boot/$TARGET-grub
cat > $MNTDIR/$TARGET-boot/$TARGET-grub/device.map <<EOF
(hd0)   /dev/loop0
(hd0,1) /dev/loop1
EOF

# install grub
$CROSSDIR/sbin/$TARGET-grub-install --no-floppy\
	--grub-mkdevicemap=$MNTDIR/$TARGET-boot/$TARGET-grub/device.map\
	--root-directory=$MNTDIR $LOOPDEV0

# copy kernel binary and config file
cp kernel.sys $MNTDIR/$TARGET-boot
cat > $MNTDIR/$TARGET-boot/$TARGET-grub/grub.cfg <<EOF
set timeout=0
set default=0

menuentry "KOS" {
	multiboot2 /$TARGET-boot/kernel.sys
	boot
}
EOF

umount $MNTDIR
# repeat until losetup completes - might fail right after umount
while ! losetup -d $LOOPDEV1; do sleep 1; umount $MNTDIR; done
rmdir $MNTDIR
while ! losetup -d $LOOPDEV0; do sleep 1; done

exit 0
