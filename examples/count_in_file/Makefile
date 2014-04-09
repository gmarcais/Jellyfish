CC = g++
CXXFLAGS = $(shell pkg-config --cflags jellyfish-2.0) -Wall -Werror -O3 -std=c++0x
LDFLAGS = $(shell pkg-config --libs jellyfish-2.0)

all: count_in_file
clean:
	rm -f *.o count_in_file
