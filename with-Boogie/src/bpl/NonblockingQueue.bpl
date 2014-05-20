// See paper:
//   Michael and Scott. Simple, Fast, and Practical Non-Blocking
//   and Blocking Concurrent Queue Algorithms. PODC 1996.


type val;
type node;
type pointer;

const nullVal: val;
const dummyVal: val;
const nullNode: node;
const nullPointer: pointer;

var next: [node] pointer;
var value: [node] val;
var ptr: [pointer] node;
var count: [pointer] int;

var head: pointer;
var tail: pointer;

procedure corral_atomic_begin();
procedure corral_atomic_end();

procedure createNode(v: val) returns (n: node) modifies value; {
  assume (n != nullNode);
  call corral_atomic_begin();
  assume (value[n] == nullVal);
  value[n] := v;
  call corral_atomic_end();
  return;
}
  

procedure createPointer(n: node) returns (p: pointer) modifies ptr; {
  assume (p != nullPointer);
  call corral_atomic_begin();
  assume (ptr[p] == nullNode);
  ptr[p] := n;
  call corral_atomic_end();
  return;
}

procedure initialize() modifies head, tail, next, ptr, count, value; {
  var n: node;
  var p: pointer;
  
  call n := createNode(dummyVal);
  call p := createPointer(n);
  
  next[n] := nullPointer;
  ptr[p] := n;
  count[p] := 0;
  head := p;
  tail := p;
}

procedure enqueue(v: val) modifies tail, next, value, ptr, count; {
  var n: node;
  var tl: pointer;
  var nxt: pointer;
  var cas: bool;
  var p1: pointer;
  var p2: pointer;
  var p3: pointer;
  
  assume (v != dummyVal && v != nullVal);
  
  call n := createNode(v);
  next[n] := nullPointer;
  
  while (true) {
    tl := tail;
    nxt := next[ptr[tl]];
    if (tl == tail) {
      if (ptr[nxt] == nullNode) {
        cas := false;
        call corral_atomic_begin();
        if (next[ptr[tail]] == nxt) {
          call p1 := createPointer(n);
          count[p1] := count[nxt] + 1;
          next[ptr[tail]] := p1;
          cas := true;
        }
        call corral_atomic_end();
        if (cas) { break; }
      } else {
        call corral_atomic_begin();
        if (tail == tl) {
          call p2 := createPointer(ptr[nxt]);
          count[p2] := count[tl] + 1;
          tail := p2;
        }
        call corral_atomic_end();
      }
    }
  }
  
  call corral_atomic_begin();
  if (tl == tail) {
    call p3 := createPointer(n);
    count[p3] := count[tl] + 1;
    tail := p3;
  } 
  call corral_atomic_end();
}

procedure dequeue() returns (v: val) modifies ptr, count, tail, head, value, next; {
  var hd: pointer;
  var tl: pointer;
  var nxt: pointer;
  var cas: bool;
  var p1: pointer;
  var p2: pointer;
  while (true) {
    hd := head;
    tl := tail;
    nxt := next[ptr[hd]];
    if (hd == head) {
      if (ptr[hd] == ptr[tl]) {
        if (ptr[nxt] == nullNode) {
           v:= dummyVal; return;
        }
        call corral_atomic_begin();
        if (tl == tail) {
          call p1 := createPointer(ptr[nxt]);
          count[p1] := count[tl] + 1;
          tail := p1;
        }
        call corral_atomic_end();
      } else {
        v := value[ptr[nxt]];
        call corral_atomic_begin();
        if (hd == head) {
          call p2 := createPointer(ptr[nxt]);
          count[p2] := count[hd]+1;
          head := p2;
          cas := true;
        }
        call corral_atomic_end();
        if (cas) { break; }
      }
    }
  }
  next[ptr[head]] := nullPointer;
  value[ptr[head]] := nullVal;
  return;
}


procedure Main () {
  assume (dummyVal != nullVal);
  assume (forall n:node :: value[n] == nullVal);
  assume (forall p:pointer :: ptr[p] == nullNode);
}
    
    
    
    