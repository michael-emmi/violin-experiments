
type node;
type val;
type lock;

const unique null: node;
const unique empty: val;
const unique v0, v1: val;
function valid_val(v: val) returns (bool) { v == v0 || v == v1 }
axiom (forall v: val :: valid_val(v) || v == empty);

var next: [node] node;
var data: [node] val;
var alloced: [node] bool;
var locked: [lock] bool;
const unique hlock, tlock: lock;
var head, tail: node;

procedure alloc() returns (n: node)
{
    assume !alloced[n];
    assume n != null;
    alloced[n] := true;
    return;
}


procedure freee(n: node)
{
    // assert alloced[n];
    alloced[n] := false;
    return;
}

procedure L.Init()
{
    var h: node;
    
    call h := alloc();
    next[h] := null;
    head := h;
    tail := h;
    assume (forall l: lock :: locked[l] == false);
    return;
}

procedure lock( l: lock )
{
    assume !locked[l];
    locked[l] := true;
    return;
}

procedure unlock( l: lock )
{
    locked[l] := false;
    return;
}  

procedure {: method} Enqueue( v: val)
{
    var n: node;
    
    // 	call {:yield} yield();
    
    call n := alloc();
    next[n] := null;
    data[n] := v;
    
    call lock( tlock );
    next[tail] := n;
    tail := n;
    call unlock( tlock );
    
    return;
}

procedure {: method} Dequeue() returns ( v: val )
{
    var h, hh: node;
    
    // 	call {:yield} yield();
    // 	assume Violin.time >= t0;
    
    
    
    // 	The correct code
    call lock( hlock );
    h := head;
    hh := next[h];
    
    // The buggy code
    // 	h := head;
    call {:yield} yield();
    // 	call lock( hlock );
    // 	hh := next[h];
    
    if (hh == null) {
        call unlock( hlock );
        v := empty;
        return;
    }
    
    v := data[hh];
    head := hh;
    call unlock( hlock );
    
    call freee(h);
    
    return;
}