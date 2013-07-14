help:
	@echo "USAGE:"
	@echo "$(MAKE) all      build everything"
	@echo "$(MAKE) clean    clean everything"
	@echo "$(MAKE) run      build and run (qemu)"
	@echo "$(MAKE) debug    build, run (qemu), and debug (qemu/gdb)"
	@echo "$(MAKE) rgdb     build, run (qemu), and debug (remote gdb)"
	@echo "$(MAKE) rgdba    build, run (qemu), and debug (remote gdb all-stop)"
	@echo "$(MAKE) qpxe     build, run (qemu) via simulated PXE"
	@echo "$(MAKE) bochs    build and run/debug (bochs)"
	@echo "$(MAKE) dep      build dependencies"
	@echo "$(MAKE) drivers	build linux drivers"


all libs clean dep depend vclean distclean run debug rgdb rgdba rgdbdebug rgdbadebug bochs pxe lpxe rpxe qpxe usb usbtest tests defines echo drivers:
	nice -10 $(MAKE) -I $(CURDIR)/src -C src -j $(shell fgrep processor /proc/cpuinfo|wc -l) $@

tgz: distclean
	rm -f kos.tgz; tar czvf kos.tgz --xform 's,,kos/,' \
	--exclude src/extern/acpica --exclude src/config \
	config COPYING Makefile patches README setup_crossenv.sh src
	$(MAKE) dep

-include Makefile.local # development targets, not for release
