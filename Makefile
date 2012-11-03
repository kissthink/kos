help:
	@echo "USAGE:"
	@echo "$(MAKE) all      build everything"
	@echo "$(MAKE) clean    clean everything"
	@echo "$(MAKE) run      build and run (qemu)"
	@echo "$(MAKE) debug    build and debug (qemu)"
	@echo "$(MAKE) bochs    build and run/debug (bochs)"
	@echo "$(MAKE) dep      build dependencies"

all clean vclean run debug bochs dep depend:
	$(MAKE) -C src $@ -j $(cat /proc/cpuinfo|fgrep processor|wc -l) 

tgz: clean
	cd .. ; tar czvf /tmp/kos.tgz -X kos/exclude kos/src kos/patches \
	kos/COPYING kos/README kos/Makefile kos/setup_crossenv.sh
	mv /tmp/kos.tgz .
