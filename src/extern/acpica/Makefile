include ../../Makefile.config

ACPICA_SRC =            source
ACPICA_COMMON =         $(ACPICA_SRC)/common
ACPICA_TOOLS =          $(ACPICA_SRC)/tools
ACPICA_OSL =            $(ACPICA_SRC)/os_specific/service_layers
ACPICA_CORE =           $(ACPICA_SRC)/components
ACPICA_INCLUDE =        $(ACPICA_SRC)/include
ACPICA_DEBUGGER =       $(ACPICA_CORE)/debugger
ACPICA_DISASSEMBLER =   $(ACPICA_CORE)/disassembler
ACPICA_DISPATCHER =     $(ACPICA_CORE)/dispatcher
ACPICA_EVENTS =         $(ACPICA_CORE)/events
ACPICA_EXECUTER =       $(ACPICA_CORE)/executer
ACPICA_HARDWARE =       $(ACPICA_CORE)/hardware
ACPICA_NAMESPACE =      $(ACPICA_CORE)/namespace
ACPICA_PARSER =         $(ACPICA_CORE)/parser
ACPICA_RESOURCES =      $(ACPICA_CORE)/resources
ACPICA_TABLES =         $(ACPICA_CORE)/tables
ACPICA_UTILITIES =      $(ACPICA_CORE)/utilities

ACPICA=$(ACPICA_DISPATCHER)\
	$(ACPICA_EVENTS)\
	$(ACPICA_EXECUTER)\
	$(ACPICA_HARDWARE)\
	$(ACPICA_NAMESPACE)\
	$(ACPICA_PARSER)\
	$(ACPICA_RESOURCES)\
	$(ACPICA_TABLES)\
	$(ACPICA_UTILITIES)

SOURCES=$(wildcard $(addsuffix /*.c,$(ACPICA)))

OBJECTS=$(subst .c,.o,$(notdir $(SOURCES)))

CFLAGS += -I$(ACPICA_INCLUDE) -D__KOS__ -Wstrict-aliasing=0

vpath %.c $(ACPICA)

libacpica.a: $(OBJECTS)
	$(AR) r $@ $^
	$(RANLIB) $@
	rm -rf $(OBJECTS)

clean:

vclean:
	rm -rf libacpica.a $(OBJECTS)
