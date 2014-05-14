#define D(d) __SMACK_top_decl(d)

#define VIOLIN_INIT \
  __SMACK_code("call violin.init();")
  
#define VIOLIN_CHECK(s) \
  __SMACK_code("assert " #s "_spec(#add.open, #add.done, #rem.open, #rem.done);")  

#define VALUE(v) \
  __SMACK_top_decl("axiom V(" #v ");")

#define VALUES(n) 
    
#if VIOLIN_COUNTING == 0
  
#define VIOLIN_PROC \
  __SMACK_mod("#add.open"); \
  __SMACK_mod("#add.done"); \
  __SMACK_mod("#rem.open"); \
  __SMACK_mod("#rem.done")

#define VIOLIN_OP 
    
#define VIOLIN_OP_START(op,val) \
  __SMACK_code("call violin." #op ".start(@);", val)

#define VIOLIN_OP_FINISH(op,val) \
  __SMACK_code("call violin." #op ".finish(@);", val)

#else // VIOLIN_COUNTING > 0
    
#define VIOLIN_PROC \
  __SMACK_mod("violin.time"); \
  __SMACK_mod("violin.ret"); \
  __SMACK_mod("#add.open"); \
  __SMACK_mod("#add.done"); \
  __SMACK_mod("#rem.open"); \
  __SMACK_mod("#rem.done")

#define VIOLIN_OP \
  __SMACK_decl("var $t0: int;")
    
#define VIOLIN_OP_START(op,val) \
  __SMACK_code("call $t0 := violin." #op ".start(@);", val)

#define VIOLIN_OP_FINISH(op,val) \
  __SMACK_code("call violin." #op ".finish($t0,@);", val)

#endif
    
void violin_decls() {

  D("type val = int;");
  D("const unique empty: val;");
  D("axiom (empty == -1);");

#if VIOLIN_COUNTING == 0
  
  D("var #add.open: [val] int;");
  D("var #add.done: [val] int;");
  D("var #rem.open: [val] int;");
  D("var #rem.done: [val] int;");
  D("function V(val) returns (bool);");

  D("procedure violin.init()"
    "{"
    "  assume !V(empty);"
    "  assume (forall v: val :: {V(v)} #add.open[v] == 0);"
    "  assume (forall v: val :: {V(v)} #add.done[v] == 0);"
    "  assume (forall v: val :: {V(v)} #rem.open[v] == 0);"
    "  assume (forall v: val :: {V(v)} #rem.done[v] == 0);"
    "}");
  
  D("procedure violin.add.start(v: val)"
    "modifies #add.open;"
    "{"
    "  assume V(v);"
    "  #add.open[v] := #add.open[v] + 1;"
    "}");
  
  D("procedure violin.add.finish(v: val)"
    "modifies #add.open, #add.done;"
    "{"
    "  #add.open[v] := #add.open[v] - 1;"
    "  #add.done[v] := #add.done[v] + 1;"
    "}");
  
  D("procedure violin.remove.start(v: val)"
    "modifies #rem.open;"
    "{"
    "  #rem.open[0] := #rem.open[0] + 1;"
    "}");
  
  D("procedure violin.remove.finish(v: val)"
    "modifies #rem.open, #rem.done;"
    "{"
    "  assume v == empty || V(v);"
    "  #rem.open[0] := #rem.open[0] - 1;"
    "  #rem.done[v] := #rem.done[v] + 1;"
    "}");
  
  D("function XXX_spec(A.o: [val] int, A.d: [val] int, R.o: [val] int, R.d: [val] int)"
    "returns (bool) {"
    "  (forall v: val :: {V(v)} V(v) ==> R.d[v] <= A.o[v] + A.d[v])"
    "}");
  
  D("function stack_spec(A.o: [val] int, A.d: [val] int, R.o: [val] int, R.d: [val] int)"
    "returns (bool) {"
    "  XXX_spec(A.o,A.d,R.o,R.d)"
    "}");
  
  D("function queue_spec(A.o: [val] int, A.d: [val] int, R.o: [val] int, R.d: [val] int)"
    "returns (bool) {"
    "  XXX_spec(A.o,A.d,R.o,R.d)"
    "}");
  
#else // VIOLIN_COUNTING > 0
  
  D("var violin.time: int;");
  D("var violin.ret: bool;");

  D("var #add.open: [val][int] int;");
  D("var #add.done: [val][int][int] int;");
  D("var #rem.open: [val][int] int;");
  D("var #rem.done: [val][int][int] int;");

  D("function V(val) returns (bool);");
  D("function T(int) returns (bool);");
  
  D("procedure violin.add.start(v: val) returns (t: int)"
    "modifies #add.open;"
    "{"
    "  assume V(v);"
    "  assume T(t);"
    "  if (*) { if (violin.ret && violin.time < " #VIOLIN_COUNTING ") {"
    "    violin.time := violin.time + 1;"
    "    violin.ret := false;"
    "  } }"
    "  t := violin.time;"
    "  #add.open[v][t] := #add.open[v][t] + 1;"
    "}");
  
  D("procedure violin.add.finish(v: val, t0: int)"
    "modifies violin.time, violin.ret, #add.open, #add.done;"
    "{"
    "  var t: int;"
    "  t := violin.time;"
    "  assume T(t);"
    "  violin.ret := true;"
    "  #add.open[v][t0] := #add.open[v][t0] - 1;"
    "  #add.done[v][t0][t] := #add.done[v][t0][t] + 1;"
    "}");
  
  D("procedure violin.remove.start(v: val) returns (t: int)"
    "modifies #rem.open;"
    "{"
    "  assume T(t);"
    "  if (*) { if (violin.ret && violin.time < " #VIOLIN_COUNTING ") {"
    "    violin.time := violin.time + 1;"
    "    violin.ret := false;"
    "  } }"
    "  t := violin.time;"
    "  #rem.open[0][t] := #rem.open[0][t] + 1;"
    "}");
  
  D("procedure violin.remove.finish(v: val, t0: int)"
    "modifies violin.time, violin.ret, #rem.open, #rem.done;"
    "{"
    "  var t: int;"
    "  t := violin.time;"
    "  assume v == empty || V(v);"
    "  assume T(t);"
    "  violin.ret := true;"
    "  #rem.open[0][t0] := #rem.open[0][t0] - 1;"
    "  #rem.done[v][t0][t] := #rem.done[v][t0][t] + 1;"
    "}");
  
#if VIOLIN_COUNTING > 1
#error invalid value for VIOLIN_COUNTING / expected < 2
#endif
  
  D("function XXX_spec(A.o: [val][int] int, A.d: [val][int][int] int, "
    "  R.o: [val][int] int, R.d: [val][int][int] int)"
    "returns (bool) {"
    "  (forall v: val :: {V(v)} V(v) ==>"
    "    R.d[v][0][0] + R.d[v][0][1] + R.d[v][1][1]"
    "    <= A.o[v][0] + A.o[v][1] + A.d[v][0][0] + A.d[v][0][1] + A.d[v][1][1])"
    "}");
  
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
    "  assume !V(empty);"
    "  violin.time := 0;"
    "  violin.ret := false;"
    "  assume (forall v: val, t0: int :: {V(v),T(t0)} #add.open[v][t0] == 0);"
    "  assume (forall v: val, t0: int :: {V(v),T(t0)} #rem.open[v][t0] == 0);"
    "  assume (forall v: val, t0, tf: int :: {V(v),T(t0),T(tf)} #add.done[v][t0][tf] == 0);"
    "  assume (forall v: val, t0, tf: int :: {V(v),T(t0),T(tf)} #rem.done[v][t0][tf] == 0);"
    "}");
  
#endif
  
}

#undef D
