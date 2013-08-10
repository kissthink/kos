SRCDIR:=$(CURDIR)/../..
include $(SRCDIR)/Makefile.config

FREEBSD_KERNEL      =           sys/kern
FREEBSD_KERNEL_LIB  =           sys/libkern
FREEBSD_VM          =           sys/vm
FREEBSD_MACHINE     =           sys/amd64
FREEBSD_KOBJS       =           sys/kobjs
FREEBSD_DEVFS       =           sys/fs/devfs
FREEBSD_DEV_PCI     =           sys/dev/pci
FREEBSD_LIBIBERTY   =           sys/libiberty
FREEBSD_DRIVER      =           drivers
FREEBSD_CHAR_DRIVER =           drivers/sample/char
FREEBSD_NET_DRIVER  =           drivers/net/re
FREEBSD_TEST        =           test
FREEBSD_KOS         =           kos
FREEBSD_INCLUDE     =           $(SRCDIR)/extern/freebsd/sys
FREEBSD_KOS_INCLUDE =           include
FREEBSD_SYS_TYPES_H =           sys/sys/types.h

#FREEBSD_DIRS=sys drivers
#SOURCES=$(foreach sdir,$(FREEBSD_DIRS),$(wildcard $(sdir)/*.c))
#OBJECTS=$(subst .c,.o,$(notdir $(SOURCES)))
FREEBSD=$(FREEBSD_KERNEL) $(FREEBSD_KERNEL_LIB) $(FREEBSD_LIBIBERTY) $(FREEBSD_DRIVER) $(FREEBSD_CHAR_DRIVER)\
				$(FREEBSD_VM) $(FREEBSD_TEST) $(FREEBSD_MACHINE) #$(FREEBSD_KOBJS) $(FREEBSD_DEVFS) $(FREEBSD_NET_DRIVER) $(FREEBSD_DEV_PCI)
SOURCES=$(wildcard $(addsuffix /*.c,$(FREEBSD)))
OBJECTS=$(subst .c,.o,$(notdir $(SOURCES)))
KOS_SOURCES=$(wildcard $(addsuffix /*.cc,$(FREEBSD_KOS)))
KOS_OBJECTS=$(subst .cc,.o,$(notdir $(KOS_SOURCES)))

CFLAGS += -I$(FREEBSD_INCLUDE) -I$(FREEBSD_KOS_INCLUDE) -include $(FREEBSD_SYS_TYPES_H) -D_KERNEL -m64 -DSX_NOINLINE -DMUTEX_NOINLINE -D__FreeBSD__
#CFLAGS += -I$(FREEBSD_INCLUDE) -I$(FREEBSD_KOS_INCLUDE) -include $(FREEBSD_SYS_TYPES_H) -D_KERNEL -m64 -dD -E

CXXFLAGS+= -I$(FREEBSD_KOS_INCLUDE) -I$(SRCDIR)
#CXXFLAGS+=-I$(FREEBSD_KOS_INCLUDE) -I$(SRCDIR) -dD -E

vpath %.c $(FREEBSD)
vpath %.cc $(FREEBSD_KOS)

all: libbsd.o

$(OBJECTS): %.o: %.c
	$(CC) $(CFLAGS) -c $<

$(KOS_OBJECTS): %.o: %.cc
	$(CXX) $(CXXFLAGS) -c $<

libbsd.a: $(OBJECTS) $(KOS_OBJECTS)
	$(AR) r $@ $^
	$(RANLIB) $@

libbsd.o: $(OBJECTS) $(KOS_OBJECTS)
	$(LD) -r $^ -o $@

clean:
	rm -f libbsd.a libbsd.o $(OBJECTS) $(KOS_OBJECTS)
