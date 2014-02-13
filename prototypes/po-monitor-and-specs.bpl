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

procedure {:inline 1} op.start() returns (o: op)
modifies N;
{
  o := N;
  assume (forall oo: op :: completed(oo,C) ==> bef(oo,o));
  assume (forall oo: op :: active(oo,N,C) ==> !(o <: oo) && !(oo <: o));
  N := N + 1;
}

procedure {:inline 1} op.finish(o: op)
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

var A, R: [val] bool;
var W: [op] bool;

function sees_empty(A: [val] bool, R: [val] bool) returns (bool) {
  (forall v: val :: A[v] ==> R[v])
}

procedure see_empty();
modifies W;
ensures sees_empty(A,R) ==> (forall o: op :: active(o,N,C) ==> W[o]);
ensures sees_empty(A,R) ==> (forall o: op :: !active(o,N,C) ==> W[o] == old(W[o]));

procedure {:inline 1} init()
modifies N, C;
{
  call op.init();
  assume (forall v: val :: !A[v]);
  assume (forall v: val :: !R[v]);
  assume (forall o: op :: !W[o]);
}

procedure {:inline 1} add.start(v: val) returns (o: op)
modifies N;
{
  call o := op.start();
  assume m(o) == add;
  assume v(o) == v;
  return;
}

procedure {:inline 1} add.finish(o: op)
modifies C;
modifies A;
{
  call op.finish(o);
  A[v(o)] := true;
}

procedure {:inline 1} remove.start() returns (o: op)
modifies N;
modifies R, W;
{
  call o := op.start();
  assume m(o) == remove;
  if (v(o) == empty) {
    W[o] := sees_empty(A,R);
    
  } else {
    R[v(o)] := true;
    if (sees_empty(A,R)) {
      call see_empty();
    }
  }
  return;
}

procedure {:inline 1} remove.finish(o: op, v: val)
modifies C;
{
  assume v(o) == v;
  call op.finish(o);
  return;
}

/******************************************************************************/
/*           DEMOS:              4 VERIFIED,             3 ERRORS             */
/******************************************************************************/

procedure demo();
modifies N, C;
modifies A, R, W;

implementation demo()
{
  var a, b, c, d: op;
  
  call init();
  
  call a := add.start(1);
  call add.finish(a);
  
  // b and c are concurrent
  call b := add.start(2);
  call c := remove.start();
  call add.finish(b);
  call remove.finish(c,1);

  call d := remove.start();
  call remove.finish(d,2);
  
  assert stack_spec(N,C,A,W); // validated
  assert queue_spec(N,C,A,W); // validated
}

implementation demo()
{
  var a, b, c, d: op;
  
  call init();
  
  call a := add.start(1);
  call add.finish(a);
  
  // b completes before c starts
  call b := add.start(2);
  call add.finish(b);

  call c := remove.start();
  call remove.finish(c,1);

  call d := remove.start();
  call remove.finish(d,2);
  
  assert stack_spec(N,C,A,W); // VIOLATED
}

implementation demo()
{
  var a, b, c, d: op;
  
  call init();
  
  call a := add.start(1);
  call add.finish(a);
  
  // b completes before c starts
  call b := add.start(2);
  call add.finish(b);

  call c := remove.start();
  call remove.finish(c,1);

  call d := remove.start();
  call remove.finish(d,2);
  
  assert queue_spec(N,C,A,W); // validated
}

implementation demo()
{
  var a, b, c, d, e: op;
  
  call init();
  
  call a := add.start(1);  
  call add.finish(a);
  
  call b := add.start(2);
  call add.finish(b);
  
  call c := remove.start();
  call remove.finish(c,2);
  
  call d := remove.start();  
  call remove.finish(d,1);

  // definitely empty.
  call e := remove.start();
  call remove.finish(e,empty);
  
  assert stack_spec(N,C,A,W); // validated
}

implementation demo()
{
  var a, b, c, d, e, f: op;
  
  call init();
  
  call a := add.start(1);
  call add.finish(a);

  call b := add.start(2);
  call add.finish(b);  
  
  // it may not be empty now ...
  call e := remove.start();
  
  call c := remove.start();
  call remove.finish(c,2);  

  call d := remove.start();
  call remove.finish(d,1);
  
  // ... but it is now!

  call f := add.start(3);
  call add.finish(f);

  call remove.finish(e,empty);
  
  assert stack_spec(N,C,A,W); // validated
}

implementation demo()
{
  var a, b, c, d, e: op;
  
  call init();
  
  call a := add.start(1);
  call add.finish(a);
  
  call b := add.start(2);
  call add.finish(b);
  
  call c := remove.start();
  call remove.finish(c,2);

  // it was never empty during this operation.
  call e := remove.start();
  call remove.finish(e,empty);
  
  call d := remove.start();
  call remove.finish(d,1);
  
  assert stack_spec(N,C,A,W); // VIOLATED
}

implementation demo()
{
  var a, b, c, d, e: op;
  
  call init();
  
  // Constantin's counterexample, violates no_false_empty

  call a := add.start(1);      // -.
                               //  A: add(1)
                               //  |
  call b := add.start(2);      // ----.
                               //  |  |
  call add.finish(a);          // -'  B: add(2)
                               //     |
  call e := remove.start();    // -------.
                               //     |  E: rem => empty
                               //     |  |
  call c := remove.start();    // ----------.
                               //     |  |  |
  call add.finish(b);          // ----'  |  C: rem => 2
                               //        |  |
  call remove.finish(c,2);     // ----------'
                               //        |
  call remove.finish(e,empty); // -------'
                               //
  call d := remove.start();    // -.
                               //  D: rem => 1
  call remove.finish(d,1);     // -'

  assert no_false_empty(N,C,W); // VIOLATED
}
