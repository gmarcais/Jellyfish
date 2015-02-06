#! /usr/bin/env ruby

require 'jellyfish'

mf = Jellyfish::ReadMerFile.new(ARGV[0])
mf.each { |mer, count|
  print(mer, " ", count, "\n")
}

