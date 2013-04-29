SRCDIR:=$(CURDIR)/../..
include $(SRCDIR)/Makefile.config

#CXXFLAGS+=-DHAVE_MMAP=0 -DNO_MALLOC_STATS -Dmalloc_getpagesize=4096\
#	-DUSE_DL_PREFIX=1 -DLACKS_TIME_H -DABORT_ON_ASSERT_FAILURE=0 -DDEBUG

CXXFLAGS+=-include malloc_glue.h -DDEBUG

SRC=$(wildcard *.c)
OBJ=$(SRC:%.c=%.o)

all: $(OBJ)

$(OBJ): %.o: %.c
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ)
