include config

help:
	@echo "USAGE:"
	@echo "$(MAKE) all      build everything"
	@echo "$(MAKE) clean    clean everything"
	@echo "$(MAKE) run      build and run (qemu)"
	@echo "$(MAKE) debug    build, run (qemu), and debug (qemu/gdb)"
	@echo "$(MAKE) rgdb     build, run (qemu), and debug (remote gdb)"
	@echo "$(MAKE) bochs    build and run/debug (bochs)"
	@echo "$(MAKE) dep      build dependencies"

all clean vclean distclean run debug rgdb bochs dep depend:
	$(MAKE) -I $(CURDIR)/src -C src -j $(cat /proc/cpuinfo|fgrep processor|wc -l) $@

tgz: distclean
	rm -f kos.tgz; tar czvf kos.tgz --xform 's,,kos/,' -X exclude \
	src patches COPYING README Makefile setup_crossenv.sh
