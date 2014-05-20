#!/usr/bin/env ruby

$compile = "clang++ -I./include -I./include/basekit -I./include/coroutine -L./lib -lcoroutine"

Dir.glob("src/**/*.{c,cpp}") do |cpp|
  puts "#{$compile} #{cpp}"
  system "#{$compile} #{cpp}"
  system "./a.out | grep \"[Vv]\""
  File.unlink "a.out"
end
