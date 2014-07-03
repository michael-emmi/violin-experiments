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

def timeout_cmd(t)
  "gtimeout #{t}"
end

def run_scal(object, timeout: nil, mode: "counting", show: "none",
    adds: 1, removes: 1, delays: 0, barriers: 0)
  cmd = ""
  cmd << "#{timeout_cmd(timeout)} " if timeout
  cmd << "#{scal_exe} #{object}"
  cmd << " -mode #{mode}"
  cmd << " -show #{show}"
  cmd << " -adds #{adds}"
  cmd << " -removes #{removes}"
  cmd << " -delays #{delays}"
  cmd << " -barriers #{barriers}"
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

  File.open(File.join(data_dir,data_file), "a") do |file|
    file.puts titles(extract(nil,data_patterns))
    file.flush
    opts[:repeat].times do
      opts[:delays].each do |d|
        puts "* generating data for #{opts[:object]} w/ #{d} delays..."
        opts[:adds].each do |a|
          opts[:removes].each do |r|
            opts[:barriers].each do |b|
              opts[:modes].each do |m|
                output = run_scal(
                  opts[:object], mode: m,
                  adds: a, removes: r, delays: d, barriers: b
                )
                file.puts(row(extract(output,data_patterns)))
                file.flush
                yield if block_given?
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

def get_columns(data)
  data.first.keys.map.with_index{|k,i| [k,i+1]}.to_h
end

def wrap(data, separator = ' ')
  "\"< echo '#{data.select{|d| if block_given? then yield d else true end}.
  map(&:values).map{|d| d.join(separator)}.join("\\n")}'\""
end

def color(c)
  c = [ :beige, :acquamarine, :turquoise, :coral ][c] if c.is_a?(Integer)
  "lc rgb \"#{c}\""
end

def plot(data_file, graph_file)
  return unless block_given?
  puts "Plotting #{data_file} to #{graph_file}"
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

def plot_runtimes_per_execution(file, opts = {})
  graph_file = File.join graphs_dir, file
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
  data.each do |d|
    d[:time] = d[:time] / d[:executions] if d[:executions].is_a?(Integer)
  end
  # data = data.sort_by{|d| d[:delays]}
  # data.select!{|d| d[:adds] == opts[:adds] && d[:removes] == opts[:removes]}
  raw_data = data.select{|d| d[:mode] =~ /Unmonitored/}
  cnov_data = data.select{|d| d[:mode] =~ /NoVerify/}
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
  set xtics 4,9,36
  set xtics add ("2 delays" 4)
  set xtics add ("3 delays" 13)
  set xtics add ("4 delays" 22)
  set xtics add ("5 delays" 31)
  # set yrange [0.01:*]
  set grid y
  set tic scale 0
  plot \
    #{wrap(lin_data)} using 6 title "Linearization" #{color(3)} w filledcurve x1, \
    #{wrap(cnt_data)} using 6 title "Counting (w/ verify)" #{color(2)} w filledcurve x1, \
    #{wrap(cnov_data)} using 6 title "Counting (w/o verify)" #{color(1)} w filledcurve x1, \
    #{wrap(raw_data)} using 6 title "No monitoring" #{color(0)} w filledcurve x1
  xxx
end

# [:bkq, :dq, :msq, :rdq, :ts, :ukq].each do |obj|
#   puts "Generating runtime data for #{obj}..."
#   generate_runtime_data(
#     object: obj, modes: ["none","counting-no-verify","count","lin"],
#     adds: 1..4, removes: 1..4, delays: 0..5, barriers: 2..2)
# end

# [:bkq, :dq, :msq, :rdq, :ts, :ukq].each do |obj|
#   # puts "Generating runtime data for #{obj}..."
#   # generate_runtime_data(
#   #   object: obj, modes: ["none","counting-no-verify","count","lin"],
#   #   adds: 1..4, removes: 1..4, delays: 0..5, barriers: 2..2)
#   plot_runtimes("runtime.#{obj}.pdf", object: obj)
#   plot_runtimes_per_execution("runtime-per-exec.#{obj}.pdf", object: obj)
# end

################################################################################
####    COVERAGE     TESTS:    K    BARRIERS     VS.     LINEARIZATION      ####
################################################################################

def coverage_data_patterns_m(name) {
  violations: /#{name} saw (\d+) violations/,
  bad_histories: /#{name} saw \d+ violations in (\d+)\/\d+ histories/,
  all_histories: /#{name} saw \d+ violations in \d+\/(\d+) histories/,
  covered: /#{name} saw \d+ violations in \d+\/\d+ histories; covered (\d+)\./
} end

def coverage_data_patterns_oc(n)
  coverage_data_patterns_m("Operation-Counting\\(#{n}\\)").
  to_a.map{|k,v| ["c#{n}_#{k}".to_sym, v]}.to_h
end

def coverage_data_patterns(n)
  (0..n).reduce(default_data_patterns.merge(coverage_data_patterns_m("Line-Up"))) do |ps,i|
    ps.merge(coverage_data_patterns_oc(i))
  end
end

def generate_coverage_data(opts, &block)
  obj = opts[:object]
  generate_data "coverage.#{obj}.dat", coverage_data_patterns(opts[:barriers].last), opts, &block
  end

def plot_history_coverage_boring(opts = {})
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

def plot_history_coverage(opts = {})
  obj = opts[:object]
  plot("coverage.#{obj}.dat", "coverage.#{obj}.pdf") do |data,graph|
    data.select! {|d| d[:bad_histories] > 0}
    data.select! {|d| d[:executions] > 1000}
    # data.select! {|d| d[:adds] + d[:removes] > 2}
    return unless data.size > 0
    data.each {|d| d[:eo] = "\'#{d[:executions]} / #{d[:adds] + d[:removes]}\'"}
    data.sort_by! {|d| d[:executions].to_f / (d[:adds] + d[:removes])}
    col = get_columns(data)
    <<-xxx
    set terminal pdf size 4, 2.5
    set output '#{graph}'
    set datafile missing "-"
    set key top left
    # set style data lines
    set datafile separator ","
    set style data filledcurves y1=0
    set style fill solid border rgb "black"
    # set style fill solid border rgb "black"
    # set style data histogram
    # set style histogram rows
    set logscale y
    set style line 1 lt rgb "#FFFFFF" lw 2
    set style line 2 lt rgb "#000000" lw 2
    set style line 3 lt rgb "#555555" lw 2
    set style line 4 lt rgb "#999999" lw 2
    set style line 5 lt rgb "#CCCCCC" lw 2
    set style line 6 lt rgb "#DDDDDD" lw 2
    set style line 7 lt rgb "#EEEEEE" lw 2
    set tic scale 0
    unset xtics
    # set xtics nomirror rotate by 45 right font ",4"
    plot \
      #{wrap(data,',')} using #{col[:all_histories]}:xtic(#{col[:eo]}) title "Histoires" ls 1, \
      #{wrap(data,',')} using #{col[:bad_histories]} title "Violations" ls 2, \
      #{wrap(data,',')} using #{col[:c4_covered]} title "Covered w/ k=4" ls 3, \
      #{wrap(data,',')} using #{col[:c3_covered]} title "Covered w/ k=3" ls 4, \
      #{wrap(data,',')} using #{col[:c2_covered]} title "Covered w/ k=2" ls 5, \
      #{wrap(data,',')} using #{col[:c1_covered]} title "Covered w/ k=1" ls 6, \
      #{wrap(data,',')} using #{col[:c0_covered]} title "Covered w/ k=0" ls 7
    xxx
  end
end

# [:bkq, :dq, :msq, :rdq, :ts, :ukq].each do |obj|
#   puts "Generating coverage data for #{obj}..."
#   generate_coverage_data(
#     object: obj, mode: "versus",
#     adds: 1..4, removes: 1..4, delays: 0..5, barriers: 0..4)
# end

# [:bkq, :dq, :msq, :rdq, :ts, :ukq].each do |obj|
# [:bkq].each do |obj|
#   puts "Generating coverage data for #{obj}..."
#   generate_coverage_data(
#     object: obj, modes: ["versus"],
#     # adds: 1..4, removes: 1..4, delays: 0..5, barriers: 4..4) do
#     adds: 4..4, removes: 4..4, delays: 5..5, barriers: 4..4) do
#     plot_history_coverage(object: obj)
#   end
# end

# (2..4).each do |a|
#   (2..4).each do |r|
#     plot_history_coverage(object: :bkq, adds: a, removes: r)
#   end
# end

# [:bkq,:dq,:msq,:rdq].each do |obj|
# [:bkq].each do |obj|
#   plot_history_coverage(object: obj)
# end

plot_history_coverage(object: :bkq)


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

def plot_stress_data(opts = {})
  obj = opts[:object]
  adds = opts[:adds]
  removes = opts[:removes]

  plot("stress.#{obj}.dat", "stress.#{obj}.pdf") do |data,graph|
    data.sort_by! do |d|
      m = d[:mode][0]
      a = d[:adds]
      r = d[:removes]
      b = a+r+1
      m + (a+r).to_s(b) + (a).to_s(b) + (r).to_s(b)
    end
    # data.each do |d|
    #   d[:time] /= d[:executions].to_f if d[:time].is_a?(Numeric)
    # end
    ldata = data.select{|d| d[:mode] =~ /Lin/}
    cdata = data.select{|d| d[:mode] =~ /Counting/}
    ndata = data.select{|d| d[:mode] =~ /Unmonitored/}
    ldata.each.with_index do |d,i|
      d[:time] /= ndata[i][:time].to_f
      d[:xt] = "#{d[:adds]}+#{d[:removes]}"
    end
    cdata.each.with_index do |d,i|
      d[:time] /= ndata[i][:time].to_f
    end
    col = get_columns(ldata)
    <<-xxx
    set terminal pdf size 3, 1.8
    set output '#{graph}'
    # set title '100 #{object_name(obj)}'
    # set xlabel "No. Operations (1+1 -- 10+10)"
    # set ylabel "Execution Time \\n (normalized over time without monitor)"
    set key top left
    # set style data lines
    set style data filledcurves y1=0
    set style fill solid border rgb "black"
    set style fill solid 0.5 border rgb "black"
    set logscale y
    set style line 1 lt rgb "#CCCCCC" lw 2
    set style line 2 lt rgb "#555555" lw 2
    set yrange [1:1000]
    set grid y
    set tic scale 0
    unset xtics
    # set xtics nomirror rotate by 45 right font ",4"
    plot \
      #{wrap(ldata)} using #{col[:time]}:xtic(#{col[:xt]}) title "Linearization" ls 1, \
      #{wrap(cdata)} using #{col[:time]} title "Operation Counting" ls 2
    xxx
  end
end

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

# plot_stress_data(object: :msq)

################################################################################
####       BARRIER    TESTS:     COST    OF    INCREASiNG     BARRIERS      ####
################################################################################

def generate_barrier_data(opts,&block)
  obj = opts[:object]
  generate_data "barrier.#{obj}.dat", default_data_patterns, opts, &block
end

def plot_barrier_data(opts = {})
  obj = opts[:object]
  adds = opts[:adds]
  removes = opts[:removes]

  plot("barrier.#{obj}.dat", "barrier.#{obj}.pdf") do |data,graph|
    col = get_columns(data)
    <<-xxx
    set terminal pdf
    set output '#{graph}'
    set title '#{object_name(obj)}'
    set xlabel "XXX"
    set ylabel "Execution Time"
    set key top left
    set style data lines
    # set style fill solid 0.5 border rgb "black"
    set tic scale 0
    plot \
      #{wrap(data)} using #{col[:time]} title "XXX"
    xxx
  end
end

# generate_barrier_data(
#   object: :msq, modes: ['counting'],
#   adds: 5..5, removes: 5..5, delays: 4..4, barriers: 0..20) do
#
#   plot_barrier_data(object: :msq)
# end

