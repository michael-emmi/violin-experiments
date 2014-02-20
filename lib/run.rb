#!/usr/bin/env ruby

def one_test(c_src, args = {})
  puts "Looking for bugs in #{c_src} ..."
  puts "-" * 80

  abort "compilation problems..." \
    unless system("clang -I/usr/local/include/smack #{c_src} -c -emit-llvm -o a.o")
    

  abort "SMACK problems..." unless system("smack a.o -mem-mod-dbg") 
    
  system("rm a.o")
  system("cat #{File.dirname(__FILE__)}/monitor.bpl >> a.bpl")

  if search('a.bpl', args) then
    puts "Found a bug w/ #{args[:delays]} delays and inline depth #{args[:unroll]}."
  else
    puts "Did not find a bug."
  end
  system("rm a.bpl")
  puts "-" * 80
end

def check(bpl_src, args = {})
  d = args[:delays] || 0
  u = args[:unroll] || 1
  v = args[:verifier] || :boogie_si
  
  command = "violin.rb #{bpl_src} --verifier #{v} -u #{u} -d #{d} -v -k"
  
  output = `#{command} 2>&1`

  res = output.match /(\d+) verified, (\d+) errors?/ do |m| m[2].to_i > 0 end
  warn "unexpected Boogie result: #{output}" if res.nil?
  
  res = nil if output.match(/ \d+ time outs?/)  
    
  time = output.match /Boogie finished in ([0-9.]+)s./ do |m| m[1].to_f end
  warn "unknown Boogie time" unless time
  
  puts "#{res.nil? ? "TO" : res} / #{time} / #{args.reject{|k,_| k =~ /limit/}}"
  return res, time
end

def search(bpl_src, args = {})
  args[:delays] ||= 0
  args[:unroll] ||= 1
  args[:delay_limit] ||= 6
  args[:unroll_limit] ||= 6
  
  res = nil
  
  while args[:delays] < args[:delay_limit] &&
        args[:unroll] < args[:unroll_limit] do

    res, time = check(bpl_src,args)
    break if res
    
    if args[:delays] < args[:unroll] then
      args[:delays] += 1
    else
      args[:unroll] += 1
    end

  end
  
  res
end

puts "=" * 80
puts "Running CDS Experiments ..."
puts "=" * 80

# one_test 'src/c/ours/TreiberStack.c'
# one_test 'src/c/ours/TreiberStack-bugged.c'

one_test 'src/c/ours/EliminationStack.c', verifier: :boogie_fi
