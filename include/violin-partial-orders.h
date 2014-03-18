#define D(d) __SMACK_top_decl(d)

void violin_decls();

#define VIOLIN_INIT \
  __SMACK_code("call violin.init();")

#define VIOLIN_CHECK(s) \
  __SMACK_code("assert " #s "_spec(N,C,A,W);")
    
#define VIOLIN_PROC \
  __SMACK_mod("N"); \
  __SMACK_mod("C"); \
  __SMACK_mod("A"); \
  __SMACK_mod("R"); \
  __SMACK_mod("W")

#define VIOLIN_OP \
  __SMACK_decl("var $myop: int;")

#define VIOLIN_OP_START(op,val) \
  __SMACK_code("call $myop := " #op ".start(@);", val)

#define VIOLIN_OP_FINISH(op,val) \
  __SMACK_code("call " #op ".finish($myop,@);", val)


void violin_decls() {
  
  /****************************************************************************/
  /*        GENERIC           CODE          FOR           OPERATIONS          */
  /****************************************************************************/
  
  D("type op = int;");
  D("type method;");
  D("type val = int;");

  D("function m(op) returns (method);");
  D("function v(op) returns (val);");
  D("function T(op) returns (bool);");

  D("function started(o: op, N: int) returns (bool) { 0 <= o && o < N }");
  D("function completed(o: op, N: int, C: [op] bool) returns (bool) { started(o,N) && C[o] }");
  D("function active(o: op, N: int, C: [op] bool) returns (bool) {"
    " started(o,N) && !completed(o,N,C) "
    "}");    

  // WITH BOOGIE's POLYMORPHIC PARTIAL ORDER ...
  // function bef(o1, o2: op) returns (bool) { o1 <: o2 }
  // function bef?(o1, o2: op) returns (bool) { !(o2 <: o1) }

  // ALTERNATE VERSION WITH MONOMORPHIC PARTIAL ORDER ...
  D("function $po(i,j: int) returns (bool);");
  D("axiom (forall i: int :: $po(i,i));");
  D("axiom (forall i, j: int :: $po(i,j) && $po(j,i) ==> i == j);");
  D("axiom (forall i, j, k: int :: $po(i,j) && $po(j,k) ==> $po(i,k));");
  D("function bef(o1, o2: op) returns (bool) { $po(o1,o2) }");
  D("function bef?(o1, o2: op) returns (bool) { !$po(o2,o1) }");

  D("function uniq4(o1,o2,o3,o4: op) returns (bool) {"
    " o1 != o2 && o1 != o3 && o1 != o4 && o2 != o3 && o2 != o4 && o3 != o4 "
    "}");
  
  D("var N: op;");
  D("var C: [op] bool;");
  // invariant (forall o: op :: C[o] ==> 0 <= o && o < N);
  // i.e. (forall o: op :: completed(o,C) ==> started(o,N))
  
  D("procedure op.init()"
    "modifies N, C;"
    "{"
    "  assume (forall o: op :: {T(o)} !C[o]);"
    "  N := 0;"
    "}");

  D("procedure op.start() returns (o: op)"
    "modifies N;"
    "{"
    "  o := N;"
    "  assume T(o);"
    "  assume (forall oo: op :: {T(oo)} completed(oo,N,C) ==> bef(oo,o));"
    "  assume (forall oo: op :: {T(oo)} active(oo,N,C) ==> !bef(o,oo) && !bef(oo,o));"
    "  N := N + 1;"
    "}");

  D("procedure op.finish(o: op)"
    "modifies C;"
    "{"
    "  C[o] := true;"
    "  assume completed(o,N,C);" // for triggering purposes...
    "}");

  /****************************************************************************/
  /*    CODE        FOR       ADD       &       REMOVE        OPERATIONS      */
  /****************************************************************************/
  
  D("const unique add: method;");
  D("const unique remove: method;");
  D("const unique empty: val;");
  D("axiom (empty == -1);");

  D("function match(o1: op, o2: op) returns (bool) { "
    "  m(o1) == add && m(o2) == remove && v(o1) == v(o2) "
    "}");

  D("function no_thinair(N: int, C: [op] bool, A: [int] bool) returns (bool) {"
    "  (forall o: op :: {T(o)} completed(o,N,C) && m(o) == remove ==> "
    "    A[v(o)] || v(o) == empty"
    "  )"
    "}");

  D("function unique_removes(N: int, C: [op] bool) returns (bool) {"
    "  (forall o1, o2: op :: {T(o1), T(o2)} completed(o1,N,C) && completed(o2,N,C) && o1 != o2 ==>  "
    "    m(o1) != m(o2) || v(o1) != v(o2) || v(o1) == empty || v(o2) == empty"
    "  )"
    "}");

  D("function no_false_empty(N: int, C: [op] bool, W: [op] bool) returns (bool) {"
    "  (forall o: op :: {T(o)} completed(o,N,C) && m(o) == remove && v(o) == empty ==> W[o])"
    "}");

  D("function bag_spec(N: int, C: [op] bool, A: [val] bool, W: [op] bool) "
    "returns (bool) {"
    "  no_thinair(N,C,A) && unique_removes(N,C) && no_false_empty(N,C,W)"
    "}");

  D("function stack_order(N: int, C: [op] bool) returns (bool) {"
    "  (forall o1, o2, o1', o2': op :: {T(o1), T(o2), T(o1'), T(o2')} "
    "    started(o1,N) && started(o2,N) && completed(o1',N,C) && completed(o2',N,C)"
    "    && uniq4(o1,o2,o1',o2') "
    "    && match(o1,o1') && match(o2,o2')"
    "    && bef(o1',o2') ==> bef?(o2,o1) || (bef?(o1,o2) && bef?(o1',o2))"
    "  )"
    "}");

  D("function queue_order(N: int, C: [op] bool) returns (bool) {"
    "  (forall o1, o2, o1', o2': op :: {T(o1), T(o2), T(o1'), T(o2')} "
    "    started(o1,N) && started(o2,N) && completed(o1',N,C) && completed(o2',N,C)"
    "    && uniq4(o1,o2,o1',o2')"
    "    && match(o1,o1') && match (o2,o2')"
    "    && bef(o1',o2') ==> bef?(o1,o2)"
    "  )"
    "}");

  D("function stack_spec(N: int, C: [op] bool, A: [val] bool, W: [op] bool)"
    "returns (bool) {"
    "  bag_spec(N,C,A,W) && stack_order(N,C)"
    "}");

  D("function queue_spec(N: int, C: [op] bool, A: [val] bool, W: [op] bool)"
    "returns (bool) {"
    "  bag_spec(N,C,A,W) && queue_order(N,C)"
    "}");

  D("var A, R: [val] bool;");
  D("var W: [op] bool;");

  D("function sees_empty(A: [val] bool, R: [val] bool) returns (bool) {"
    "  (forall v: val :: A[v] ==> R[v])"
    "}");

  D("procedure see_empty();"
    "modifies W;"
    "ensures sees_empty(A,R) ==> (forall o: op :: {T(o)} active(o,N,C) ==> W[o]);"
    "ensures sees_empty(A,R) ==> (forall o: op :: {T(o)} !active(o,N,C) ==> W[o] == old(W[o]));");

  D("procedure violin.init()"
    "modifies N, C;"
    "{"
    "  call op.init();"
    "  assume (forall v: val :: !A[v]);"
    "  assume (forall v: val :: !R[v]);"
    "  assume (forall o: op :: {T(o)} !W[o]);"
    "}");

  D("procedure add.start(v: val) returns (o: op)"
    "modifies N;"
    "{"
    "  call o := op.start();"
    "  assume m(o) == add;"
    "  assume v(o) == v;"
    "  return;"
    "}");

  D("procedure add.finish(o: op, ignored: val)"
    "modifies C;"
    "modifies A;"
    "{"
    "  call op.finish(o);"
    "  A[v(o)] := true;"
    "}");

  D("procedure remove.start(ignored: val) returns (o: op)"
    "modifies N;"
    "modifies R, W;"
    "{"
    "  call o := op.start();"
    "  assume m(o) == remove;"
    "  if (v(o) == empty) {"
    "    W[o] := sees_empty(A,R);"
    "  } else {"
    "    R[v(o)] := true;"
    "    if (sees_empty(A,R)) {"
    "      call see_empty();"
    "    }"
    "  }"
    "  return;"
    "}");

  D("procedure remove.finish(o: op, v: val)"
    "modifies C;"
    "{"
    "  assume v(o) == v;"
    "  call op.finish(o);"
    "  return;"
    "}");
}

#undef D
