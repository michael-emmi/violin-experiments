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

def timeout(t)
  "gtimeout #{t}"
end

def run_scal(object, options = {})
  cmd = ""
  cmd << "#{timeout(options[:timeout])}" if options[:timeout]
  cmd << " #{scal_exe} #{object}"
  cmd << " -mode #{options[:mode]}" if options[:mode]
  cmd << " -show #{options[:show]}" if options[:show]
  cmd << " -adds #{options[:adds]}" if options[:adds]
  cmd << " -removes #{options[:removes]}" if options[:removes]
  cmd << " -delays #{options[:delays]}" if options[:delays]
  cmd << " -barriers #{options[:barriers]}" if options[:barriers]
  `#{cmd}`
end

def object_name(abbrv) {
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
}[abbrv]
end

def default_data_patterns; {
  mode: /^(.*) mode /,
  adds: / (\d+) adds/,
  removes: / (\d+) removes/,
  delays: / (\d+) delays/,
  barriers: / (\d+) barriers/,
  executions: /^(\d+) schedules enumerated/,
  time: / enumerated in ([0-9.]+)s/,
}
end

def interpret(str)
  case str
  when nil; '-'
  when /^\d+[.]\d+$/; str.to_f
  when /^\d+$/; str.to_i
  else str.gsub(/-/,'_')
  end
end

def extract(output, patterns = default_data_patterns, data = {})
  patterns.each do |key,pat|
    data[key] = interpret((pat.match(output) || [])[1])
  end
  data
end

def titles(data)
  data.map(&:first).map.with_index{|k,i| "#{i+1}:#{k}"}.join(" ")
end

def row(data)
  data.map{|_,v| v}.join(" ")
end

def generate_data(data_file, data_patterns = default_data_patterns, opts)
  data = []
  opts[:adds] ||= (1..1)
  opts[:removes] ||= (1..1)
  opts[:barriers] ||= (0..0)
  opts[:delays] ||= (0..0)
  opts[:modes] ||= ["none"]
  opts[:repeat] ||= 1
  File.open(File.join(data_dir,data_file), "w") do |file|
    file.puts titles(extract(nil,data_patterns))
    opts[:repeat].times do
      opts[:delays].each do |d|
        puts "* generating data for #{opts[:object]} w/ #{d} delays..."
        opts[:adds].each do |a|
          opts[:removes].each do |r|
            opts[:barriers].each do |b|
              opts[:modes].each do |m|
                output = run_scal(
                  opts[:object], mode: m, show: "none",
                  adds: a, removes: r, delays: d, barriers: b
                )
                file.puts(row(extract(output,data_patterns)))
                file.flush
              end
            end
          end
        end
      end
    end
  end
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

def gnuplot(commands)
  IO.popen("gnuplot -p", "w") do |io|
    io.puts commands
  end
end

def wrap(data)
  "\"< echo '#{data.select{|d| if block_given? then yield d else true end}.
  map(&:values).map{|d| d.join(' ')}.join("\\n")}'\""
end

def color(c)
  c = [ :beige, :acquamarine, :turquoise, :coral ][c] if c.is_a?(Integer)
  "lc rgb \"#{c}\""
end

def plot(data_file, graph_file)
  return unless block_given?
  data = read_data(File.join data_dir, data_file)
  gnuplot(yield data, File.join(graphs_dir, graph_file))
end

################################################################################
####    RUNTIME   TESTS:    NONE    VS.   COUNTING   VS.   LINEARIZATION    ####
################################################################################

def generate_runtime_data(opts)
  obj = opts[:object]
  generate_data "runtime.#{obj}.dat", default_data_patterns, opts
end

def plot_runtimes(opts = {})
  obj = opts[:object]
  adds = opts[:adds]
  removes = opts[:removes]

  plot("runtime.#{obj}.dat", "runtime.#{obj}.#{adds}x#{removes}.pdf") do |data,graph|
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
    # data.select!{|d| d[:adds] == adds && d[:removes] == removes}
    raw_data = data.select{|d| d[:mode] =~ /Unmonitored/}
    cnov_data = data.select{|d| d[:mode] =~ /Counting -- no verify/}
    cnt_data = data.select{|d| d[:mode] =~ /Counting$/}
    lin_data = data.select{|d| d[:mode] =~ /Linearization/}
    <<-xxx
    set terminal pdf
    set output '#{graph}'
    set title '#{object_name(obj)}'
    set key box opaque top left
    set style data lines
    set style fill transparent solid 0.5 border rgb "black"
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
      #{wrap(lin_data)} using 5 title "Executions/1K" #{color(0)} w filledcurve x1, \
      #{wrap(lin_data)} using 6 title "Linearization" #{color(3)} w filledcurve x1, \
      #{wrap(cnt_data)} using 6 title "Operation Counting" #{color(1)} w filledcurve x1, \
      #{wrap(cnov_data)} using 6 title "Operation Counting" #{color(1)} w filledcurve x1, \
      #{wrap(raw_data)} using 6 title "No monitoring" #{color(2)} w filledcurve x1
    xxx
  end
end

################################################################################
####    COVERAGE     TESTS:    K    BARRIERS     VS.     LINEARIZATION      ####
################################################################################

def coverage_data_patterns
  lu = "Line-Up"
  oc = "Operation-Counting"
  default_data_patterns.merge({
    bad_executions: /#{lu} found (\d+) violations/,
    bad_histories: /#{lu} found \d+ violations \/ (\d+) histories/,
    covered: /histories; #{oc} covers (\d+)\./,
    c_executions: /#{oc} found (\d+) violations/,
    c_histories: /#{oc} found \d+ violations \/ (\d+) histories/,
  })
end

def generate_coverage_data(opts)
  obj = opts[:object]
  generate_data "coverage.#{obj}.dat", coverage_data_patterns, opts
end

def plot_history_coverage(opts = {})
  obj = opts[:object]
  adds = opts[:adds]
  removes = opts[:removes]

  plot("coverage.#{obj}.dat", "coverage.#{obj}.#{adds}x#{removes}.pdf") do |data,graph|
    data.sort! do |a,b|
      next -1 if a[:delays] < b[:delays]
      next 1 if a[:delays] > b[:delays]
      next -1 if a[:barriers] < b[:barriers]
      next 1 if a[:barriers] > b[:barriers]
      next 0
    end
    data.select!{|d| d[:adds] == adds && d[:removes] == removes}
    data.each{|d| d[:bad_histories] -= d[:covered]; d[:covered] -= d[:c_histories]}
    <<-xxx
    set terminal pdf
    set output '#{graph}'
    set title '#{object_name(obj)} w/ #{adds} adds & #{removes} removes'
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
      #{wrap(data)} using 11 title "Counting Violations" #{color(2)}, \
      #{wrap(data)} using 9 title "Covered by Counting" #{color(1)}, \
      #{wrap(data)} using 8 title "All Violations" #{color(0)}
    xxx
  end
end

################################################################################
####  STRESS TESTS  : LINEARIZATION  VS. INCREASING  NUMBER OF  OPERATIONS  ####
################################################################################

def stress_data_patterns
  lu = "Line-Up"
  default_data_patterns.merge({
    seq_histories: /(\d+) histories computed/,
    seq_time: /histories computed in ([0-9.]+s)/,
    avg_lins: /#{lu} performed (\d+) \(avg\)/,
    max_lins: / (\d+) \(max\) linearizations/,
  })
end

def generate_stress_data(opts)
  obj = opts[:object]
  generate_data "stress.#{obj}.dat", stress_data_patterns, opts
end

def get_columns(data)
  data.first.keys.map.with_index{|k,i| [k,i+1]}.to_h
end

def plot_stress_data(opts = {})
  obj = opts[:object]
  adds = opts[:adds]
  removes = opts[:removes]

  plot("stress.#{obj}.dat", "stress.#{obj}.pdf") do |data,graph|
    col = get_columns(data)
    data.sort_by! do |d|
      m = d[:mode][0]
      a = d[:adds]
      r = d[:removes]
      b = a+r+1
      m + (a+r).to_s(b) + (a).to_s(b) + (r).to_s(b)
    end
    data = data.chunk do |d|
      [d[:adds],d[:removes]]
    end.map do |_,ds|
      ds.first[:time] = ds.map{|d|d[:time]}.reduce(:+) / ds.count \
        if ds.all? {|d| d[:time].is_a?(Integer)}
      ds.first
    end
    ldata = data.select{|d| d[:mode] =~ /Lin/}
    cdata = data.select{|d| d[:mode] =~ /Counting/}
    ndata = data.select{|d| d[:mode] =~ /Unmonitored/}
    ndata.each do |d|
      d[:executions] /= 100000.0 if d[:executions].is_a?(Integer)
    end
    <<-xxx
    set terminal pdf
    set output '#{graph}'
    set title '#{object_name(obj)}'
    # set key invert
    set key box opaque top left
    set style data lines
    set style fill transparent solid 0.5 border rgb "black"
    set logscale y
    set style fill solid border rgb "black"
    set boxwidth 0.8
    set xlabel "No. Operations (1+1 -- 10+10)"
    set ylabel "Execution Time (s) -- timeout 5m"
    # set auto x
    # set auto y
    set yrange [0.01:*]
    set grid y
    set tic scale 0
    plot \
      #{wrap(ldata)} using #{col[:time]} title "Linearization" #{color(3)} w filledcurve x1, \
      #{wrap(cdata)} using #{col[:time]} title "Counting" #{color(2)} w filledcurve x1, \
      #{wrap(ndata)} using #{col[:time]} title "No Monitor" #{color(0)} w filledcurve x1, \
      #{wrap(ndata)} using #{col[:executions]} title "100K Executions" #{color(1)} w filledcurve x1
    xxx
  end
end

################################################################################
####                             N'IMPORTE QUOI                             ####
################################################################################


# [:bkq, :dq, :msq, :rdq, :ts, :ukq].each do |obj|
#   puts "Generating runtime data for #{obj}..."
#   generate_runtime_data(
#     object: obj, modes: ["none","counting-no-verify","count","lin"],
#     adds: 1..4, removes: 1..4, delays: 0..5, barriers: 2..2)
# end

# [:bkq, :dq, :msq, :rdq, :ts, :ukq].each do |obj|
#   puts "Generating coverage data for #{obj}..."
#   generate_coverage_data(
#     object: obj, mode: "versus",
#     adds: 1..4, removes: 1..4, delays: 0..5, barriers: 0..4)
# end

# [:msq].each do |obj|
#   puts "Generating stress data for #{obj}"
#   generate_stress_data(
#     object: obj, modes: ["none", "counting-no-verify", "counting", "lin-skip-atomic"],
#     adds: 1..10, removes: 1..10, delays: 3..3, barriers: 2..2)
# end

# generate_stress_data(
#   object: :msq, modes: ["none","count","lin-skip-atomic"],
#   adds: 1..10, removes: 1..10, delays: 3..3, barriers: 2..2,
#   repeat: 3)

plot_stress_data(object: :msq)


# (2..4).each do |a|
#   (2..4).each do |r|
#     plot_history_coverage(object: :bkq, adds: a, removes: r)
#   end
# end

