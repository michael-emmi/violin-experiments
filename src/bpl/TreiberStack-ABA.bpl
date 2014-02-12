type addr;
const unique null: addr;

type val;
type valRet = val;
const {: value} unique v0, v1: val;
const {: value} unique rEmpty,r0,r1: valRet;
function valid_val(v: val) returns (bool) { v == v0 || v == v1 }
// axiom (forall v: val :: valid_val(v) || v == empty);

var alloced: [addr] bool;
var data: [addr] val;
var next: [addr] addr;
var top: addr;


function argToRet(v: val) returns (valRet);
axiom (argToRet(v0) == r0);
axiom (argToRet(v1) == r1);

procedure alloc() returns (a: addr)
modifies alloced;
{
    assume !alloced[a];
    assume a != null;
    alloced[a] := true;
    return;
}

procedure freee(a: addr)
modifies alloced;
{
    // assert alloced[a];
    alloced[a] := false;
    return;
}

procedure L.Init() {
    var p: addr;
    
    call p := alloc();
    top := p;
    data[top] := rEmpty;
    next[top] := null;
}

procedure {: method} Push (v: val) 
{
    var p, q: addr;
    
    while (true) {
        
        p := top;
        
        // call {:yield 1} yield();
        
        call q := alloc();
        data[q] := v;
        next[q] := p;
        
        // call {:yield 1} yield();
        
        // begin atomic
        if (top == p) {
            top := q;
            break;
        }
        // end atomic
    }
}

procedure {: method} Pop() returns (v: valRet) 
{
    var p, q: addr;
    
    while (true) {
        
        p := top;
        
        // call {:yield 1} yield();
        
        if (data[p] == rEmpty) {
            v := rEmpty;
            break;
            
        } else {
            
            // call {:yield 1} yield();
            
            q := next[p];
            
            // BUG: yield here
            call {:yield 1} yield();
            
            // begin atomic
            if (top == p) {
                v := argToRet(data[p]);
                top := q;
                call freee(p);
                break;
            }
            // end atomic
        }
    }
}
