SRCDIR=$(CURDIR)/../..
include $(SRCDIR)/Makefile.config

CDI_AC97    =   ac97
CDI_ATA     =   ata
CDI_E1000   =   e1000
CDI_EXT2    =   ext2
CDI_FLOPPY  =   floppy
CDI_HDAUDIO =   hdaudio
CDI_ISO9660 =   iso9660
CDI_NE2K    =   ne2k
CDI_PCNET   =   pcnet
CDI_RAMDISK =   ramdisk
CDI_RTL8139 =   rtl8139
CDI_SERIAL  =   serial
CDI_SIS900  =   sis900

CDI=$(CDI_AC97)\
  $(CDI_ATA)\
  $(CDI_E1000)\
  $(CDI_EXT2)\
  $(CDI_FLOPPY)\
  $(CDI_HDAUDIO)\
  $(CDI_ISO9660)\
  $(CDI_NE2K)\
  $(CDI_PCNET)\
  $(CDI_RAMDISK)\
  $(CDI_RLT8139)\
  $(CDI_SERIAL)\
  $(CDI_SIS900)

run_command=$(if $(V),$(2),@echo $(1);$(2))

#CDI_SRCS=$(wildcard $(addsuffix /*.c,$(CDI)))
#CDI_OBJS=$(subst .c,.o,$(notdir $(CDI_SRCS)))

CDI_SRCS=$(shell find -name '*.c')
CDI_OBJS=$(addsuffix .o,$(basename $(CDI_SRCS)))

CFLAGS += -Iinclude -Iext2/libext2/include -D__KOS__ $(OPTFLAGS)

%.o: %.c
		$(call run_command,"CC $^",$(CC) $(CFLAGS) $(addprefix -I,$(addprefix $(dir $^),. include)) -c -o $@ $^)

libcdi.a: $(CDI_OBJS)
		$(AR) r $@ $^
		$(RANLIB) $@
		rm -rf $(CDI_OBJS)

clean:

vclean:
	rm -rf libcdi.a $(CDI_OBJS)

doc:
	for lang in german english; do \
		sed -e "s/%LANG%/$$lang/g#" doc/doxyfile.in > doc/doxyfile; \
		doxygen doc/doxyfile; \
	done

