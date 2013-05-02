SRCDIR:=$(CURDIR)/..
include $(SRCDIR)/Makefile.config

CXXFLAGS=$(OPTFLAGS) $(DBGFLAGS) $(LANGFLAGS)
ifeq ($(CC),clang)
CXXFLAGS+=$(CLANGFLAGS)
endif
LDFLAGS=-static ../ulib/libKOS.o

SRC=$(wildcard *.cc)
OBJ=$(SRC:%.cc=%.o)
EXE=$(SRC:%.cc=exec/%)

all: $(EXE) exec/motd

$(OBJ): %.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(EXE): exec/%: %.o
	mkdir -p exec
	$(CXX) $(LDFLAGS) $< -o $@
#	strip $@

exec/motd:
	@echo creating $@
	@mkdir -p exec
	@echo > $@
	@echo "Hello everybody! So glad to see you..." >> $@
	@echo >> $@
	@echo -n "Build Date: " >> $@
	@date >> $@
	@echo >> $@

built: $(EXE) exec/motd
	touch built

echo:
	@echo SRC: $(SRC)
	@echo OBJ: $(OBJ)
	@echo EXE: $(EXE)

clean:
	rm -f $(OBJ) $(EXE) exec/motd built
	rm -rf exec
	rm -f Makefile.dep

dep depend Makefile.dep:
	$(CXX) -MM $(CXXFLAGS) $(SRC) > $@

-include Makefile.dep
