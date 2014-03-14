#!/usr/bin/env ruby

def one_test(c_src, args = {})
  begin
    puts "Looking for bugs in #{c_src} ..."
    puts "-" * 80

    abort "compilation problems..." \
      unless system("clang -I/usr/local/include/smack -I./include #{c_src} -c -emit-llvm -o a.o")

    system("~/Code/c2s/lib/c2s.rb a.o #{ARGV * " "}")
    puts "-" * 80

  ensure
    # File.unlink('a.o') if File.exists?('a.o')
  end
end

ARGV << "--verifier boogie_fi" if ARGV.grep(/--verifier/).empty?

puts "=" * 80
puts "Running CDS Experiments ..."
puts "=" * 80

one_test 'src/c/ours/TreiberStack.c'
one_test 'src/c/ours/TreiberStack-bugged.c'
one_test 'src/c/ours/EliminationStack.c'
