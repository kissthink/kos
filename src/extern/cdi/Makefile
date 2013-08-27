SRCDIR:=$(CURDIR)/../..
include $(SRCDIR)/Makefile.config

CDI_NE2K	    =					  ne2k
CDI_RTL8139	  =						rtl8139
CDI_E1000     =           e1000
CDI_PCNET     =           pcnet
CDI_ATA       =           ata

CDI=$(CDI_NE2k) $(CDI_RTL8139) $(CDI_E1000) $(CDI_PCNET) $(CDI_ATA)

CDI_NE2K_SRCS=$(wildcard $(addsuffix /*.c,$(CDI_NE2K)))
CDI_NE2K_OBJS=$(subst .c,.o,$(CDI_NE2K_SRCS))
CDI_RTL8139_SRCS=$(wildcard $(addsuffix /*.c,$(CDI_RTL8139)))
CDI_RTL8139_OBJS=$(subst .c,.o,$(CDI_RTL8139_SRCS))
CDI_E1000_SRCS=$(wildcard $(addsuffix /*.c,$(CDI_E1000)))
CDI_E1000_OBJS=$(subst .c,.o,$(CDI_E1000_SRCS))
CDI_PCNET_SRCS=$(wildcard $(addsuffix /*.c,$(CDI_PCNET)))
CDI_PCNET_OBJS=$(subst .c,.o,$(CDI_PCNET_SRCS))
CDI_ATA_SRCS=$(wildcard $(addsuffix /*.c,$(CDI_ATA)))
CDI_ATA_OBJS=$(subst .c,.o,$(CDI_ATA_SRCS))

CDI_KOS   =               impl
KOS_SRCS=$(wildcard $(addsuffix /*.cc,$(CDI_KOS)))
KOS_OBJS=$(subst .cc,.o,$(KOS_SRCS))

OBJECTS=$(CDI_NE2K_OBJS) $(CDI_RTL8139_OBJS) $(CDI_E1000_OBJS) $(CDI_PCNET_OBJS) $(CDI_ATA_OBJS)

CFLAGS += -I$(SRCDIR) -Iinclude -D__KOS__ -DDEBUG -DDEBUG_ENABLE -D_GNU_SOURCE $(OPTFLAGS)
CXXFLAGS += -I$(SRCDIR) -Iinclude -D__KOS__ $(OPTFLAGS)

vpath %.c $(CDI)
vpath %.cc $(CDI_KOS)

all: libcdi.o

$(CDI_NE2K_OBJS): %.o: %.c
		$(CC) -c $(CFLAGS) -I$(CDI_NE2K)/include $< -o $@

$(CDI_RTL8139_OBJS): %.o: %.c
		$(CC) -c $(CFLAGS) -I$(CDI_RTL8139)/include $< -o $@

$(CDI_E1000_OBJS): %.o: %.c
		$(CC) -c $(CFLAGS) $< -o $@

$(CDI_PCNET_OBJS): %.o: %.c
		$(CC) -c $(CFLAGS) -I$(CDI_PCNET)/include $< -o $@

$(CDI_ATA_OBJS): %.o: %.c
		$(CC) -c $(CFLAGS) $< -o $@

$(KOS_OBJS): %.o: %.cc
		$(CXX) -c $(CXXFLAGS) $< -o $@

libcdi.o: $(OBJECTS) $(KOS_OBJS)
		$(LD) -r -o $@ $^

libcdi.a: $(OBJECTS)
		$(AR) r $@ $^
		$(RANLIB) $@

define:
		@echo $(CC)
		@echo $(SRCDIR)
		@echo $(CDI_NE2K_SRCS)
		@echo $(CDI_NE2K_OBJS)

clean:

vclean:
		rm -rf libcdi.a libcdi.o $(OBJECTS) $(KOS_OBJS)
