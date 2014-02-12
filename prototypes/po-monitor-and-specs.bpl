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

procedure {:inline 1} op.init()
modifies N, C;
{
  assume (forall o: op :: !C[o]);
  N := 0;
}

procedure {:inline 1} op.begin() returns (o: op)
modifies N;
{
  o := N;
  assume (forall oo: op :: completed(oo,C) ==> bef(oo,o));
  assume (forall oo: op :: active(oo,N,C) ==> !(o <: oo) && !(oo <: o));
  N := N + 1;
}

procedure {:inline 1} op.end(o: op)
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

function no_thinair(N: int, C: [op] bool, added: [int] bool) returns (bool) {
  (forall o: op :: completed(o,C) && m(o) == remove ==> 
    added[v(o)] || v(o) == empty
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

function bag_spec(N: int, C: [op] bool, A: [val] bool, E: [op] bool) 
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

function stack_spec(N: int, C: [op] bool, A: [val] bool, E: [op] bool)
returns (bool) {
  bag_spec(N,C,A,E) && stack_order(N,C)
}

function queue_spec(N: int, C: [op] bool, A: [val] bool, E: [op] bool)
returns (bool) {
  bag_spec(N,C,A,E) && queue_order(N,C)
}

var added: [val] bool;
var low, high: int;
// invariant (low <= high)
var saw_empty: [op] bool;

procedure {:inline 1} init()
modifies N, C;
modifies added, low, high;
{
  call op.init();
  assume (forall v: val :: !added[v]);
  assume (forall o: op :: !saw_empty[o]);
  low := 0;
  high := 0;
}

procedure {:inline 1} add.begin(v: val) returns (o: op)
modifies N;
modifies added, high;
{
  call o := op.begin();
  assume m(o) == add;
  assume v(o) == v;
  added[v] := true;
  high := high + 1;
  return;
}

procedure {:inline 1} add.end(o: op)
modifies C;
modifies low;
{
  call op.end(o);
  low := low + 1;
}

procedure {:inline 1} remove.begin() returns (o: op)
modifies N;
modifies low, saw_empty;
{
  call o := op.begin();
  assume m(o) == remove;
  saw_empty[o] := (low <= 0);
  if (v(o) != empty) {
    low := low - 1;
    if (low == 0) {
      call see_empty();
    }
  }
  return;
}

procedure {:inline 1} remove.end(o: op, v: val)
modifies C;
modifies high, saw_empty;
{
  assume v(o) == v;
  call op.end(o);
  if (v(o) != empty) {
    high := high - 1;
  }
  return;
}

procedure see_empty();
modifies saw_empty;
ensures low == 0 ==> (forall o: op :: active(o,N,C) ==> saw_empty[o]);
ensures low == 0 ==> 
  (forall o: op :: !active(o,N,C) ==> saw_empty[o] == old(saw_empty[o]));

/******************************************************************************/
/*  DEMOS   (1)   assertion   validated,    and   (2)   assertion   violated  */
/******************************************************************************/

procedure demo();
modifies N, C;
modifies added, low, high, saw_empty;

implementation demo()
{
  var a, b, c, d: op;
  
  call init();
  
  call a := add.begin(1);
  call add.end(a);
  
  // b and c are concurrent
  call b := add.begin(2);
  call c := remove.begin();
  call add.end(b);
  call remove.end(c,1);

  call d := remove.begin();
  call remove.end(d,2);
  
  assert stack_spec(N,C,added,saw_empty); // validated
  assert queue_spec(N,C,added,saw_empty); // validated
}

implementation demo()
{
  var a, b, c, d: op;
  
  call init();
  
  call a := add.begin(1);
  call add.end(a);
  
  // b completes before c begins
  call b := add.begin(2);
  call add.end(b);

  call c := remove.begin();
  call remove.end(c,1);

  call d := remove.begin();
  call remove.end(d,2);
  
  assert stack_spec(N,C,added,saw_empty); // VIOLATED
}

implementation demo()
{
  var a, b, c, d: op;
  
  call init();
  
  call a := add.begin(1);
  call add.end(a);
  
  // b completes before c begins
  call b := add.begin(2);
  call add.end(b);

  call c := remove.begin();
  call remove.end(c,1);

  call d := remove.begin();
  call remove.end(d,2);
  
  assert queue_spec(N,C,added,saw_empty); // validated
}

implementation demo()
{
  var a, b, c, d, e: op;
  
  call init();
  
  call a := add.begin(1);  
  call add.end(a);
  
  call b := add.begin(2);
  call add.end(b);
  
  call c := remove.begin();
  call remove.end(c,2);
  
  call d := remove.begin();  
  call remove.end(d,1);

  // definitely empty.
  call e := remove.begin();
  call remove.end(e,empty);
  
  assert stack_spec(N,C,added,saw_empty); // validated
}

implementation demo()
{
  var a, b, c, d, e, f: op;
  
  call init();
  
  call a := add.begin(1);
  call add.end(a);

  call b := add.begin(2);
  call add.end(b);  
  
  // it may not be empty now ...
  call e := remove.begin();
  
  call c := remove.begin();
  call remove.end(c,2);  

  call d := remove.begin();
  call remove.end(d,1);
  
  // ... but it is now!

  call f := add.begin(3);
  call add.end(f);

  call remove.end(e,empty);
  
  assert stack_spec(N,C,added,saw_empty); // validated
}

implementation demo()
{
  var a, b, c, d, e: op;
  
  call init();
  
  call a := add.begin(1);
  call add.end(a);
  
  call b := add.begin(2);
  call add.end(b);
  
  call c := remove.begin();
  call remove.end(c,2);

  // it was never empty during this operation.
  call e := remove.begin();
  call remove.end(e,empty);
  
  call d := remove.begin();
  call remove.end(d,1);
  
  assert stack_spec(N,C,added,saw_empty); // VIOLATED
}
