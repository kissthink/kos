#!/bin/bash
#
# Written by Martin Karsten (mkarsten@uwaterloo.ca)
#
# also see http://wiki.osdev.org/Loopback_Device for other ideas
#
source $(dirname $0)/../config

[ $# -lt 2 ] && {
	echo "usage: $0 <target-iso> <kernel binary> [gdb] [<module>] ..."
	exit 1
}
target=$1
kernel=$2
shift 2
mkdir -p iso/boot/grub
cp $kernel iso/boot
echo "set timeout=1" >> iso/boot/grub/grub.cfg
echo "set default=0" >> iso/boot/grub/grub.cfg
#echo "set debug=all" >> iso/boot/grub/grub.cfg
echo >> iso/boot/grub/grub.cfg
echo 'menuentry "KOS" {' >> iso/boot/grub/grub.cfg
echo -n "  multiboot2 /boot/$kernel" >> iso/boot/grub/grub.cfg
echo -n " acpi,boot,dev,frame,libc,pci,vm" >> iso/boot/grub/grub.cfg
[ "$1" = "gdb" ] && {
	echo -n ",gdb" >> iso/boot/grub/grub.cfg
	shift 1
}
echo >> iso/boot/grub/grub.cfg
[ $# -gt 0 ] && cp $* iso/boot && for i in $* ; do
	echo -n "  module2 /boot/" >> iso/boot/grub/grub.cfg
	echo -n "$(basename $i)" >> iso/boot/grub/grub.cfg
	echo " $(basename $i) sample args" >> iso/boot/grub/grub.cfg
done
echo "  boot" >> iso/boot/grub/grub.cfg
echo "}" >> iso/boot/grub/grub.cfg

if [ "$target" = "pxe" -o "$target" = "fpxe" ]; then
	if [ "$target" = "fpxe" ]; then
		# full PXE: need pxelinux.0 pxelinux.cfg/default vesamenu.c32 in $TFTPDIR
		rm -rf $TFTPDIR/boot
		$CROSSDIR/sbin/$TARGET-grub-mknetdir --net-directory=iso
	fi
	cp -a iso/boot $TFTPDIR
elif [ "$target" = "rpxe" ]; then
	scp -rp iso/boot $TFTPSERVER:$TFTPDIR
else
	$CROSSDIR/bin/$TARGET-grub-mkrescue -o $target iso
fi
