CC = g++
ODIR ?= build
IDIR = -Ilib

LIB := utils
LIBPATH := $(ODIR)/lib$(LIB).so

SRCS := $(wildcard *.cpp)
OUTS := $(patsubst %.cpp, $(ODIR)/%.out, $(SRCS))

all: $(LIBPATH) $(OUTS)

$(LIBPATH): lib/*.cpp
	mkdir -p $(ODIR)
	$(CC) -o $@ -shared -fPIC $^ -Wl,-rpath='pwd'/$(ODIR)

$(ODIR)/%.out: %.cpp $(LIBPATH)
	mkdir -p $(ODIR)
	$(CC) -o $@ $< -L$(ODIR) -l$(LIB) $(IDIR) -Wl,-rpath=.

clean:
	rm -rf $(ODIR)
	rm blocks_info.txt
