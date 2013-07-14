#!/bin/bash

# Removes all source files not required for compiling driver code

find src/extern/linux-3.9.4/arch/ -regextype posix-extended -regex "(.*/.*config)|(.*/syscalls/.*)|(.*/kernel/asm-offsets.*\.c)|(.*/tools/.*)|(.*/Kconfig.*)|(.*/Makefile.*)|(.*/Kbuild)|(.*/.*\.h)" -prune -o -exec rm -f {} \; 2> /dev/null
find src/extern/linux-3.9.4/block/ -regextype posix-extended -regex "(.*/Kconfig.*)|(.*/Makefile)" -prune -o -exec rm -f {} \; 2> /dev/null
find src/extern/linux-3.9.4/crypto/ -regextype posix-extended -regex "(.*/Kconfig.*)|(.*/Makefile)" -prune -o -exec rm -f {} \; 2> /dev/null
find src/extern/linux-3.9.4/fs/ -regextype posix-extended -regex "(.*/Kconfig.*)|(.*/Makefile)|(.*/Kbuild)" -prune -o -exec rm -f {} \; 2> /dev/null
find src/extern/linux-3.9.4/init/ -regextype posix-extended -regex "(.*/Kconfig.*)|(.*/Makefile)" -prune -o -exec rm -f {} \; 2> /dev/null
find src/extern/linux-3.9.4/ipc/ -regextype posix-extended -regex "(.*/Kconfig.*)|(.*/Makefile)" -prune -o -exec rm -f {} \; 2> /dev/null
find src/extern/linux-3.9.4/kernel/ -regextype posix-extended -regex "(.*/Kconfig.*)|(.*/Makefile)|(.*/bounds\.c)" -prune -o -exec rm -f {} \; 2> /dev/null
find src/extern/linux-3.9.4/lib/ -regextype posix-extended -regex "(.*/Kconfig.*)|(.*/Makefile)" -prune -o -exec rm -f {} \; 2> /dev/null
find src/extern/linux-3.9.4/mm/ -regextype posix-extended -regex "(.*/Kconfig.*)|(.*/Makefile)" -prune -o -exec rm -f {} \; 2> /dev/null
find src/extern/linux-3.9.4/net/ -regextype posix-extended -regex "(.*/Kconfig.*)|(.*/Makefile)" -prune -o -exec rm -f {} \; 2> /dev/null
find src/extern/linux-3.9.4/samples/ -regextype posix-extended -regex "(.*/Kconfig.*)|(.*/Makefile)" -prune -o -exec rm -f {} \; 2> /dev/null
find src/extern/linux-3.9.4/security/ -regextype posix-extended -regex "(.*/Kconfig.*)|(.*/Makefile)|(.*/.*\.h)" -prune -o -exec rm -f {} \; 2> /dev/null
find src/extern/linux-3.9.4/sound/ -regextype posix-extended -regex "(.*/Kconfig.*)|(.*/Makefile)" -prune -o -exec rm -f {} \; 2> /dev/null
find src/extern/linux-3.9.4/tools/ -regextype posix-extended -regex "(.*/Kconfig.*)|(.*/Makefile)|(.*/.*\.h)" -prune -o -exec rm -f {} \; 2> /dev/null
find src/extern/linux-3.9.4/usr/ -regextype posix-extended -regex "(.*/Kconfig.*)|(.*/Makefile)" -prune -o -exec rm -f {} \; 2> /dev/null
find src/extern/linux-3.9.4/virt/ -regextype posix-extended -regex "(.*/Kconfig.*)|(.*/Makefile)" -prune -o -exec rm -f {} \; 2> /dev/null
