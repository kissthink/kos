SRCDIR:=$(CURDIR)/..
include $(SRCDIR)/Makefile.config

CXXFLAGS=$(OPTFLAGS) $(DBGFLAGS) $(LANGFLAGS)
ifeq ($(CC),clang)
CXXFLAGS+=-target x86_64-pc-kos
endif
LDFLAGS=-static ../ulib/libKOS.o

SRC=$(wildcard *.cc)
OBJ=$(SRC:%.cc=%.o)
EXE=$(SRC:%.cc=exec/%)

all: $(EXE)

$(OBJ): %.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(EXE): exec/%: %.o
	mkdir -p exec
	$(CXX) $(LDFLAGS) $< -o $@
#	strip $@

built: $(EXE)
	touch built

echo:
	@echo SRC: $(SRC)
	@echo OBJ: $(OBJ)
	@echo EXE: $(EXE)

clean:
	rm -f $(OBJ) $(EXE) built
	rm -rf exec
	rm -f Makefile.dep

dep depend Makefile.dep:
	$(CXX) -MM $(CXXFLAGS) $(SRC) > $@

-include Makefile.dep
