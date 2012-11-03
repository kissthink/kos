#!/bin/bash
#
# Written by Martin Karsten (mkarsten@uwaterloo.ca)
#
# also see http://wiki.osdev.org/Loopback_Device for other ideas
#
source $(dirname $0)/config.sh

[ $(id -u) -eq 0 ] || {
	echo -n "Login as root - "
	exec su -l root -c "$0 $*"
}

TMPDIR=$(mktemp -d /tmp/mount.XXXXXXXXXX)
MNTDIR=$(mount|fgrep $1|cut -f3 -d' ')
mounted=0
[ -z "$MNTDIR" ] && {
	MNTDIR=$TMPDIR
	mount ${1}1 $MNTDIR
	mounted=1
}	

$PREFIX/sbin/x86_64-pc-kos-grub-install --no-floppy\
	--root-directory=$MNTDIR $1

cp kernel.sys $MNTDIR/x86_64-pc-kos-boot
echo "\
set timeout=1
set default=0

menuentry "KOS" {
	multiboot2 /x86_64-pc-kos-boot/kernel.sys
	boot
}" > $MNTDIR/x86_64-pc-kos-boot/x86_64-pc-kos-grub/grub.cfg

[ $mounted -eq 1 ] && umount $MNTDIR
rmdir $TMPDIR

exit 0
