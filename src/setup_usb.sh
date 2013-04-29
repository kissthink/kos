#!/bin/bash
#
# Written by Martin Karsten (mkarsten@uwaterloo.ca)
#
# also see http://wiki.osdev.org/Loopback_Device for other ideas
#
source $(dirname $0)/../config

[ $(id -u) -eq 0 ] || {
	echo -n "Login as root - "
	exec su root -c "$0 $*"
}

MNTDIR=$(mount|fgrep $1|cut -f3 -d' ')
[ -z "$MNTDIR" ] || {
	umount $MNTDIR || { echo "cannot unmount $MNTDIR"; exit 1; };
}

printf "o\nn\n\n\n\n\nt\nb\nw\n"|fdisk $1
mkfs.vfat ${1}1

TMPDIR=$(mktemp -d /tmp/mount.XXXXXXXXXX)
mount ${1}1 $TMPDIR

$CROSSDIR/sbin/$TARGET-grub-install --no-floppy --root-directory=$TMPDIR $1
cp kernel.sys $TMPDIR/boot
echo "\
set timeout=1
set default=0

menuentry "KOS" {
	multiboot2 /boot/kernel.sys boot,frame,pci
	boot
}" > $TMPDIR/boot/grub/grub.cfg

umount $TMPDIR
rmdir $TMPDIR

exit 0
