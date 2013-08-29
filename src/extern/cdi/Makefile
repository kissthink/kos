SRCDIR:=$(CURDIR)/../..
include $(SRCDIR)/Makefile.config

CDI_NE2K	    =					  ne2k
CDI_RTL8139	  =						rtl8139
CDI_E1000     =           e1000
CDI_PCNET     =           pcnet
CDI_ATA       =           ata
CDI_FLOPPY    =           floppy
CDI_AC97      =           ac97
CDI_HDAUDIO   =           hdaudio
CDI_SIS900    =           sis900

CDI=$(CDI_NE2k) $(CDI_RTL8139) $(CDI_E1000) $(CDI_PCNET) $(CDI_ATA) $(CDI_FLOPPY) $(CDI_AC97) $(CDI_HDAUDIO) $(CDI_SIS900)

CDI_NE2K_SRCS     = $(wildcard $(addsuffix /*.c,$(CDI_NE2K)))
CDI_NE2K_OBJS     = $(subst .c,.o,$(CDI_NE2K_SRCS))
CDI_RTL8139_SRCS  = $(wildcard $(addsuffix /*.c,$(CDI_RTL8139)))
CDI_RTL8139_OBJS  = $(subst .c,.o,$(CDI_RTL8139_SRCS))
CDI_E1000_SRCS    = $(wildcard $(addsuffix /*.c,$(CDI_E1000)))
CDI_E1000_OBJS    = $(subst .c,.o,$(CDI_E1000_SRCS))
CDI_PCNET_SRCS    = $(wildcard $(addsuffix /*.c,$(CDI_PCNET)))
CDI_PCNET_OBJS    = $(subst .c,.o,$(CDI_PCNET_SRCS))
CDI_ATA_SRCS      = $(wildcard $(addsuffix /*.c,$(CDI_ATA)))
CDI_ATA_OBJS      = $(subst .c,.o,$(CDI_ATA_SRCS))
CDI_EXT2_SRCS     = $(wildcard $(addsuffix /*.c,$(CDI_EXT2)))
CDI_EXT2_OBJS     = $(subst .c,.o,$(CDI_EXT2_SRCS))
CDI_LIBEXT2_SRCS  = $(wildcard $(addsuffix /*.c,$(CDI_LIBEXT2)))
CDI_LIBEXT2_OBJS  = $(subst .c,.o,$(CDI_LIBEXT2_SRCS))
CDI_FLOPPY_SRCS   = $(wildcard $(addsuffix /*.c,$(CDI_FLOPPY)))
CDI_FLOPPY_OBJS   = $(subst .c,.o,$(CDI_FLOPPY_SRCS))
CDI_AC97_SRCS     = $(wildcard $(addsuffix /*.c,$(CDI_AC97)))
CDI_AC97_OBJS     = $(subst .c,.o,$(CDI_AC97_SRCS))
CDI_HDAUDIO_SRCS  = $(wildcard $(addsuffix /*.c,$(CDI_HDAUDIO)))
CDI_HDAUDIO_OBJS  = $(subst .c,.o,$(CDI_HDAUDIO_SRCS))
CDI_SIS900_SRCS   = $(wildcard $(addsuffix /*.c,$(CDI_SIS900)))
CDI_SIS900_OBJS   = $(subst .c,.o,$(CDI_SIS900_SRCS))

CDI_KOS   =               impl impl/scsi
KOS_SRCS=$(wildcard $(addsuffix /*.cc,$(CDI_KOS)))
KOS_OBJS=$(subst .cc,.o,$(KOS_SRCS))

NET_OBJS		=	$(CDI_NE2K_OBJS) $(CDI_RTL8139_OBJS) $(CDI_E1000_OBJS) $(CDI_PCNET_OBJS) $(CDI_SIS900_OBJS)
ATA_OBJS		=	$(CDI_ATA_OBJS) $(CDI_LIBEXT2_OBJS) $(CDI_EXT2_OBJS) $(CDI_FLOPPY_OBJS)
AUDIO_OBJS	=	$(CDI_AC97_OBJS) $(CDI_HDAUDIO_OBJS)
OBJECTS			=	$(NET_OBJS) $(ATA_OBJS) $(AUDIO_OBJS)

CFLAGS 		+= -Wno-missing-field-initializers -Wstrict-aliasing=0 -I$(SRCDIR) -Iinclude -D__KOS__ -DDEBUG -D_GNU_SOURCE $(OPTFLAGS)
CXXFLAGS 	+= -Wstrict-aliasing=0 -I$(SRCDIR) -Iinclude -D__KOS__ $(OPTFLAGS)

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

$(CDI_LIBEXT2_OBJS): %.o: %.c
		$(CC) -c $(CFLAGS) -I$(CDI_LIBEXT2)/include $< -o $@

$(CDI_EXT2_OBJS): %.o: %.c $(CDI_LIBEXT2_OBJS)
		$(CC) -c $(CFLAGS) -I$(CDI_LIBEXT2)/include $< -o $@

$(CDI_FLOPPY_OBJS): %.o: %.c
		$(CC) -c $(CFLAGS) $< -o $@

$(CDI_AC97_OBJS): %.o: %.c
		$(CC) -c $(CFLAGS) $< -o $@

$(CDI_HDAUDIO_OBJS): %.o: %.c
		$(CC) -c $(CFLAGS) $< -o $@

$(CDI_SIS900_OBJS): %.o: %.c
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
		@echo $(CDI_NE2K_OBJS)
		@echo $(CDI_RTL8139_OBJS)
		@echo $(CDI_E1000_OBJS)
		@echo $(CDI_PCNET_OBJS)
		@echo $(CDI_ATA_OBJS)
		@echo $(CDI_FLOPPY_OBJS)
		@echo $(CDI_AC97_OBJS)

clean:

vclean:
		rm -rf libcdi.a libcdi.o $(OBJECTS) $(KOS_OBJS)
