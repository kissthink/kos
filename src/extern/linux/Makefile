SRCDIR:=$(CURDIR)/../..
include $(SRCDIR)/Makefile.config

LINUX_DRIVER =            driver
LINUX_DRIVER_BASE =       $(LINUX_DRIVER)/base
LINUX_DRIVER_PCI =        $(LINUX_DRIVER)/pci
LINUX_KERNEL =            kernel
LINUX_KOS =               kos
LINUX_LIB =               lib

LINUX=$(LINUX_DRIVER)\
	$(LINUX_DRIVER_BASE)\
	$(LINUX_DRIVER_PCI)\
	$(LINUX_KERNEL)\
	$(LINUX_LIB)

SOURCES=$(wildcard $(addsuffix /*.c,$(LINUX)))
OBJECTS=$(subst .c,.o,$(notdir $(SOURCES)))
KOS_SOURCES=$(wildcard $(addsuffix /*.cc,$(LINUX_KOS)))
KOS_OBJECTS=$(subst .cc,.o,$(notdir $(KOS_SOURCES)))
# FIXME only using e1000 driver for now; will expand to other drivers in the future
DRIVER_DIR=drivers/net/ethernet/intel/e1000
DRIVER_OBJECTS=$(SRCDIR)/extern/linux-$(LINUXVER)/$(DRIVER_DIR)/e1000.o

LINUX_INCLUDE =                 $(SRCDIR)/extern/linux-$(LINUXVER)/include
LINUX_UAPI_INCLUDE =            $(SRCDIR)/extern/linux-$(LINUXVER)/include/uapi
LINUX_ARCH_INCLUDE =            $(SRCDIR)/extern/linux-$(LINUXVER)/arch/x86/include
LINUX_ARCH_GEN_INCLUDE =        $(SRCDIR)/extern/linux-$(LINUXVER)/arch/x86/include/generated
LINUX_ARCH_UAPI_INCLUDE =       $(SRCDIR)/extern/linux-$(LINUXVER)/arch/x86/include/uapi
LINUX_ARCH_UAPI_GEN_INCLUDE =   $(SRCDIR)/extern/linux-$(LINUXVER)/arch/x86/include/generated/uapi
LINUX_KCONFIG_INCLUDE =         $(SRCDIR)/extern/linux-$(LINUXVER)/include/linux/kconfig.h
LINUX_AUTOCONF_INCLUDE =        $(SRCDIR)/extern/linux-$(LINUXVER)/include/generated/autoconf.h
LINUX_DRIVERS_BASE_INCLUDE =    $(SRCDIR)/extern/linux-$(LINUXVER)/drivers/base
KOS_WRAPPER_INCLUDE=            $(CURDIR)/kos

CFLAGS += -I$(LINUX_INCLUDE) -I$(LINUX_UAPI_INCLUDE) -I$(LINUX_ARCH_INCLUDE)\
	-I$(LINUX_ARCH_GEN_INCLUDE) -I$(LINUX_ARCH_UAPI_INCLUDE) -I$(LINUX_ARCH_UAPI_GEN_INCLUDE)\
	-I$(LINUX_DRIVERS_BASE_INCLUDE) -I$(KOS_WRAPPER_INCLUDE)\
	-include $(LINUX_KCONFIG_INCLUDE) -include $(LINUX_AUTOCONF_INCLUDE) -D__KERNEL__ -m64

CXXFLAGS+=-I$(SRCDIR)

vpath %.c $(LINUX)
vpath %.cc $(LINUX_KOS)

$(OBJECTS): %.o: %.c $(LINUX_AUTOCONF_INCLUDE)
	$(CC) $(CFLAGS) -c $<

$(KOS_OBJECTS): %.o: %.cc $(LINUX_AUTOCONF_INCLUDE)
	@echo $<
	$(CXX) $(CXXFLAGS) -c $<

$(LINUX_AUTOCONF_INCLUDE): $(SRCDIR)/extern/linux-$(LINUXVER)/Makefile
	$(MAKE) -C $(SRCDIR)/extern/linux-$(LINUXVER) defconfig
	$(MAKE) -C $(SRCDIR)/extern/linux-$(LINUXVER) prepare
	$(MAKE) -C $(SRCDIR)/extern/linux-$(LINUXVER) scripts

drivers: libdriver.a

libdriver.a: $(DRIVER_OBJECTS) $(OBJECTS) $(KOS_OBJECTS)
	$(AR) r $@ $^
	$(RANLIB) $@
	rm -rf $(OBJECTS) $(KOS_OBJECTS) $(DRIVER_OBJECTS)

$(DRIVER_OBJECTS): $(OBJECTS) $(KOS_OBJECTS)
	$(MAKE) -C $(SRCDIR)/extern/linux-$(LINUXVER) SUBDIRS=$(DRIVER_DIR)

clean:
	rm -f libdriver.a $(OBJECTS) $(KOS_OBJECTS) $(DRIVER_OBJECTS)
#	$(MAKE) -C $(SRCDIR)/extern/linux-$(LINUXVER) $@
#	$(MAKE) -C $(SRCDIR)/extern/linux-$(LINUXVER) mrproper
#	$(MAKE) -C $(SRCDIR)/extern/linux-$(LINUXVER) distclean
