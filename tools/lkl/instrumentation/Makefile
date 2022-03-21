CC=clang
INCLUDES=-I. -I../staticlib -I../metapagetable
CFLAGS=-c -Werror -Wall -std=c++0x -g `llvm-config --cxxflags` -O0 -fPIC
LDFLAGS=-g -shared

all: libPluginExample.so test

test: libPluginExample.so
	tests/tester.sh

lib%.so: %.o
	$(CC) $(LDFLAGS)  $< -o $@

%.o: %.cpp
	$(CC) $(CFLAGS) $< -c -o $@
