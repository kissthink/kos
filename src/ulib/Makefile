SRCDIR:=$(CURDIR)/..
include $(SRCDIR)/Makefile.config

CXXFLAGS=$(OPTFLAGS) $(DBGFLAGS) $(LANGFLAGS)
ifeq ($(CC),clang)
CXXFLAGS+=-target x86_64-pc-kos
endif

SRC=$(wildcard *.cc)
OBJ=$(SRC:%.cc=%.o)
LIB=libKOS.a

all: $(LIB)

$(OBJ): %.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(LIB): $(OBJ)
	$(AR) cr $@ $^

echo:
	@echo SRC: $(SRC)
	@echo OBJ: $(OBJ)
	@echo LIB: $(LIB)

clean:
	rm -f $(OBJ) $(LIB)
	rm -f Makefile.dep

dep depend Makefile.dep:
	$(CXX) -MM $(CXXFLAGS) $(SRC) > $@

-include Makefile.dep
