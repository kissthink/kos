help:
	@echo "USAGE:"
	@echo "$(MAKE) all      build everything"
	@echo "$(MAKE) clean    clean everything"
	@echo "$(MAKE) run      build and run (qemu)"
	@echo "$(MAKE) debug    build and debug (qemu)"
	@echo "$(MAKE) bochs    build and run/debug (bochs)"
	@echo "$(MAKE) dep      build dependencies"

all clean vclean run debug bochs dep depend:
	$(MAKE) -I $(CURDIR)/src -C src -j $(cat /proc/cpuinfo|fgrep processor|wc -l) $@

tgz: clean
	rm -f kos.tgz; tar czvf kos.tgz --xform 's,,kos/,' -X exclude \
	src patches COPYING README Makefile setup_crossenv.sh gdbinit
