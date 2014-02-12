/******************************************************************************/
/******************************************************************************/
/**   DEMO  OF  USING  BOOGIE  FOR  OBSERVATIONAL REFINEMENT  VERIFICATION   **/
/******************************************************************************/
/******************************************************************************/

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
  assume (forall oo: op :: active(oo,N,C) ==> !(o <: oo) && !(oo <: o));
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

function no_thinair(N: int, C: [op] bool, A: [int] bool) returns (bool) {
  (forall o: op :: completed(o,C) && m(o) == remove ==> 
    A[v(o)] || v(o) == empty
  )
}

function unique_removes(N: int, C: [op] bool) returns (bool) {
  (forall o1, o2: op :: completed(o1,C) && completed(o2,C) && o1 != o2 ==>  
    m(o1) != m(o2) || v(o1) != v(o2) || v(o1) == empty || v(o2) == empty
  )
}

function no_false_empty(N: int, C: [op] bool, E: [op] bool) returns (bool) {
  (forall o: op :: completed(o,C) && m(o) == remove && v(o) == empty ==> E[o])
}

function {:spec} bag_spec(N: int, C: [op] bool, A: [val] bool, E: [op] bool) 
returns (bool) {
  no_thinair(N,C,A) && unique_removes(N,C) && no_false_empty(N,C,E)
}

function stack_order(N: int, C: [op] bool) returns (bool) {
  (forall o1, o2, o1', o2': op ::
    started(o1,N) && started(o2,N) && completed(o1',C) && completed(o2',C)
    && uniq4(o1,o2,o1',o2') 
    && match(o1,o1') && match(o2,o2')
    && o1' <: o2' ==> bef?(o2,o1) || (bef?(o1,o2) && bef?(o1',o2))
  )
}

function queue_order(N: int, C: [op] bool) returns (bool) {
  (forall o1, o2, o1', o2': op :: 
    started(o1,N) && started(o2,N) && completed(o1',C) && completed(o2',C)
    && uniq4(o1,o2,o1',o2')
    && match(o1,o1') && match (o2,o2')
    && o1' <: o2' ==> bef?(o1,o2)
  )
}

function {:spec} stack_spec(N: int, C: [op] bool, A: [val] bool, E: [op] bool)
returns (bool) {
  bag_spec(N,C,A,E) && stack_order(N,C)
}

function {:spec} queue_spec(N: int, C: [op] bool, A: [val] bool, E: [op] bool)
returns (bool) {
  bag_spec(N,C,A,E) && queue_order(N,C)
}

var A: [val] bool;
var low, high: int;
// invariant (low <= high)
var E: [op] bool;

procedure {:init} init()
modifies N, C;
modifies A, low, high;
{
  call op.init();
  assume (forall v: val :: !A[v]);
  assume (forall o: op :: !E[o]);
  low := 0;
  high := 0;
}

procedure add.start(v: val) returns (o: op)
modifies N;
modifies A, high;
{
  call o := op.start();
  assume m(o) == add;
  assume v(o) == v;
  A[v] := true;
  high := high + 1;
  return;
}

procedure add.finish(o: op)
modifies C;
modifies low;
{
  call op.finish(o);
  low := low + 1;
}

procedure remove.start() returns (o: op)
modifies N;
modifies low, E;
{
  call o := op.start();
  assume m(o) == remove;
  E[o] := (low <= 0);
  if (v(o) != empty) {
    low := low - 1;
    if (low == 0) {
      call see_empty();
    }
  }
  return;
}

procedure remove.finish(o: op, v: val)
modifies C;
modifies high, E;
{
  assume v(o) == v;
  call op.finish(o);
  if (v(o) != empty) {
    high := high - 1;
  }
  return;
}

procedure see_empty();
modifies E;
ensures low == 0 ==> (forall o: op :: active(o,N,C) ==> E[o]);
ensures low == 0 ==> 
  (forall o: op :: !active(o,N,C) ==> E[o] == old(E[o]));
