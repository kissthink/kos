SRCDIR:=$(CURDIR)
include $(SRCDIR)/Makefile.config
-include Makefile.state

SOURCEDIRS=util dev drivers mach kern gdb world
SOURCES=$(wildcard $(addsuffix /*.cc,$(SOURCEDIRS)))
BSOURCES=$(notdir $(SOURCES))
OBJECTS=$(BSOURCES:%.cc=%.o)
SOBJECTS=$(BSOURCES:%.cc=%.s)
EOBJECTS=$(BSOURCES:%.cc=%.e)
AS_SOURCES=$(wildcard mach/*.S) $(wildcard gdb/*.S)
AS_OBJECTS=$(subst .S,.o,$(notdir $(AS_SOURCES)))
BOOT_SOURCES=$(wildcard boot/*.S)
BOOT_OBJECTS=$(subst .S,.o,$(notdir $(BOOT_SOURCES)))
LIBS=extern/acpica/libacpica.a extern/dlmalloc/malloc.o #extern/linux/libdriver.a
USERLIB=ulib/libKOS.a
STAGE=stage

.PHONY: all libs clean dep depend vclean distclean run\
	debug rgdb rgdba rgdbdebug rgdbadebug\
	bochs pxe lpxe rpxe qpxe\
	usb usbtest tests defines echo

.PHONY: Makefile.state

vpath %.cc $(SOURCEDIRS)
vpath %.S mach boot gdb
vpath %.asm boot
vpath %.cpp test

all: kernel.sys user/built

Makefile.state:
	@echo "CCSAVE=$(CC)" > Makefile.state
	@echo "RGDBSAVE=$(RGDB)" >> Makefile.state
ifneq ($(CC),$(CCSAVE))
	@$(MAKE) clean
endif
ifneq ($(RGDB),$(RGDBSAVE))
	@rm -f $(IMAGE)
endif

run: $(IMAGE)
	$(QEMU)

debug: $(IMAGE)
	$(QEMU) -s -S & sleep 1
	$(GDB) -x gdbinit.qemu kernel.sys.debug

rgdb:
	$(MAKE) RGDB=gdb $(IMAGE)
	$(QEMURGDB) & sleep 1
	$(GDB) -x gdbinit.rgdb kernel.sys.debug

rgdba:
	$(MAKE) RGDB=gdba $(IMAGE)
	$(QEMURGDB) & sleep 1
	$(GDB) -x gdbinit.rgdb kernel.sys.debug

rgdbdebug:
	$(MAKE) RGDB=gdb $(IMAGE)
	xterm -e $(GDB) -x gdbinit.qemu kernel.sys.debug &
	$(QEMURGDB) -s -S & sleep 1
	$(GDB) -x gdbinit.rgdb kernel.sys.debug

rgdbadebug:
	$(MAKE) RGDB=gdba $(IMAGE)
	xterm -e $(GDB) -x gdbinit.qemu kernel.sys.debug &
	$(QEMURGDB) -s -S & sleep 1
	$(GDB) -x gdbinit.rgdb kernel.sys.debug

tgdb:
	$(GDB) -x gdbinit.ttyS0 kernel.sys.debug

bochs: $(IMAGE)
	$(TOOLSDIR)/bin/bochs -f bochsrc -rc bochsrc.dbg -q

$(ISO) pxe rpxe: kernel.sys user/built setup_iso.sh
	rm -rf $(STAGE)
	./setup_iso.sh $(STAGE) $@ kernel.sys $(RGDB) user/exec/*

pxe: $(GRUBPXE)

lpxe: $(GRUBPXE)
	$(MAKE) pxe

qpxe: $(GRUBPXE)
	$(QEMUPXE)

$(GRUBPXE):
	rm -rf $(TFTPDIR)/boot
	$(GRUBDIR)/sbin/$(TARGET)-grub-mknetdir --net-directory=$(TFTPDIR)
	@echo ATTENTION: You also need pxelinux.0 pxelinux.cfg/default vesamenu.c32 in $(TFTPDIR)

$(VDISK): kernel.sys setup_img.sh
	dd if=/dev/zero of=$(VDISK) bs=516096c count=64 2>/dev/null
	./setup_img.sh $(VDISK) || { rm -f $(VDISK); false; }

usb: kernel.sys setup_usb.sh
	@test -b $(USBDEV)1 || { echo no USB device found at $(USBDEV)1 && false; }
	./setup_usb.sh $(USBDEV)

kernel.sys: kernel.sys.debug
	$(STRIP) $@.debug -o $@

# rebuild binary from scratch to enforce file order: boot.o must be first
kernel.sys.debug: linker.ld $(BOOT_OBJECTS) $(OBJECTS) $(AS_OBJECTS) $(LIBS)
	rm -f $@
	$(LD) -nodefaultlibs -T linker.ld -o $@ $(filter-out $<,$^) $(LDFLAGS)

$(BOOT_OBJECTS): %.o: %.S
	$(AS) $(ASFLAGS_BOOT) $< -o $@

$(OBJECTS): %.o: %.cc
	$(CXX) -c $(CXXFLAGS) $<
	
$(AS_OBJECTS): %.o: %.S
	$(AS) $(ASFLAGS) $< -o $@

$(SOBJECTS): %.s: %.cc
	$(CXX) -S $(PREFLAGS) $<
	
$(EOBJECTS): %.e: %.cc
	$(CXX) -E $(PREFLAGS) $< > $@

$(BOOT_OBJECTS) $(OBJECTS) $(AS_OBJECTS) $(SOBJECTS) $(EOBJECTS): Makefile.config

libs: $(LIBS)

# compiling acpica with clang is possible, but throws warnings
extern/acpica/libacpica.a: extern/acpica/Makefile
	$(MAKE) CC=$(GCC) -C extern/acpica $(notdir $@)

extern/dlmalloc/malloc.o: extern/dlmalloc/Makefile
	$(MAKE) -C extern/dlmalloc $(notdir $@)

extern/linux/libdriver.a: extern/linux/Makefile
	$(MAKE) -C extern/linux $(notdir $@)

$(USERLIB):
	$(MAKE) -C ulib $(notdir $@)

user/built: $(USERLIB)
	$(MAKE) -C user

defines:
	$(CXX) -dM -E - < /dev/null
	
echo:
	@echo SOURCES: $(SOURCES)
	@echo BSOURCES: $(BSOURCES)
	@echo OBJECTS: $(OBJECTS)

clean:
	$(MAKE) -C extern/dlmalloc $@
	$(MAKE) -C extern/acpica $@
	$(MAKE) -C extern/linux $@
	$(MAKE) -C user $@
	$(MAKE) -C ulib $@
	rm -f kernel.sys.debug kernel.sys $(VDISK) $(ISO) $(BOOT_OBJECTS)\
		$(OBJECTS) $(SOBJECTS) $(EOBJECTS) $(AS_OBJECTS)
	rm -rf $(STAGE)
	rm -f Makefile.o.dep Makefile.s.dep Makefile.e.dep

distclean: clean
	rm -f Makefile.dep Makefile.state

vclean: distclean
	$(MAKE) -C extern/acpica $@
	rm -f /tmp/KOS.dbg /tmp/KOS.serial /tmp/qemu.log

dep depend Makefile.dep: # include order: extern, util, mach, dep, kern
	$(CXX) -MM $(CXXFLAGS) $(SOURCES) > Makefile.o.dep
	cat Makefile.o.dep | sed -e "/.o:/s//.s:/" > Makefile.s.dep
	cat Makefile.o.dep | sed -e "/.o:/s//.e:/" > Makefile.e.dep
	cat Makefile.o.dep Makefile.s.dep Makefile.e.dep > Makefile.dep
	rm -f Makefile.o.dep Makefile.s.dep Makefile.e.dep

-include Makefile.dep
