#define D(d) __SMACK_top_decl(d)

#define VIOLIN_INIT \
  __SMACK_code("call violin.init();")
  
#define VIOLIN_CHECK(s) \
  __SMACK_code("assert " #s "_spec(violin.add.open, violin.add.done, \
    violin.remove.open, violin.remove.done);")
  
#define VIOLIN_PROC \
  __SMACK_mod("violin.time"); \
  __SMACK_mod("violin.ret"); \
  __SMACK_mod("violin.add.open"); \
  __SMACK_mod("violin.add.done"); \
  __SMACK_mod("violin.remove.open"); \
  __SMACK_mod("violin.remove.done")

#define VIOLIN_OP \
  __SMACK_decl("var $t0: int;")
    
#define VIOLIN_OP_START(op,val) \
  __SMACK_code("call $t0 := violin." #op ".start(@);", val)

#define VIOLIN_OP_FINISH(op,val) \
  __SMACK_code("call violin." #op ".finish($t0,@);", val)
    
void violin_decls() {

  D("type val = int;");

  D("const violin.TIME_BOUND: int;");
  D("axiom violin.TIME_BOUND == 0;");
  D("var violin.time: int;");
  D("var violin.ret: bool;");

  D("var violin.add.open: [val][int] int;");
  D("var violin.add.done: [val][int][int] int;");
  D("var violin.remove.open: [val][int] int;");
  D("var violin.remove.done: [val][int][int] int;");
  
  D("procedure violin.add.start(v: val) returns (t: int)"
    "modifies violin.add.open;"
    "{"
    "  t := violin.time;"
    "  violin.add.open[v][t] := violin.add.open[v][t] + 1;"
    "  return;"
    "}");
  
  D("procedure violin.add.finish(v: val, t0: int)"
    "modifies violin.time, violin.ret, violin.add.open, violin.add.done;"
    "{"
    "  var t: int;"
    "  t := violin.time;"
    "  violin.ret := true;"
    "  violin.add.open[v][t0] := violin.add.open[v][t0] - 1;"
    "  violin.add.done[v][t0][t] := violin.add.done[v][t0][t] + 1;"
    "  return;"
    "}");
  
  D("procedure violin.remove.start(v: val) returns (t: int)"
    "modifies violin.remove.open;"
    "{"
    "  t := violin.time;"
    "  violin.remove.open[0][t] := violin.remove.open[0][t] + 1;"
    "  return;"
    "}");
  
  D("procedure violin.remove.finish(v: val, t0: int)"
    "modifies violin.time, violin.ret, violin.remove.open, violin.remove.done;"
    "{"
    "  var t: int;"
    "  t := violin.time;"
    "  violin.ret := true;"
    "  violin.remove.open[0][t0] := violin.remove.open[0][t0] - 1;"
    "  violin.remove.done[v][t0][t] := violin.remove.done[v][t0][t] + 1;"
    "  return;"
    "}");

#if VIOLIN_COUNTING == 0
  D("function XXX_spec(A.o: [val][int] int, A.d: [val][int][int] int,"
    "             R.o: [val][int] int, R.d: [val][int][int] int)"
    "returns (bool) {"
    "  R.d[1][0][0] <= A.o[1][0] + A.d[1][0][0]"
    "  && R.d[0][0][0] <= A.o[0][0] + A.d[0][0][0]"
    "}");
#else
#error invalid value for VIOLIN_COUNTING / expected 0
#endif
  
  D("function stack_spec(A.o: [val][int] int, A.d: [val][int][int] int,"
    "             R.o: [val][int] int, R.d: [val][int][int] int)"
    "returns (bool) {"
    "  XXX_spec(A.o,A.d,R.o,R.d)"
    "}");

  D("function queue_spec(A.o: [val][int] int, A.d: [val][int][int] int,"
    "             R.o: [val][int] int, R.d: [val][int][int] int)"
    "returns (bool) {"
    "  XXX_spec(A.o,A.d,R.o,R.d)"
    "}");

  D("procedure violin.init()"
    "modifies violin.time, violin.ret;"
    "{"
    "  violin.time := 0;"
    "  violin.ret := false;"
    "  assume (forall t0: int, v: val :: violin.add.open[v][t0] == 0);"
    "  assume (forall t0: int, v: val :: violin.remove.open[v][t0] == 0);"
    "  assume (forall t0, tf: int, v: val :: violin.add.done[v][t0][tf] == 0);"
    "  assume (forall t0, tf: int, v: val :: violin.remove.done[v][t0][tf] == 0);"
    "}");

  D("procedure violin.tick()"
    "modifies violin.time, violin.ret;"
    "{"
    "  while (*) {"
    "    assume violin.time < violin.TIME_BOUND;"
    "    assume {:yield} true;"
    "    if (violin.ret) {"
    "      violin.time := violin.time + 1;"
    "      violin.ret := false;"
    "    }"
    "  }"
    "}");
}

#undef D
