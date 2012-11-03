include Makefile.config

SOURCEDIRS=util dev mach kern gdb
SOURCES=$(wildcard $(addsuffix /*.cc,$(SOURCEDIRS)))
BSOURCES=$(notdir $(SOURCES))
OBJECTS=$(BSOURCES:%.cc=%.o)
SOBJECTS=$(BSOURCES:%.cc=%.s)
EOBJECTS=$(BSOURCES:%.cc=%.e)
AS_SOURCES=$(wildcard mach/*.S)
AS_OBJECTS=$(subst .S,.o,$(notdir $(AS_SOURCES)))
NASM_SOURCES=$(wildcard boot/*.asm)
NASM_OBJECTS=$(subst .asm,.o,$(notdir $(NASM_SOURCES)))
LIBS=extern/acpica/libacpica.a

.PHONY: all run debug bochs image iso usbtest usb pxe libs tests defines dep depend echo clean vclean

vpath %.cc $(SOURCEDIRS)
vpath %.S mach
vpath %.asm boot
vpath %.cpp test

all: kernel.sys

user/built:
	$(MAKE) -C user

run: $(IMAGE)
	$(QEMU)

debug: $(IMAGE)
	$(QEMU) -s -S & sleep 1
	$(GDB) -x gdbinit kernel.sys.debug

bochs: $(IMAGE)
	bochs -f bochsrc -rc bochsrc.dbg -q

image: $(VDISK)

iso: $(ISO)

usbtest:
	@test -b $(USBDEV)1 || { echo USE device at $(USBDEV)1 not found && false; }

usb: usbtest kernel.sys setup_usb.sh
	./setup_usb.sh $(USBDEV)

pxe: kernel.sys
	$(EXEPREFIX)-grub-mkimage --format=i386-pc --output=core.img --prefix='(pxe)/boot/grub' pxe pxechain
	cat $(PREFIX)/lib/grub/i386-pc/pxeboot.img core.img > pxegrub.0
	scp pxegrub.0 fileserver:/tftpboot2/
	scp kernel.sys fileserver:/tftpboot2/boot/

$(VDISK): kernel.sys setup_img.sh
	dd if=/dev/zero of=$(VDISK) bs=516096c count=64 2>/dev/null
	./setup_img.sh $(VDISK) || { rm -f $(VDISK); false; }

$(ISO): kernel.sys user/built setup_iso.sh
	rm -rf iso
	./setup_iso.sh $(ISO) kernel.sys user/exec/*

kernel.sys: kernel.sys.debug
	$(STRIP) $^ -o $@

# rebuild binary from scratch to enforce file order: boot.o must be first
kernel.sys.debug: linker.ld $(NASM_OBJECTS) $(OBJECTS) $(AS_OBJECTS) $(LIBS)
	rm -f $@
	$(LD) -nodefaultlibs -T linker.ld -o $@ $(filter-out $<,$^) $(LDFLAGS)

$(NASM_OBJECTS): %.o: %.asm
	$(NASM) $(NASMFLAGS) $< -o $@

$(AS_OBJECTS): %.o: %.S
	$(AS) $(ASFLAGS) $< -o $@

$(OBJECTS): %.o: %.cc
	$(CXX) -c $(CXXFLAGS) $<
	
$(SOBJECTS): %.s: %.cc
	$(CXX) -S $(PREFLAGS) $<
	
$(EOBJECTS): %.e: %.cc
	$(CXX) -E $(PREFLAGS) $< > $@

libs: $(LIBS)

extern/acpica/libacpica.a: extern/acpica/Makefile
	$(MAKE) -C extern/acpica $(notdir $@)

defines:
	$(CXX) -dM -E - < /dev/null
	
dep depend: # include order: extern, util, mach, dep, kern
	$(CXX) -MM $(CXXFLAGS) $(SOURCES) > Makefile.o.dep
	cat Makefile.o.dep | sed -e "/.o:/s//.s:/" > Makefile.s.dep
	cat Makefile.o.dep | sed -e "/.o:/s//.e:/" > Makefile.e.dep
	cat Makefile.o.dep Makefile.s.dep Makefile.e.dep > Makefile.dep
	rm -f Makefile.o.dep Makefile.s.dep Makefile.e.dep

echo:
	@echo SOURCES: $(SOURCES)
	@echo BSOURCES: $(BSOURCES)
	@echo OBJECTS: $(OBJECTS)

clean:
	$(MAKE) -C extern/acpica $@
	$(MAKE) -C user $@
	rm -f kernel.sys.debug kernel.sys $(VDISK) $(ISO)\
		$(OBJECTS) $(SOBJECTS) $(EOBJECTS) $(AS_OBJECTS) $(NASM_OBJECTS)
	rm -rf iso pxegrub.0 core.img
	rm -f /tmp/KOS.dbg /tmp/KOS.serial /tmp/qemu.log

vclean: clean
	$(MAKE) -C extern/acpica $@

distclean: clean
	rm -f Makefile.o.dep Makefile.s.dep Makefile.e.dep Makefile.dep

-include Makefile.dep
