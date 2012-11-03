#!/bin/bash
#
# Written by Martin Karsten (mkarsten@uwaterloo.ca)
#
# also see http://wiki.osdev.org/Loopback_Device for other ideas
#
source $(dirname $0)/config.sh

[ $# -lt 2 ] && {
	echo "usage: $0 <target-iso> <kernel binary> [<module>] ..."
	exit 1
}
target=$1
kernel=$2
shift 2
mkdir -p iso/boot/grub
cp $kernel iso/boot
[ $# -gt 0 ] && cp $* iso/boot
echo "set timeout=0" >> iso/boot/grub/grub.cfg
echo "set default=0" >> iso/boot/grub/grub.cfg
echo >> iso/boot/grub/grub.cfg
echo 'menuentry "KOS" {' >> iso/boot/grub/grub.cfg
echo -n "  multiboot2 /boot/$kernel" >> iso/boot/grub/grub.cfg
echo -n " boot,frame,pci,pag" >> iso/boot/grub/grub.cfg
echo >> iso/boot/grub/grub.cfg
[ $# -gt 0 ] && for i in $* ; do
	echo -n "  module2 /boot/" >> iso/boot/grub/grub.cfg
	basename $i >> iso/boot/grub/grub.cfg
done
echo "  boot" >> iso/boot/grub/grub.cfg
echo "}" >> iso/boot/grub/grub.cfg
$PREFIX/bin/x86_64-pc-kos-grub-mkrescue -o $target iso
