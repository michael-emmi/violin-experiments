#!/usr/bin/env ruby

def one_test(c_src, args = {})
  puts "Looking for bugs in #{c_src} ..."
  puts "-" * 80

  abort "compilation problems..." \
    unless system("clang -I/usr/local/include/smack -I./include #{c_src} -c -emit-llvm -o a.o")

  v = args[:verifier] || :boogie_fi
  d = args[:delays] || 0
  u = args[:unroll] || 1
  system("c2s a.o -i -p -u #{u} -d #{d} --verifier #{v}")
  File.unlink 'a.o'
  puts "-" * 80
end

puts "=" * 80
puts "Running CDS Experiments ..."
puts "=" * 80

one_test 'src/c/ours/TreiberStack.c', delays: 3, unroll: 3
one_test 'src/c/ours/TreiberStack-bugged.c', delays: 3, unroll: 3
one_test 'src/c/ours/EliminationStack.c', delays: 3, unroll: 3
