CC = g++
CXXFLAGS = $(shell pkg-config --cflags jellyfish-2.0) -std=c++0x -Wall -O3
LDFLAGS = -Wl,--rpath=$(shell pkg-config --libs-only-L jellyfish-2.0 | sed -e 's/-L//g')
LDLIBS = $(shell pkg-config --libs jellyfish-2.0)

all: jf_count_dump
clean:
	rm -f *.o jf_count_dump
