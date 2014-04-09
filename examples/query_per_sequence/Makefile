CC = g++
CXXFLAGS = $(shell pkg-config --cflags jellyfish-2.0) -std=c++0x -Wall -Werror -O3
LDFLAGS = $(shell pkg-config --libs jellyfish-2.0)

all: query_per_sequence
query_per_sequence: query_per_sequence.cc sequence_mers.hpp
clean:
	rm -f *.o query_per_sequence


