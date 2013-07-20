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
echo -n " acpi,boot,dev,frame,libc,pci,vm" >> $stage/boot/grub/grub.cfg
gdbmode=0
[ "$1" = "gdb" ] && {
	echo -n ",gdbe" >> $stage/boot/grub/grub.cfg
  gdbmode=1
	shift 1
}
[ "$1" = "gdba" ] && {
	echo -n ",gdbe,gdba" >> $stage/boot/grub/grub.cfg
  gdbmode=1
	shift 1
}
echo >> $stage/boot/grub/grub.cfg
[ $# -gt 0 ] && cp $* $stage/boot && for i in $* ; do
	echo -n "  module2 /boot/" >> $stage/boot/grub/grub.cfg
	echo -n "$(basename $i)" >> $stage/boot/grub/grub.cfg
	echo " $(basename $i) sample args" >> $stage/boot/grub/grub.cfg
done
if [ $gdbmode -eq 1 ]; then
  cp kernel.sys.debug $stage/boot
  echo -n "  module2 /boot/" >> $stage/boot/grub/grub.cfg
  echo -n "kernel.sys.debug" >> $stage/boot/grub/grub.cfg
  echo " kernel.sys.debug sample args" >> $stage/boot/grub/grub.cfg
fi
echo "  boot" >> $stage/boot/grub/grub.cfg
echo "}" >> $stage/boot/grub/grub.cfg

if [ "$target" = "pxe" ]; then
	cp -a $stage/boot $TFTPDIR
elif [ "$target" = "rpxe" ]; then
	scp -rp $stage/boot $TFTPSERVER:$TFTPDIR
else
	$CROSSDIR/bin/$TARGET-grub-mkrescue -o $target $stage
fi
