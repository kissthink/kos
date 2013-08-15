#!/bin/bash
#echo "\
#.extern main
#
#.global _start
#_start:
#call main
#
# loop in case we haven't yet rescheduled
#_halt:
#hlt
#jmp _halt" > $1/libgloss/libnosys/crt0.S

echo "\
extern void exit(int);
extern int main();
void _start() {
	_exit(main());
	for (;;) {}
}" > $1/libgloss/libnosys/crt0.c


sed -i 's/OUTPUTS = /OUTPUTS = crt0.o /' $1/libgloss/libnosys/Makefile.in

echo 'newlib_cflags="${newlib_cflags} -DMALLOC_PROVIDED"'\
	>> $1/newlib/configure.host

cd $1/libgloss
autoconf
