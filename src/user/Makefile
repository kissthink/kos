include ../Makefile.config

CXXFLAGS=-g -std=c++11
LDFLAGS=-static -L$(PREFIX)/x86_64-pc-kos/lib -lstdc++ -lc\
  -L$(PREFIX)/lib/gcc/x86_64-pc-kos/$(GCCVER) -lgcc -emain

SRC=$(wildcard *.cc)
EXE=$(SRC:%.cc=exec/%)

all: $(EXE)

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(EXE): exec/%: %.o
	mkdir -p exec
	$(LD) $(LDFLAGS) $< -o $@
	touch built

echo:
	@echo SRC: $(SRC)
	@echo EXE: $(EXE)

clean:
	rm -f built *.o $(EXE)
	rm -rf exec
