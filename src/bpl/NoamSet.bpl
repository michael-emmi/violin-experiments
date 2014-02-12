type node;

const min: int;
const max: int;
const nullNode: node;

var next: [node] node;
var key: [node] int;
var marked: [node] bool;
var used: [node] bool;

var tail: node;
var head: node;

procedure corral_atomic_begin();
procedure corral_atomic_end();

procedure use(n: node) modifies used; {
  assume (n != nullNode);
  call corral_atomic_begin();
  assume (!used[n]);
  used[n] := true;
  call corral_atomic_end();
}

procedure intialize() modifies next, key, head, tail, used, marked; {
  var n1: node;
  var n2: node;
  call use(n1);
  call use(n2);
  
  key[n1] := min;
  marked[n1] := false;
  next[n1] := n2;
  
  key[n2] := max;
  marked[n2] := false;
  next[n2] := nullNode;

  head := n1;
  tail := n2;
}

procedure locate(k: int) returns (p: node, c: node) {
  p := head;
  c := next[p];
  
  while (key[c] < k) {
    p := c;
    c := next[p];
  }
  return;
}

procedure remove(k: int) returns (retval: bool) modifies marked, next; {
  var restart: bool;
  var p: node;
  var c: node;
  
  assume (min < k && k < max);
  restart := true;
  while (restart) {
    call p,c := locate(k);
    
    call corral_atomic_begin();
    if (next[p] == c && !marked[p]) {
      restart := false;
      if (key[c] == k) {
        marked[c] := true;
        next[p] := next[c];
        retval := true;
      } else { retval := false; }
    }
    call corral_atomic_end();
  }
  return;
}

procedure add(k: int) returns (retval: bool) modifies marked, next, used, key; {
  var restart: bool;
  var p: node;
  var c: node;
  var t: node;
  
  assume (min < k && k < max);
  call use(t);
  restart := true;
  
  while (restart) {
    call p,c := locate(k);
    
    call corral_atomic_begin();
    if (next[p] == c && !marked[p]) {
      restart := false;
      if (key[c] != k) {
        marked[t] := false;
        key[t] := k;
        next[t] := c;
        next[p] := t;
        retval := true;
      } else { retval := false; }
    }
    call corral_atomic_end();
  }
  return;
}
  
    

procedure Main () {
  assume (forall n:node :: !used[n]);
}
