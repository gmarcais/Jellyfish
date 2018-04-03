#! /usr/bin/env ruby

require 'mkmf'

def pkgconfig(s)
  res = `pkg-config #{s}`
  if $? != 0
    STDERR.puts("\nCan't get compilation information for Jellyfish, check you PKG_CONFIG_PATH\n");
    exit(1)
  end
  res.chomp
end

$defs << pkgconfig("--cflags jellyfish-2.0") << '-std=c++0x'
libs = [pkgconfig("--libs jellyfish-2.0"),
        pkgconfig("--libs-only-L jellyfish-2.0").gsub(/-L/, "-Wl,-rpath,")]

if Array === $libs
  $libs += libs
elsif String === $libs
  $libs += " " + libs.join(" ")
else
  $libs = libs.join(" ")
end

create_makefile('jellyfish')
