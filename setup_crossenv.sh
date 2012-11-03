#!/bin/bash

# note: use 'make info' and 'make pdf' to build GNU docs

TMPDIR=/spare/tmp
DLDIR=/home/sukwon0709/git/kos/download
PTDIR=/home/sukwon0709/git/kos/patches
PREFIX=/usr/local/cross
TARGET=x86_64-pc-kos

BINUTILS=binutils-2.22
NEWLIB=newlib-1.20.0
GCC=gcc-4.7.2
GDB=gdb-7.5
GRUB=grub-2.00
BOCHS=bochs-2.6
QEMU=qemu-1.2.0

mkdir -p $TMPDIR

function error() {
	echo FATAL ERROR: $1
	exit 1
}

function build_gcc() {
	rm -rf $TMPDIR/$GCC && mkdir $TMPDIR/$GCC && cd $TMPDIR/$GCC || error "$TMPDIR/$GCC access"
	# this order of extraction (gcc after binutils/newlib ) is important!!
	# otherwise error: 'DEMANGLE_COMPONENT_TRANSACTION_CLONE' undeclared
	tar xjf $DLDIR/$BINUTILS.tar.bz2 --strip-components 1 || error "$BINUTILS extract"
	tar xzf $DLDIR/$NEWLIB.tar.gz --strip-components 1 || error "$NEWLIB extract"
	tar xjf $DLDIR/$GCC.tar.bz2 --strip-components 1 || error "$GCC extract"
	patch -p1 < $PTDIR/gcc.small.patch || error "$GCC patch"
	ln -s elfos.h gcc/config/kos.h
	rm -rf $TMPDIR/build-gcc && mkdir $TMPDIR/build-gcc && cd $TMPDIR/build-gcc || error "$TMPDIR/build-gcc access"
	../$GCC/configure --target=$TARGET --prefix=$PREFIX\
		--disable-threads --disable-tls --disable-nls\
		--enable-languages=c,c++ --with-newlib || error "$GCC configure"
	nice -10 make -j $(fgrep processor /proc/cpuinfo|wc -l) all &&\
		echo SUCCESS: crossgcc build
	# rebuild newlib and libstdc++ with mcmodel=kernel
	cd $TMPDIR/build-gcc/$TARGET/newlib
	sed -e "/CFLAGS = -g -O2/s//CFLAGS = -g -O2 -mcmodel=kernel/" -i Makefile
	make clean; nice -10 make -j $(fgrep processor /proc/cpuinfo|wc -l) all &&\
		echo SUCCESS: newlib rebuild
	cd $TMPDIR/build-gcc/$TARGET/libstdc++-v3
	sed -e "/CFLAGS = -g -O2/s//CFLAGS = -g -O2 -mcmodel=kernel/" -i Makefile
	sed -e "/CXXFLAGS = -g -O2/s//CXXFLAGS = -g -O2 -mcmodel=kernel/" -i Makefile
	make clean; nice -10 make -j $(fgrep processor /proc/cpuinfo|wc -l) all &&\
		echo SUCCESS: libstdc++ rebuild
	# rebuild gcc with new version of libraries (just to be safe)
	cd $TMPDIR/build-gcc
	nice -10 make -j $(fgrep processor /proc/cpuinfo|wc -l) all &&\
		echo SUCCESS: crossgcc rebuild
}

function install_gcc() {
	su -c "make -C $TMPDIR/build-gcc install && echo SUCCESS: gcc install"
}

function build_gdb() {
	rm -rf $TMPDIR/$GDB && cd $TMPDIR || error "$TMPDIR access"
	tar xjf $DLDIR/$GDB.tar.bz2 || error "$GDB extract"
	cd $TMPDIR/$GDB
	patch -p1 < $PTDIR/gdb.patch || error "$GDB patch"
	rm -rf $TMPDIR/build-gdb && mkdir $TMPDIR/build-gdb && cd $TMPDIR/build-gdb || error "$TMPDIR/build-gdb access"
	../$GDB/configure --target=$TARGET --prefix=$PREFIX || error "$GDB configure"
	nice -10 make -j $(fgrep processor /proc/cpuinfo|wc -l) all &&\
		echo SUCCESS: gdb build
}

function install_gdb() {
	su -c "make -C $TMPDIR/build-gdb install && echo SUCCESS: gdb install"
}

function build_grub2() {
	rm -rf $TMPDIR/$GRUB && cd $TMPDIR || error "$TMPDIR access"
	tar xJf $DLDIR/$GRUB.tar.xz || error "$GRUB extract"
	cd $TMPDIR/$GRUB
	patch -p1 < $PTDIR/grub.patch || error "$GRUB patch"
	rm -rf $TMPDIR/build-grub && mkdir $TMPDIR/build-grub && cd $TMPDIR/build-grub || error "$TMPDIR/build-grub access"
	../$GRUB/configure --target=$TARGET --prefix=$PREFIX\
		--disable-werror || error "$GRUB configure"
	nice -10 make -j $(fgrep processor /proc/cpuinfo|wc -l) all &&\
		echo SUCCESS: grub2 build
}

function install_grub2() {
	su -c "make -C $TMPDIR/build-grub install && echo SUCCESS: grub2 install"
}

function install_all() {
	su -c "
	make -C $TMPDIR/build-gcc install && echo SUCCESS: gcc install
	make -C $TMPDIR/build-gdb install && echo SUCCESS: gdb install
	make -C $TMPDIR/build-grub install && echo SUCCESS: grub2 install"
}

function build_bochs() {
	rm -rf $TMPDIR/$BOCHS && cd $TMPDIR || error "$TMPDIR access"
	tar xzf $DLDIR/$BOCHS.tar.gz || error "$BOCHS extract"
	cd $TMPDIR/$BOCHS
	 ./configure --enable-smp --enable-cpu-level=6 --enable-all-optimizations\
		--enable-x86-64 --enable-pci --enable-vmx --enable-debugger\
		--enable-disasm --enable-debugger-gui --enable-logging --enable-fpu\
		--enable-iodebug --enable-sb16=dummy --enable-cdrom --enable-x86-debugger\
		--disable-plugins --disable-docbook --with-x --with-x11 --with-term\
		|| error "$BOCHS configure"
	sed -ie "/LIBS =  -lm/s//LIBS = -pthread -lm/" Makefile
	nice -10 make -j $(fgrep processor /proc/cpuinfo|wc -l) all &&\
		echo SUCCESS: bochs build
}

function install_bochs() {
	su -c "make -C $TMPDIR/$BOCHS install && echo SUCCESS: bochs install"
}

function build_qemu() {
	rm -rf $TMPDIR/$QEMU && cd $TMPDIR || error "$TMPDIR access"
	tar xjf $DLDIR/$QEMU.tar.bz2 || error "$QEMU extract"
	rm -rf $TMPDIR/build-qemu && mkdir $TMPDIR/build-qemu && cd $TMPDIR/build-qemu || error "$TMPDIR/build-qemu access"
	../$QEMU/configure --target-list=x86_64-softmmu || error "$QEMU configure"
	nice -10 make -j $(fgrep processor /proc/cpuinfo|wc -l) all &&\
		echo SUCCESS: qemu build
}

function install_qemu() {
	su -c "make -C $TMPDIR/build-qemu install && echo SUCCESS: qemu install"
}

if [ "$1" = "bochs" ]; then
	build_bochs
	install_bochs
elif [ "$1" = "qemu" ]; then
	build_qemu
	install_qemu
else
	build_gcc
	build_gdb
	build_grub2
	install_all
fi
