CC = g++
CXXFLAGS = $(shell pkg-config --cflags jellyfish-2.0) -std=c++0x -Wall -O3
LDFLAGS = $(shell pkg-config --libs jellyfish-2.0) -Wl,--rpath=$(shell pkg-config --libs-only-L jellyfish-2.0 | sed -e 's/-L//g')

all: jf_count_dump
clean:
	rm -f *.o jf_count_dump
