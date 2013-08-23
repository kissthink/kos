SRCDIR:=$(CURDIR)/../..
include $(SRCDIR)/Makefile.config

CDI_NE2K	=								ne2k

CDI=$(CDI_NE2k)

CDI_NE2K_SRCS=$(wildcard $(addsuffix /*.c,$(CDI_NE2K)))
CDI_NE2K_OBJS=$(subst .c,.o,$(CDI_NE2K_SRCS))

CDI_KOS   =               impl
KOS_SRCS=$(wildcard $(addsuffix /*.cc,$(CDI_KOS)))
KOS_OBJS=$(subst .cc,.o,$(KOS_SRCS))

OBJECTS=$(CDI_NE2K_OBJS)

CFLAGS += -I$(SRCDIR) -Iinclude -D__KOS__ $(OPTFLAGS)
CXXFLAGS += -I$(SRCDIR) -Iinclude -D__KOS__ $(OPTFLAGS)

vpath %.c $(CDI)
vpath %.cc $(CDI_KOS)

all: libcdi.o

$(CDI_NE2K_OBJS): %.o: %.c
		$(CC) -c $(CFLAGS) -I$(CDI_NE2K)/include $< -o $@

$(KOS_OBJS): %.o: %.cc
		$(CXX) -c $(CXXFLAGS) $< -o $@

libcdi.o: $(CDI_NE2K_OBJS) $(KOS_OBJS)
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
