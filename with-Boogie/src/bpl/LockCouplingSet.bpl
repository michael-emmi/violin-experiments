type node;
type val = int;
const unique null: node;

const unique MIN_KEY: val;
const unique MAX_KEY: val;
type valArg = val;
type valRet = val;
const {:value} unique k0, k1: valArg;
const {:value} unique vtrue, vfalse: valRet;

function valid_val(k: val) returns (bool) { k == k0 || k == k1 }
axiom (forall v: val :: valid_val(v) || v == MIN_KEY || v == MAX_KEY || v == vtrue || v == vfalse);
axiom (MIN_KEY < MAX_KEY);
axiom (forall v: val :: valid_val(v) ==> (MIN_KEY < v && v < MAX_KEY));

var next: [node] node;
var key: [node] val;
var locked: [node] bool;
var alloced: [node] bool;

var head: node;

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

procedure lock( n: node )
{
    assume !locked[n];
    locked[n] := true;
    return;
}

procedure unlock( n: node )
{
    locked[n] := false;
    return;
}

procedure L.Init()
{	
    var h, t: node;
    
    call h := alloc();
    call t := alloc();
    
    key[h] := MIN_KEY;
    key[t] := MAX_KEY;
    next[h] := t;
    next[t] := null;
    
    head := h;
    
    assume (forall n: node :: !locked[n]);
    return;
}

procedure {: method} Add( kk: valArg) returns (b: valRet)
{
    var pred: node;
    var curr: node;
    var n: node;
    
    call {:yield} yield();
    assume Violin.time >= t0;
    
    call n := alloc();	
    
    call lock(head);
    pred := head;
    curr := next[pred];
    call lock(curr);
    
    while (key[curr] < kk) {
        call unlock(pred);
        pred := curr;
        curr := next[curr];
        call lock(curr);
    }
    
    if (key[curr] == kk) {
        call unlock(curr);
        call unlock(pred);
        b := vfalse;
        return;
        
    } else {
        key[n] := kk;
        next[n] := curr;
        next[pred] := n;
        
        // Correct placement of unlocks
        call unlock(curr);
        call unlock(pred);
        
        b := vtrue;
        return;
    }
    
    return;
}

procedure {: method} Remove( kk: valArg) returns (b: valRet)
{
    var pred: node;
    var curr: node;
    
    call lock(head);
    pred := head;
    curr := next[pred];
    call lock(curr);
    
    while (key[curr] < kk) {
        call unlock(pred);
        pred := curr;
        curr := next[curr];
        call lock(curr);
    }
    
    if (key[curr] == kk) {
        
        // BUG: incorrect placement of unlocks
        //         call unlock(curr);
        //         call unlock(pred);
        
        call {:yield} yield();
        
        next[pred] := next[curr];
        
        // Correct placement of unlocks
        call unlock(curr);
        call unlock(pred);
        
        b := vtrue;
        return;
        
    } else {
        call unlock(curr);
        call unlock(pred);
        b := vfalse;
        return;
    }
    
    
    return;
}

