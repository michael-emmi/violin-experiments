#!/usr/bin/env ruby

def one_test(c_src, args = {})
  begin
    puts "Looking for bugs in #{c_src} ..."
    puts "-" * 80
    
    frontend = "clang -I/usr/local/include/smack -I./include #{c_src} -c -emit-llvm -o a.o"
    c2s = "~/Code/c2s/lib/c2s.rb a.o #{ARGV * " "}"
    
    puts frontend if $verbose
    abort "compilation problems..." unless system(frontend)
    puts c2s if $verbose
    abort "c2s failed..." unless system(c2s)
    puts "-" * 80

  ensure
    File.unlink('a.o') if File.exists?('a.o') unless $keep
  end
end

$keep = !ARGV.grep(/^(-k|--keep)$/).empty?
$verbose = !ARGV.grep(/^(-v|--verbose)$/).empty?
ARGV.reject!{|arg| arg =~ /--counting/}
ARGV << "--verifier boogie_fi" if ARGV.grep(/--verifier/).empty?


puts "=" * 80
puts "Running CDS Experiments ..."
puts "=" * 80

# one_test 'src/c/ours/TreiberStack.c'
# one_test 'src/c/ours/TreiberStack-bugged.c'
# one_test 'src/c/ours/EliminationStack-scratch.c'

one_test 'src/c/ours/BasketsQueue.c'
