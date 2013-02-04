include Makefile.config

CXXFLAGS=-g -std=c++11
LDFLAGS=-static -L$(GCCDIR)/$(TARGET)/lib -lstdc++ -lc\
  -L$(GCCDIR)/lib/gcc/$(TARGET)/$(GCCVER) -lgcc -emain

SRC=$(wildcard *.cc)
EXE=$(SRC:%.cc=exec/%)

all: $(EXE)

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(EXE): exec/%: %.o
	mkdir -p exec
#	$(LD) $(LDFLAGS) $< -o $@
	$(CXX) $< -o $@
	touch built

echo:
	@echo SRC: $(SRC)
	@echo EXE: $(EXE)

clean:
	rm -f built *.o $(EXE)
	rm -rf exec
