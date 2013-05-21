#!/bin/bash
#
# Written by Martin Karsten (mkarsten@uwaterloo.ca)
#
# also see http://wiki.osdev.org/Loopback_Device for other ideas
#
source $(dirname $0)/../config

[ $# -lt 2 ] && {
	echo "usage: $0 <stagedir> <target> <kernel binary> [gdb] [<module>] ..."
	echo "<target>: specify pxe or .iso file name"
	exit 1
}
stage=$1
target=$2
kernel=$3
shift 3
mkdir -p $stage/boot/grub
cp $kernel $stage/boot
echo "set timeout=1" >> $stage/boot/grub/grub.cfg
echo "set default=0" >> $stage/boot/grub/grub.cfg
#echo "set debug=all" >> $stage/boot/grub/grub.cfg
echo >> $stage/boot/grub/grub.cfg
echo 'menuentry "KOS" {' >> $stage/boot/grub/grub.cfg
echo -n "  multiboot2 /boot/$kernel" >> $stage/boot/grub/grub.cfg
echo -n " acpi,boot,dev,frame,libc,lgdb,pci,scheduler,vm" >> $stage/boot/grub/grub.cfg
[ "$1" = "gdb" ] && {
	echo -n ",gdb" >> $stage/boot/grub/grub.cfg
	shift 1
}
[ "$1" = "allstopgdb" ] && {
	echo -n ",gdb,allstopgdb" >> $stage/boot/grub/grub.cfg
	shift 1
}
echo >> $stage/boot/grub/grub.cfg
[ $# -gt 0 ] && cp $* $stage/boot && for i in $* ; do
	echo -n "  module2 /boot/" >> $stage/boot/grub/grub.cfg
	echo -n "$(basename $i)" >> $stage/boot/grub/grub.cfg
	echo " $(basename $i) sample args" >> $stage/boot/grub/grub.cfg
done
echo "  boot" >> $stage/boot/grub/grub.cfg
echo "}" >> $stage/boot/grub/grub.cfg

if [ "$target" = "pxe" ]; then
	cp -a $stage/boot $TFTPDIR
elif [ "$target" = "rpxe" ]; then
	scp -rp $stage/boot $TFTPSERVER:$TFTPDIR
else
	$CROSSDIR/bin/$TARGET-grub-mkrescue -o $target $stage
fi
