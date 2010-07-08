#! /usr/bin/env ruby

$brk_base = nil
$brk_max = nil
$mmaped = 0

require 'optparse'

$opts = {
  :ignore => false,
  :all => false,
  :verbose => false,
  :current => false,
  :debug => false,
  :unbalanced => false,
}

parser = OptionParser.new do |o|
  o.banner = "#{File.basename($0)} [options] trace_file"

  o.on("--ignore-unknown", "Ignore unknown lines") { 
    $opts[:ignore] = true
  }
  o.on("--verbose", "Print details for all subprocesses") {
    $opts[:verbose] = true
  }
  o.on("--all", "Include read only maps in count") {
    $opts[:all] = true
  }
  o.on("--current", "Display current counts as well") {
    $opts[:current] = true
  }
  o.on("--unbalanced", "Display unbalanced mmap/munmap calls") {
    $opts[:unbalanced] = true
  }
  o.on("--debug", "Print debugging information") {
    $opts[:debug] = true
  }
  o.on_tail("--help", "This message") {
    puts(o)
    exit(0)
  }
end
parser.parse!(ARGV)

class String
  def parse_int
    case self
    when "NULL"
      0
    when /^0x/
      self.to_i(16)
    when /^0/
      self.to_i(8)
    else
      self.to_i
    end
  end
end

class Numeric
  SI_LARGE_SUFFIX = [' ', 'k', 'M', 'G', 'T', 'P', 'E', 'Z', 'Y']
  SI_SMALL_SUFFIX = ['m', 'u', 'n', 'p', 'f', 'a', 'z', 'y']
  def to_SI(multiple = 1000.0)
    nb = self.to_f
    neg = nb < 0
    nb = nb.abs
    return "0 " if nb == 0.0
    multiple = multiple.to_f
    if nb >= 1.0
      SI_LARGE_SUFFIX.each do |s|
        return("%s%.3g%s" % [neg ? "-" : "", nb, s]) if nb < multiple
        nb = nb / multiple
      end
    else
      SI_SMALL_SUFFIX.each do |s|
        nb *= multiple
        return("%s%.3g%s" % [neg ? "-" : "", nb, s]) if nb >= 1.0
      end
    end
  end
end

def print_report(title, heap, mapped, vm)
  puts("%10s: heap %10d %8s mmap %10d %8s vm %10d %8s" %
       [title[0, 10], heap, "(#{heap.to_SI(1024)})",
        mapped, "(#{mapped.to_SI(1024)})",
        vm, "(#{vm.to_SI(1024)})"])
end


class ProcessStats
  def initialize(known_addr)
    @brk_base = @brk_cur = nil
    @heap = @heap_max = 0
    @mmaped = @mmaped_max = 0
    @vm = @vm_max = 0
    @known_addr = known_addr
  end
  
  attr_reader :heap_max, :mmaped_max, :vm_max
  attr_reader :heap, :mmaped, :vm

  def update_vm
    @vm = @heap + @mmaped
    @vm_max = @vm if @vm > @vm_max
  end

  def brk(args, res, line)
    addr = args.parse_int
    res = res.parse_int
    if addr == 0
      @brk_cur = @brk_base = res
    else
      @brk_cur = res
      @heap = @brk_cur - @brk_base
      @heap_max = @heap if @heap > @heap_max
    end
    update_vm
  end
  
  def mmap(args, res, line)
    res = res.split[0].parse_int
    return if res == -1
    addr, length, prot, flags, fd, off = args.split(/\s*,\s*/)
    prot = prot.split(/\s*\|\s*/)
    if $opts[:all] || prot.include?("PROT_WRITE")
      @mmaped += length.parse_int
      @mmaped_max = @mmaped if @mmaped > @mmaped_max
      (@known_addr[res] ||= []) << line
    end
    update_vm
  end

  def mremap(args, res, line)
    res = res.split[0].parse_int
    return if res == -1
    old_address, old_size, new_size = args.split(/\s*,\s*/)
    return unless @known_addr.delete(old_address)
    @mmaped += new_size - old_size
    @mmaped_max = @mmaped if @mmaped > @mmaped_max
    (@known_addr[res] ||= []) << line
  end

  def munmap(args, res, line)
    ires = res.split[0].parse_int
    return if ires == -1
    addr, length = args.split(/\s*,\s*/)
    addr = addr.parse_int
    if @known_addr.delete(addr)
      @mmaped -= length.parse_int
    end
    update_vm
  end

  def method_missing(meth, *args)
    unless $opts[:ignore]
      raise "Unknown system call #{meth} '#{args.inspect}'"
    end
  end
end

class ProcessStatHash
  def initialize
    @known_addr = {}
    @hash = Hash.new { |h, k| h[k] = ProcessStats.new(@known_addr) }
    @heap = @mmaped = @vm = 0
    @heap_max = @mmaped_max = @vm_max = 0
  end
  attr_reader :heap, :mmaped, :vm
  attr_reader :heap_max, :mmaped_max, :vm_max
  attr_reader :known_addr

  def update(pid, cmd, args, res, line)
    ps = @hash[pid]
    ps_heap, ps_mmaped, ps_vm = ps.heap, ps.mmaped, ps.vm
    ps.__send__(cmd, args, res, line)
    @heap += ps.heap - ps_heap
    @mmaped += ps.mmaped - ps_mmaped
    @vm += ps.vm - ps_vm
    @heap_max = @heap if @heap > @heap_max
    @mmaped_max = @mmaped if @mmaped > @mmaped_max
    @vm_max = @vm if @vm > @vm_max
  end

  def each(&block); @hash.each(&block); end
end

pss = ProcessStatHash.new
line_buf = {}

LRGEXP = /^(\d+\s*)?(\w+)\s*\(\s*([^)]*)\s*\)\s*=\s*(.*)$/
UNFEXP = /^(\d+\s*)?(.*?) <unfinished ...>$/
RESEXP = /^(\d+\s*)?<... \w+ resumed> (.*)$/
open(ARGV[0]) do |fd|
  i = 0
  fd.each do |l|
    case l
    when UNFEXP
      pid = $1 ? $1.chomp.parse_int : 0
      line_buf[pid] = "#{pid} #{$2}"
      next

    when RESEXP
      pid = $1 ? $1.chomp.parse_int : 0
      line = line_buf[pid] + $2
    else
      line = l
    end

    unless line =~ LRGEXP
      raise "Line did not parse: #{line}" unless $opts[:ignore]
      next
    end

    i += 1
    pid, cmd, args, res = $1, $2, $3, $4
    pid = pid ? pid.chomp.parse_int : 0
    pss.update(pid, cmd, args, res, l)
    if $opts[:debug]
      p [pid, cmd, args, res]
      print_report("cur_#{i}", pss.heap, pss.mmaped, pss.vm)
      print_report("max_#{i}", pss.heap_max, pss.mmaped_max, pss.vm_max)
    end
  end
end

if $opts[:verbose]
  pss.each do |pid, ps|
    print_report("cur_#{pid}", ps.heap, ps.mmaped, ps.vm) if $opts[:current]
    print_report("max_#{pid}", ps.heap_max, ps.mmaped_max, ps.vm_max)
  end
end


if $opts[:current]
  print_report("current", pss.heap, pss.mmaped, pss.vm)
end
print_report("max", pss.heap_max, pss.mmaped_max, pss.vm_max)

if $opts[:unbalanced]
  puts("Unbalanced: ")
  pss.known_addr.each { |k, v|
    puts("0x%16x %s" % [k, v.shift])
    v.each { |l| puts("                   #{l}") }
  }
end
