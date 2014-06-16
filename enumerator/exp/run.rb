#!/usr/bin/env ruby

def data_dir
  File.join File.dirname(__FILE__), "data"
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

def extract(output, data = {})
  lu = "Line-Up"
  oc = "Operation-Counting"
  
  _, data[:adds], data[:removes], data[:delays], data[:barriers] =
    /w\/ (\d+) adds, (\d+) removes, (\d+) delays, (\d+) barriers\./.match(output).to_a.map(&:to_i)

  _, data[:executions], data[:time] =
    /(\d+) schedules enumerated in (\d+)s\./.match(output).to_a.map(&:to_i)

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
  data = read_data("#{data_dir}/coverage.#{opts[:object]}.dat")
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
  set style line 1 lt 1 lc rgb "green"
  set style line 2 lt 1 lc rgb "green"
  set boxwidth 0.8
  set xlabel "Number of Barriers & Delays"
  set ylabel "Number of Histories"
  # set auto x
  set xtics 2,5,20
  set xtics add ("0 barriers" 2)
  set xtics add ("1 barrier" 7)
  set xtics add ("2 barriers" 12)
  set xtics add ("3 barriers" 17)
  set xtics add ("4 barriers" 22)
  set yrange [1:*]
  set grid y
  set tic scale 0
  plot \
    #{wrap(data)} using 11 title "Counting Violations" lc rgb "turquoise", \
    #{wrap(data)} using 9 title "Covered by Counting" lc rgb "aquamarine", \
    #{wrap(data)} using 8 title "All Violations" lc rgb "beige"
  xxx
end

def read_data(file)
  lines = File.readlines(file)
  titles = lines.shift.split.map{|t| t.split(":")[1].to_sym}
  lines.map{|l| titles.zip(l.split.map(&:to_i)).to_h}
end

def compare(object, adds, removes, delays, barriers)
  data = []
  puts titles(extract(""))
  (1..adds).each do |a|
    (1..removes).each do |r|
      (barriers+1).times do |b|
        (delays+1).times do |d|
          datum = extract(run(object, mode: "versus", adds: a, removes: r, delays: d, barriers: b))
          puts row(datum)
          data << datum
          one_plot(data, 'test.pdf')
        end
      end
    end
  end
end

# compare("bkq", 4, 4, 4, 4)

(2..4).each do |a|
  (2..4).each do |r|
    plot_history_coverage(object: :bkq, adds: a, removes: r)
  end
end

