/******************************************************************************/
/*         GENERIC           CODE          FOR           OPERATIONS           */
/******************************************************************************/

type op = int;
type method;
type val = int;

function m(op) returns (method);
function v(op) returns (val);

function started(o: op, N: int) returns (bool) { 0 <= o && o < N }
function completed(o: op, C: [op] bool) returns (bool) { C[o] }
function active(o: op, N: int, C: [op] bool) returns (bool) { 
  started(o,N) && !completed(o,C) 
}

function bef(o1, o2: op) returns (bool) { o1 <: o2 }
function bef?(o1, o2: op) returns (bool) { !(o2 <: o1) }

function uniq4(o1,o2,o3,o4: op) returns (bool) {
  o1 != o2 && o1 != o3 && o1 != o4 && o2 != o3 && o2 != o4 && o3 != o4
}

var N: op;
var C: [op] bool;
// invariant (forall o: op :: C[o] ==> 0 <= o && o < N);
// i.e. (forall o: op :: completed(o,C) ==> started(o,N))

procedure op.init()
modifies N, C;
{
  assume (forall o: op :: !C[o]);
  N := 0;
}

procedure op.start() returns (o: op)
modifies N;
{
  o := N;
  assume (forall oo: op :: completed(oo,C) ==> bef(oo,o));
  assume (forall oo: op :: active(oo,N,C) ==> !bef(o,oo) && !bef(oo,o));
  N := N + 1;
}

procedure op.finish(o: op)
modifies C;
{
  C[o] := true;
}

/******************************************************************************/
/*     CODE        FOR       ADD       &       REMOVE        OPERATIONS       */
/******************************************************************************/

const unique add: method;
const unique remove: method;
const unique empty: val;
axiom (empty == -1);

function match(o1: op, o2: op) returns (bool) { 
  m(o1) == add && m(o2) == remove && v(o1) == v(o2) 
}

function {:spec} no_thinair(N: int, C: [op] bool, A: [int] bool) returns (bool) {
  (forall o: op :: completed(o,C) && m(o) == remove ==> 
    A[v(o)] || v(o) == empty
  )
}

function {:spec} unique_removes(N: int, C: [op] bool) returns (bool) {
  (forall o1, o2: op :: completed(o1,C) && completed(o2,C) && o1 != o2 ==>  
    m(o1) != m(o2) || v(o1) != v(o2) || v(o1) == empty || v(o2) == empty
  )
}

function {:spec} no_false_empty(N: int, C: [op] bool, W: [op] bool) returns (bool) {
  (forall o: op :: completed(o,C) && m(o) == remove && v(o) == empty ==> W[o])
}

function {:spec} bag_spec(N: int, C: [op] bool, A: [val] bool, W: [op] bool) 
returns (bool) {
  no_thinair(N,C,A) && unique_removes(N,C) && no_false_empty(N,C,W)
}

function {:spec} stack_order(N: int, C: [op] bool) returns (bool) {
  (forall o1, o2, o1', o2': op ::
    started(o1,N) && started(o2,N) && completed(o1',C) && completed(o2',C)
    && uniq4(o1,o2,o1',o2') 
    && match(o1,o1') && match(o2,o2')
    && bef(o1',o2') ==> bef?(o2,o1) || (bef?(o1,o2) && bef?(o1',o2))
  )
}

function {:spec} queue_order(N: int, C: [op] bool) returns (bool) {
  (forall o1, o2, o1', o2': op :: 
    started(o1,N) && started(o2,N) && completed(o1',C) && completed(o2',C)
    && uniq4(o1,o2,o1',o2')
    && match(o1,o1') && match (o2,o2')
    && bef(o1',o2') ==> bef?(o1,o2)
  )
}

function {:spec} stack_spec(N: int, C: [op] bool, A: [val] bool, W: [op] bool)
returns (bool) {
  bag_spec(N,C,A,W) && stack_order(N,C)
}

function {:spec} queue_spec(N: int, C: [op] bool, A: [val] bool, W: [op] bool)
returns (bool) {
  bag_spec(N,C,A,W) && queue_order(N,C)
}

var A, R: [val] bool;
var W: [op] bool;

function sees_empty(A: [val] bool, R: [val] bool) returns (bool) {
  (forall v: val :: A[v] ==> R[v])
}

procedure see_empty();
modifies W;
ensures sees_empty(A,R) ==> (forall o: op :: active(o,N,C) ==> W[o]);
ensures sees_empty(A,R) ==> (forall o: op :: !active(o,N,C) ==> W[o] == old(W[o]));

procedure {:init} init()
modifies N, C;
{
  call op.init();
  assume (forall v: val :: !A[v]);
  assume (forall v: val :: !R[v]);
  assume (forall o: op :: !W[o]);
}

procedure add.start(v: val) returns (o: op)
modifies N;
{
  call boogie_si_record_int(v);
  call o := op.start();
  assume m(o) == add;
  assume v(o) == v;
  return;
}

procedure add.finish(o: op)
modifies C;
modifies A;
{
  call op.finish(o);
  A[v(o)] := true;
}

procedure remove.start() returns (o: op)
modifies N;
modifies R, W;
{
  call o := op.start();
  call boogie_si_record_int(o);
  assume m(o) == remove;
  if (v(o) == empty) {
    W[o] := sees_empty(A,R);
    call boogie_si_record_bool(W[o]);
    
  } else {
    R[v(o)] := true;
    if (sees_empty(A,R)) {
      call see_empty();
    }
  }
  return;
}

procedure remove.finish(o: op, v: val)
modifies C;
{
  call boogie_si_record_int(v);
  assume v(o) == v;
  call op.finish(o);
  return;
}