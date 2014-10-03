#! /usr/bin/env ruby

require 'mkmf'

swig = find_executable('swig')
system(swig, '-c++', '-ruby', '-o', 'jellyfish_wrap.cxx', '../jellyfish.i')

$defs << `pkg-config --cflags jellyfish-2.0`.chomp << '-std=c++0x'
$libs << `pkg-config --libs jellyfish-2.0`.chomp
$libs << `pkg-config --libs-only-L jellyfish-2.0 | sed -e 's/-L/-Wl,-rpath,/g'`.chomp

create_makefile('jellyfish')
