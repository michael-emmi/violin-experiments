#!/usr/bin/env ruby

src = "src/c/ours/EliminationStack.c"
bc = "a.o"
name = "elim"

lines = File.readlines(src)
name = File.basename(src,".c")
num = 0

# the first one
base = "#{name}-#{num.to_s.rjust(2,"0")}"
File.write("#{base}.c", lines.join)
system("clang -I/usr/local/include/smack -DMEMORY_MODEL_NO_REUSE_IMPLS -I./include #{base}.c -c -emit-llvm -o #{bc}")
system("smackgen.py --verifier=boogie-plain #{bc} -o #{base}.bpl")
system("echo \"// @c2s-options --unroll 1\" >> #{base}.bpl")
system("echo \"// @c2s-options --rounds 3\" >> #{base}.bpl")
system("echo \"// @c2s-expected Verified\" >> #{base}.bpl")

lines.count.times do |i|
  next unless lines[i] =~ /\/\/\s*REACH/
  target = lines[i].sub(/\/\/\s*/,"")
  num += 1
  base = "#{name}-#{num.to_s.rjust(2,"0")}"
  content = lines[0..i-1] + [target] + lines[i+1..-1]
  File.write("#{base}.c", content.join)
  system("clang -I/usr/local/include/smack -DMEMORY_MODEL_NO_REUSE_IMPLS -I./include #{base}.c -c -emit-llvm -o #{bc}")
  system("smackgen.py --verifier=boogie-plain #{bc} -o #{base}.bpl")
  system("echo \"// @c2s-options --unroll 1\" >> #{base}.bpl")
  system("echo \"// @c2s-options --rounds 3\" >> #{base}.bpl")
  system("echo \"// @c2s-expected Got a trace\" >> #{base}.bpl")
end

