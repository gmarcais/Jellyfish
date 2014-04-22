#! /usr/bin/env ruby

$: << "."
require 'jellyfish'

# Jellyfish::MerDNA::k(10)
# m = Jellyfish::MerDNA.new

# puts m
# rm = m.get_reverse_complement
# puts rm
# puts m == rm
# puts m < rm
# puts rm < m

# 10.times {
#   m.randomize
#   p m
# }

# Jellyfish::MerDNA::k.times { |i|
#   print m[i], " "
#   m[i] = ["A", "C", "G", "T"][i % 4]
# }
# puts
# puts m

# m.set("ACGTTGCAAT")
# puts m

mf = Jellyfish::QueryMerFile.new("../tests/parallel_hashing_binary.jf")
puts Jellyfish::MerDNA::k

rf = Jellyfish::ReadMerFile.new("../tests/parallel_hashing_binary.jf")

while(rf.next2) do
  puts("#{rf.key} #{rf.val} #{mf[rf.key]}")
end

