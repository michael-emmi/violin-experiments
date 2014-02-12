#!/usr/bin/env ruby

def one_test(c_src)
  system("clang -I/usr/local/include/smack #{c_src} -c -emit-llvm -o a.o")
  system("smack a.o")
  system("rm a.o")
  system("cat #{File.dirname(__FILE__)}/monitor.bpl >> a.bpl")
  system("violin.rb a.bpl --verifier boogie_fi -l 3 -d 3")
  system("rm a.bpl")
end

one_test 'src/c/ours/TreiberStack.c'
