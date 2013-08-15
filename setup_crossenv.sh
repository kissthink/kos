#!/bin/bash

# see 'config' for installation directories:
# . grub in $GRUBDIR
# . gcc/binutils in $GCCDIR
# . gdb, bochs, and qemu in $TOOLSDIR
# note: use 'make info' and 'make pdf' to build GNU docs

cpucount=$(fgrep processor /proc/cpuinfo|wc -l)

TMPDIR=/spare/tmp
cd $(dirname $0)
DLDIR=$(pwd)/download
PTDIR=$(pwd)/patches
source $(pwd)/config
cd -

BINUTILS=binutils-2.23.2 # GNU mirror
BOCHS=bochs-2.6.2        # http://bochs.sourceforge.net/
GCC=gcc-$GCCVER          # GNU mirror
GDB=gdb-7.6              # GNU mirror
GRUB=grub-2.00           # GNU mirror
NEWLIB=newlib-2.0.0      # http://sourceware.org/newlib/
QEMU=qemu-1.5.1          # http://www.qemu.org/

# download FreeBSD's src.txz from local mirror, or original URL:
# http://ftp.freebsd.org/pub/FreeBSD/releases/amd64/9.1-RELEASE/src.txz

mkdir -p $TMPDIR

uname -s|fgrep -qi CYGWIN && ROOTEXEC=bash || ROOTEXEC=su

function error() {
	echo FATAL ERROR: $1
	exit 1
}

function unpack() {
	rm -rf $TMPDIR/$1 && cd $TMPDIR || error "$TMPDIR access"
	tar xaf $DLDIR/$1.$2 || error "$1 extract"
}

function applypatch() {
	dir=$TMPDIR/$1
	[ -f $PTDIR/$1.patch ] && {
		patch -d $dir -p1 < $PTDIR/$1.patch || error "$1 patch"
	}
	[ -x $PTDIR/$1.sh ] && {
		$PTDIR/$1.sh $dir || error "$1 shell patch"
	}
}

function prebuild() {
	dir=$TMPDIR/$1-build
	rm -rf $dir && mkdir $dir && cd $dir || error "$dir access"
}

function build() {
	cd $TMPDIR/$1-build
	nice -10 make -j $cpucount all || error "$1 build failed"
}

function install() {
	echo -n "Installing $1 - "
	$ROOTEXEC -c "make -C $TMPDIR/$1-build install && echo SUCCESS: $1 install"
}

function build_gcc() {
	rm -rf $TMPDIR/$GCC && mkdir $TMPDIR/$GCC && cd $TMPDIR/$GCC || error "$TMPDIR/$GCC access"
	# order often important (gcc last)
	tar xaf $DLDIR/$BINUTILS.tar.bz2 --strip-components 1 || error "$BINUTILS extract"
	tar xaf $DLDIR/$NEWLIB.tar.gz --strip-components 1 || error "$NEWLIB extract"
	tar xaf $DLDIR/$GCC.tar.bz2 --strip-components 1 || error "$GCC extract"
	sh $PTDIR/$BINUTILS.sh $TMPDIR/$GCC || error "$BINUTILS shell path"
	sh $PTDIR/$NEWLIB.sh $TMPDIR/$GCC || error "$NEWLIB shell path"
	applypatch $GCC
	prebuild $GCC
	../$GCC/configure --target=$TARGET --prefix=$GCCDIR\
		--enable-languages=c,c++ --enable-lto --disable-nls --disable-tls\
		--with-newlib || error "$GCC configure"
	# compile gcc first pass
	build $GCC
	cd $TMPDIR/$GCC-build/$TARGET
	# recompile newlib with -mcmodel=kernel
	sed -i 's/CFLAGS = -g -O2/CFLAGS = -g -O2 -mcmodel=kernel/' newlib/Makefile
	make -C newlib clean
	nice -10 make -C newlib -j $cpucount all || error "newlib recompile failed"
	# recompile libstdc++-v3 with -mcmodel=kernel
	sed -i 's/CFLAGS = -g -O2/CFLAGS = -g -O2 -mcmodel=kernel/' libstdc++-v3/Makefile
	sed -i 's/CXXFLAGS = -g -O2/CXXFLAGS = -g -O2 -mcmodel=kernel/' libstdc++-v3/Makefile
	make -C libstdc++-v3 clean
	nice -10 make -C libstdc++-v3 -j $cpucount all || error "libstdc++-v3 recompile failed"
	# recompile gcc with new library settings
	build $GCC
}

function build_gdb() {
	unpack $GDB tar.bz2
	applypatch $GDB
	prebuild $GDB
	../$GDB/configure --target=$TARGET --prefix=$TOOLSDIR || error "$GDB configure"
	build $GDB
}

function build_grub() {
	unpack $GRUB tar.xz
	applypatch $GRUB
	prebuild $GRUB
	../$GRUB/configure --target=$TARGET --prefix=$GRUBDIR --disable-werror\
	--disable-device-mapper || error "$GRUB configure"
	build $GRUB
}

function build_bochs() {
	unpack $BOCHS tar.gz
	applypatch $BOCHS
	prebuild $BOCHS
	../$BOCHS/configure --enable-smp --enable-cpu-level=6 --enable-x86-64\
		--enable-all-optimizations --enable-pci --enable-vmx --enable-debugger\
		--enable-disasm --enable-debugger-gui --enable-logging --enable-fpu\
		--enable-iodebug --enable-sb16=dummy --enable-cdrom --enable-x86-debugger\
		--disable-plugins --disable-docbook --with-x --with-x11 --with-term\
		--prefix=$TOOLSDIR || error "$BOCHS configure"
	sed -i 's/LIBS =  -lm/LIBS = -pthread -lm/' Makefile
	build $BOCHS
}

function build_qemu() {
	unpack $QEMU tar.bz2
	applypatch $QEMU
	prebuild $QEMU
	../$QEMU/configure --target-list=x86_64-softmmu --prefix=$TOOLSDIR\
	--python=/usr/bin/python2 --enable-sdl || error "$QEMU configure"
	build $QEMU
}

while [ $# -gt 0 ]; do case "$1" in
bochs)
	build_bochs && install $BOCHS;;
gcc)
	build_gcc && install $GCC;;
gdb)
	build_gdb && install $GDB;;
grub)
	build_grub && install $GRUB;;
qemu)
	build_qemu && install $QEMU;;
allcross)
	build_gcc && install $GCC
	build_gdb && install $GDB
	build_grub && install $GRUB;;
*)
	echo unknown argument: $1;;
esac; shift; done
