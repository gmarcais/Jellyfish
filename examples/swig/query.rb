#! /usr/bin/env ruby

require 'jellyfish'

qf = Jellyfish::QueryMerFile.new(ARGV[0])
ARGV[1..-1].each { |s|
  print(s, " ", qf[Jellyfish::MerDNA.new(s)], "\n")
}

