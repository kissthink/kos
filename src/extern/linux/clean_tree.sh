#!/bin/bash

# Removes all source files not required for compiling driver code

source ../../../config

find ../linux-$LINUXVER/arch/ -regextype posix-extended -regex "(.*/.*config)|(.*/syscalls/.*)|(.*/kernel/asm-offsets.*\.c)|(.*/tools/.*)|(.*/Kconfig.*)|(.*/Makefile.*)|(.*/Kbuild)|(.*/.*\.h)" -prune -o -exec rm -f {} \; 2> /dev/null
find ../linux-$LINUXVER/block/ -regextype posix-extended -regex "(.*/Kconfig.*)|(.*/Makefile)" -prune -o -exec rm -f {} \; 2> /dev/null
find ../linux-$LINUXVER/crypto/ -regextype posix-extended -regex "(.*/Kconfig.*)|(.*/Makefile)" -prune -o -exec rm -f {} \; 2> /dev/null
find ../linux-$LINUXVER/fs/ -regextype posix-extended -regex "(.*/Kconfig.*)|(.*/Makefile)|(.*/Kbuild)" -prune -o -exec rm -f {} \; 2> /dev/null
find ../linux-$LINUXVER/init/ -regextype posix-extended -regex "(.*/Kconfig.*)|(.*/Makefile)" -prune -o -exec rm -f {} \; 2> /dev/null
find ../linux-$LINUXVER/ipc/ -regextype posix-extended -regex "(.*/Kconfig.*)|(.*/Makefile)" -prune -o -exec rm -f {} \; 2> /dev/null
find ../linux-$LINUXVER/kernel/ -regextype posix-extended -regex "(.*/Kconfig.*)|(.*/Makefile)|(.*/bounds\.c)" -prune -o -exec rm -f {} \; 2> /dev/null
find ../linux-$LINUXVER/lib/ -regextype posix-extended -regex "(.*/Kconfig.*)|(.*/Makefile)" -prune -o -exec rm -f {} \; 2> /dev/null
find ../linux-$LINUXVER/mm/ -regextype posix-extended -regex "(.*/Kconfig.*)|(.*/Makefile)" -prune -o -exec rm -f {} \; 2> /dev/null
find ../linux-$LINUXVER/net/ -regextype posix-extended -regex "(.*/Kconfig.*)|(.*/Makefile)" -prune -o -exec rm -f {} \; 2> /dev/null
find ../linux-$LINUXVER/samples/ -regextype posix-extended -regex "(.*/Kconfig.*)|(.*/Makefile)" -prune -o -exec rm -f {} \; 2> /dev/null
find ../linux-$LINUXVER/security/ -regextype posix-extended -regex "(.*/Kconfig.*)|(.*/Makefile)|(.*/.*\.h)" -prune -o -exec rm -f {} \; 2> /dev/null
find ../linux-$LINUXVER/sound/ -regextype posix-extended -regex "(.*/Kconfig.*)|(.*/Makefile)" -prune -o -exec rm -f {} \; 2> /dev/null
find ../linux-$LINUXVER/tools/ -regextype posix-extended -regex "(.*/Kconfig.*)|(.*/Makefile)|(.*/.*\.h)" -prune -o -exec rm -f {} \; 2> /dev/null
find ../linux-$LINUXVER/usr/ -regextype posix-extended -regex "(.*/Kconfig.*)|(.*/Makefile)" -prune -o -exec rm -f {} \; 2> /dev/null
find ../linux-$LINUXVER/virt/ -regextype posix-extended -regex "(.*/Kconfig.*)|(.*/Makefile)" -prune -o -exec rm -f {} \; 2> /dev/null
