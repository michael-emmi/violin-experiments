#!/usr/bin/env ruby

def data_dir
  File.join File.dirname(__FILE__), "data"
end

def runtime_data_file(obj)
  File.join data_dir, "runtime.#{obj}.dat"
end

def coverage_data_file(obj)
  File.join data_dir, "coverage.#{obj}.dat"
end

def graphs_dir
  File.join File.dirname(__FILE__), "graphs"
end

def scal_exe
  File.join File.dirname(__FILE__), "../src/scal/scal"
end

def scal(object, options = {})
  cmd = "#{scal_exe} #{object}"
  cmd << " -mode #{options[:mode]}" if options[:mode]
  cmd << " -show #{options[:show]}" if options[:show]
  cmd << " -adds #{options[:adds]}" if options[:adds]
  cmd << " -removes #{options[:removes]}" if options[:removes]
  cmd << " -delays #{options[:delays]}" if options[:delays]
  cmd << " -barriers #{options[:barriers]}" if options[:barriers]
  cmd
end

def object_names; {
  bkq: "Bounded-Size K FIFO",
  dq: "Distributed Queue",
  dtsq: "DTS Queue",
  fcq: "Flat-Combining Queue",
  lbq: "Lock-Based Queue",
  msq: "Michael-Scott Queue",
  rdq: "Random-Dequeue Queue",
  sl: "Single List",
  ts: "Treiber Stack",
  ukq: "Unbounded-Size K FIFO",
  wfq11: "Wait-free Queue (2011)",
}
end

def run(object, options = {})
  `#{scal(object,options)}`
end

def extract_basic_data(output, data = {})  
  _, data[:adds], data[:removes], data[:delays], _, data[:barriers] =
    /w\/ (\d+) adds, (\d+) removes, (\d+) delays(, (\d+) barriers)?\./.match(output).to_a.map(&:to_i)
  data[:executions], data[:time] =
    /(\d+) schedules enumerated in ([0-9.]+)s\./.match(output){|m| [m[1].to_i, m[2].to_f]}
  data
end

def extract_runtime_data(output, data = {})
  lu = "Line-Up"
  oc = "Operation-Counting"
  data = extract_basic_data(output)
  _, data[:mode] = /(.*) mode w\/ \d+ adds,/.match(output).to_a
  data
end

def extract_coverage_data(output, data = {})
  lu = "Line-Up"
  oc = "Operation-Counting"
  data = extract_basic_data(output)
  _, data[:bad_executions], data[:bad_histories], data[:covered] =
    /#{lu} found (\d+) violations \/ (\d+) histories; #{oc} covers (\d+)\./.match(output).to_a.map(&:to_i)
  _, data[:c_executions], data[:c_histories] =
    /#{oc} found (\d+) violations \/ (\d+) histories\./.match(output).to_a.map(&:to_i)
  data
end

def row(data)
  data.map{|_,v| v}.join(" ")
end

def titles(data)
  data.map(&:first).map.with_index{|k,i| "#{i+1}:#{k}"}.join(" ")
end

def gnuplot(commands)
  IO.popen("gnuplot -p", "w") do |io|
    io.puts commands
  end
end

def wrap(data)
  "\"< echo '#{data.select{|d| if block_given? then yield d else true end}.
  map(&:values).map{|d| d.join(' ')}.join("\\n")}'\""
end

def plot_history_coverage(opts = {})
  data = read_data(coverage_data_file(opts[:object]))
  data.sort! do |a,b|
    next -1 if a[:delays] < b[:delays]
    next 1 if a[:delays] > b[:delays]
    next -1 if a[:barriers] < b[:barriers]
    next 1 if a[:barriers] > b[:barriers]
    next 0
  end
  data.select!{|d| d[:adds] == opts[:adds] && d[:removes] == opts[:removes]}
  data.each{|d| d[:bad_histories] -= d[:covered]; d[:covered] -= d[:c_histories]}
  gnuplot <<-xxx
  set terminal pdf
  set output '#{graphs_dir}/coverage.#{opts[:object]}.#{opts[:adds]}x#{opts[:removes]}.pdf'
  set title '#{object_names[opts[:object]]} w/ #{opts[:adds]} adds & #{opts[:removes]} removes'
  # set key invert
  set key box opaque top left
  set style data histogram
  set style histogram rows
  # set style histogram clustered gap 0
  # set logscale y # cannot use logscale with histogram rows
  set style fill solid border rgb "black"
  set boxwidth 0.8
  set xlabel "Number of Barriers & Delays"
  set ylabel "Number of Histories"
  # set auto x
  set xtics 2,5,20
  set xtics add ("0 delays" 2)
  set xtics add ("1 delays" 7)
  set xtics add ("2 delays" 12)
  set xtics add ("3 delays" 17)
  set xtics add ("4 delays" 22)
  set yrange [1:*]
  set grid y
  set tic scale 0
  plot \
    #{wrap(data)} using 11 title "Counting Violations" lc rgb "turquoise", \
    #{wrap(data)} using 9 title "Covered by Counting" lc rgb "aquamarine", \
    #{wrap(data)} using 8 title "All Violations" lc rgb "beige"
  xxx
end

def plot_runtimes(opts = {})
  graph_file = File.join graphs_dir,
    "runtime.#{opts[:object]}.#{opts[:adds]}x#{opts[:removes]}.pdf"
  data = read_data(runtime_data_file(opts[:object]))
  data.select!{|d| d[:delays] > 1 && d[:adds] > 1 && d[:removes] > 1}
  data.sort! do |a,b|
    next -1 if a[:delays] < b[:delays]
    next 1 if a[:delays] > b[:delays]
    next -1 if a[:adds] < b[:adds]
    next 1 if a[:adds] > b[:adds]
    next -1 if a[:removes] < b[:removes]
    next 1 if a[:removes] > b[:removes]
    next 0
  end
  data.each{|d| d[:executions] = d[:executions] / 1000.0}
  # data = data.sort_by{|d| d[:delays]}
  # data.select!{|d| d[:adds] == opts[:adds] && d[:removes] == opts[:removes]}
  raw_data = data.select{|d| d[:mode] =~ /Unmonitored/}
  cnt_data = data.select{|d| d[:mode] =~ /Counting/}
  lin_data = data.select{|d| d[:mode] =~ /Linearization/}
  gnuplot <<-xxx
  set terminal pdf
  set output '#{graph_file}'
  set title '#{object_names[opts[:object]]}'
  set key box opaque top left
  set style data lines
  # set style data histogram
  # set style histogram rows
  # set style histogram clustered gap 0
  set style fill transparent solid 0.5 border rgb "black"
  # set style fill solid border rgb "black"
  set boxwidth 0.8
  set xlabel "Increasing delays & operations"
  set ylabel "Execution Time"
  set logscale y
  set xtics 4,9,27
  set xtics add ("2 delays" 4)
  set xtics add ("3 delays" 13)
  set xtics add ("4 delays" 22)
  # set yrange [0.01:*]
  set grid y
  set tic scale 0
  plot \
    #{wrap(lin_data)} using 5 title "Executions/1K" lc rgb "beige" w filledcurve x1, \
    #{wrap(lin_data)} using 6 title "Linearization" lc rgb "coral" w filledcurve x1, \
    #{wrap(cnt_data)} using 6 title "Operation Counting" lc rgb "aquamarine" w filledcurve x1, \
    #{wrap(raw_data)} using 6 title "No monitoring" lc rgb "turquoise" w filledcurve x1
  xxx
end

def read_data(file)
  lines = File.readlines(file)
  titles = lines.shift.split.map{|t| t.split(":")[1].to_sym}
  lines.map{|l| titles.zip(l.split.map do |i|
    case i
    when /^\d+$/; i.to_i
    when /^[0-9.]+$/; i.to_f
    else i
    end
  end).to_h}
end

def generate_data(data_file,opts)
  data = []
  opts[:adds] ||= (1..1)
  opts[:removes] ||= (1..1)
  opts[:barriers] ||= (0..0)
  opts[:delays] ||= (0..0)
  opts[:mode] ||= ["none"]
  File.open(data_file, "w") do |file|
    file.puts titles(yield nil)
    opts[:adds].each do |a|
      opts[:removes].each do |r|
        opts[:delays].each do |d|
          opts[:barriers].each do |b|
            opts[:modes].each do |m|
              file.puts(row(yield run(
                opts[:object], mode: m, show: "none",
                adds: a, removes: r, delays: d, barriers: b
              )))
              file.flush
            end
          end
        end
      end
    end
  end
end

def generate_coverage_data(opts)
  generate_data(coverage_data_file(opts[:object]),opts){|d| extract_coverage_data(d)}
end
  
def generate_runtime_data(opts)
  generate_data(runtime_data_file(opts[:object]),opts){|d| extract_runtime_data(d)}
end

# generate_coverage_data(
#   object: :bkq, mode: "versus",
#   adds: 1..4, removes: 1..4, delays: 0..4, barriers: 0..4)
  
# generate_runtime_data(
#   object: :bkq, modes: ["none","count","lin"],
#   adds: 1..4, removes: 1..4, delays: 0..4, barriers: 2..2)

plot_runtimes(object: :bkq)


# (2..4).each do |a|
#   (2..4).each do |r|
#     plot_history_coverage(object: :bkq, adds: a, removes: r)
#   end
# end

