#!/usr/bin/env ruby

def one_test(c_src, args = {})
  begin
    puts "Looking for bugs in #{c_src} ..."
    puts "-" * 80

    abort "compilation problems..." \
      unless system("clang -I/usr/local/include/smack -I./include #{c_src} -c -emit-llvm -o a.o")

    v = args[:verifier] || :boogie_fi
    d = args[:delays] || 0
    u = args[:unroll] || 1
    system("c2s a.o -i -u #{u} -d #{d} --verifier #{v}")
    puts "-" * 80

  ensure
    File.unlink('a.o') if File.exists?('a.o')
  end
end

puts "=" * 80
puts "Running CDS Experiments ..."
puts "=" * 80

one_test 'src/c/ours/TreiberStack.c', delays: 3, unroll: 3
one_test 'src/c/ours/TreiberStack-bugged.c', delays: 3, unroll: 3
one_test 'src/c/ours/EliminationStack.c', delays: 3, unroll: 3
