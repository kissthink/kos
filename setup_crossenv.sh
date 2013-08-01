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

function prebuild() {
	dir=$TMPDIR/$1
	[ -f $PTDIR/$1.patch ] && {
		patch -d $dir -p1 < $PTDIR/$1.patch || error "$1 patch"
	}
	rm -rf $dir-build && mkdir $dir-build && cd $dir-build || error "$dir-build access"
}

function build() {
	cd $TMPDIR/$1-build
	nice -10 make -j $cpucount $2 all || error "$1 build failed"
}

function install() {
	$ROOTEXEC -c "make -C $TMPDIR/$1-build install && echo SUCCESS: $1 install"
}

function build_usergcc() {
	rm -rf $TMPDIR/$GCC && mkdir $TMPDIR/$GCC && cd $TMPDIR/$GCC || error "$TMPDIR/$GCC access"
	# order often important (gcc last)
	tar xaf $DLDIR/$BINUTILS.tar.bz2 --strip-components 1 || error "$BINUTILS extract"
	tar xaf $DLDIR/$GCC.tar.bz2 --strip-components 1 || error "$GCC extract"
# for binutils 2.32.2:
	sed -i -e 's/@colophon/@@colophon/' \
	       -e 's/doc@cygnus.com/doc@@cygnus.com/' bfd/doc/bfd.texinfo
# for gcc 4.7.2:
#	sed -i 's/BUILD_INFO=info/BUILD_INFO=/' $GCC/gcc/configure
#	sed -i 's/thread_local/thread_localX/' $GCC/gcc/tree.h
	prebuild $GCC
	../$GCC/configure --target=$TARGET --prefix=$UKOSDIR\
		--enable-languages=c --enable-lto --enable-shared\
		--disable-nls --disable-libquadmath --disable-libssp\
		|| error "$GCC configure"
	build $GCC
}
# --enable-gold --enable-ld=default --enable-plugin --with-plugin-ld=ld.gold

function build_bsdlibc() {
	BSDDIR=$TMPDIR/bsd
	rm -rf $BSDDIR

	BINDIR=$BSDDIR/bin
	mkdir -p $BINDIR
	export PATH=$BINDIR:$PATH
	echo "while read line; do for i in \$line; do echo \$i; done; done|sort|uniq"\
		> $BINDIR/tsort
	chmod +x $BINDIR/tsort
	echo "exec echo \$*" > $BINDIR/lorder
	chmod +x $BINDIR/lorder

	cd $BSDDIR
	tar xaf $DLDIR/src.txz
	cd usr/src/usr.bin/make
	patch -p1 < $PTDIR/fbsd-make.patch
	sh $PTDIR/fbsd-make.sed.patch
	make -f Makefile.dist
	cp pmake $BINDIR
	cd $BSDDIR
	mkdir -p dest/usr/include
	cd dest/usr/include
	ln -s ../../../usr/src/include/rpcsvc .
	cd $BSDDIR/usr/src/include
	ln -s ../sys/amd64/include machine
	ln -s ../sys/x86/include x86

	cd $BSDDIR
	echo "#undef __FreeBSD_version
#define __FreeBSD_version 901000" > usr/src/include/osreldate.h
	cp usr/src/lib/msun/src/math.h usr/src/include
	cp usr/src/lib/msun/amd64/fenv.h usr/src/include
	cp usr/src/lib/libutil/libutil.h usr/src/include
	mkdir usr/src/include/altq
	cp usr/src/sys/contrib/altq/altq/if_altq.h usr/src/include/altq
	cd usr/src/include/rpcsvc
	rpcgen -h crypt.x > crypt.h
	rpcgen -h key_prot.x > key_prot.h
	patch nis.x < $PTDIR/fbsd-nis.x.patch
	rpcgen -h nis.x > nis.h
	rpcgen -h yp.x > yp.h
	mv key_prot.h ../rpc
	  	
	cd $BSDDIR/usr/src/lib/csu
	patch -p1 < $PTDIR/fbsd-csu.patch
	cd amd64

	CC=$UKOSDIR/bin/$TARGET-gcc PATH=$UKOSDIR/$TARGET/bin:$PATH \
	pmake DESTDIR=$BSDDIR/dest SSP_CFLAGS=-fno-stack-protector

	cd $BSDDIR/usr/src/lib/libc
	patch -p1 < $PTDIR/fbsd-libc.patch
	sed -i 's/\$$/\$ /' nls/*.msg
	sed -i 's/2 RPC/72 RPC/' nls/mn_MN.UTF-8.msg
	
	ln -s amd64 unknown
	ln -s amd64 x86_64	

	# this command finds cc1, but ld does not produce a shared library
	CC=$UKOSDIR/bin/$TARGET-gcc PATH=$UKOSDIR/$TARGET/bin:$PATH \
	pmake DESTDIR=$BSDDIR/dest SSP_CFLAGS=-fno-stack-protector \
	KOSGCCDIR=$UKOSDIR/lib/gcc/$TARGET/$GCCVER/include
	rm libc.*
	# this command does not find cc1, but ld produces shared library
	PATH=$UKOSDIR/$TARGET/bin:$PATH \
	pmake DESTDIR=$BSDDIR/dest SSP_CFLAGS=-fno-stack-protector \
	KOSGCCDIR=$UKOSDIR/lib/gcc/$TARGET/$GCCVER/include
}

function install_bsdlibc() {
	# copy libs to $UKOSDIR/$TARGET/lib
	# copy header files to $UKOSDIR/$TARGET/include
}

function build_gcc() {
	rm -rf $TMPDIR/$GCC && mkdir $TMPDIR/$GCC && cd $TMPDIR/$GCC || error "$TMPDIR/$GCC access"
	# order often important (gcc last)
	tar xaf $DLDIR/$BINUTILS.tar.bz2 --strip-components 1 || error "$BINUTILS extract"
	tar xaf $DLDIR/$NEWLIB.tar.gz --strip-components 1 || error "$NEWLIB extract"
	tar xaf $DLDIR/$GCC.tar.bz2 --strip-components 1 || error "$GCC extract"
# for binutils 2.32.2:
	sed -i -e 's/@colophon/@@colophon/' \
	       -e 's/doc@cygnus.com/doc@@cygnus.com/' bfd/doc/bfd.texinfo
# for gcc 4.7.2:
#	sed -i 's/BUILD_INFO=info/BUILD_INFO=/' gcc/configure
#	sed -i 's/thread_local/thread_localX/' gcc/tree.h
	cp $PTDIR/crt0.S libgloss/libnosys || error "crt0.S copy"
	sed -i 's/OUTPUTS = /OUTPUTS = crt0.o /' libgloss/libnosys/Makefile.in
	echo 'newlib_cflags="${newlib_cflags} -DMALLOC_PROVIDED"' >> newlib/configure.host
	cd libgloss; autoconf || error "libgloss autoconf" ; cd ..
	prebuild $GCC
	../$GCC/configure --target=$TARGET --prefix=$GCCDIR\
		--disable-threads --disable-tls --disable-nls --enable-lto\
		--enable-languages=c,c++ --with-newlib || error "$GCC configure"
	# compile gcc first pass
	nice -10 make -j $cpucount all || error "$GCC first pass failed"
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
	# recompile gcc with new libraries
	build $GCC
}

function build_gdb() {
	unpack $GDB tar.bz2
	prebuild $GDB
	../$GDB/configure --target=$TARGET --prefix=$TOOLSDIR || error "$GDB configure"
	build $GDB
}

function build_grub() {
	unpack $GRUB tar.xz
	prebuild $GRUB
	../$GRUB/configure --target=$TARGET --prefix=$GRUBDIR --disable-werror\
	--disable-device-mapper || error "$GRUB configure"
	build $GRUB
}

function build_bochs() {
	unpack $BOCHS tar.gz
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
	prebuild $QEMU
	../$QEMU/configure --target-list=x86_64-softmmu --prefix=$TOOLSDIR\
	--python=/usr/bin/python2 --enable-sdl || error "$QEMU configure"
	build $QEMU
}

function build_user() {
	while [ $# -gt 0 ]; do case "$1" in
	gcc)
		build_usergcc && install $GCC;;
	bsdlibc)
		build_bsdlibc;;
	*)
		echo unknown argument: $1;;
	esac; shift; done
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
user)
	shift
	build_user $1;;
*)
	echo unknown argument: $1;;
esac; shift; done
