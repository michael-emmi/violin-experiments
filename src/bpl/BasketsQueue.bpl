/* 
 * The Baskets Queue
 *   adapted from 
 *   M. Hoffman, Ori Shalev, Nir Shavit
 *   OPODIS 2007, LNCS 4878, pp. 401-414
 */
type node;
type val;
type valRet = val;
type ptr;

const unique N.null: node;
const {:value } unique v0, v1: val;
const {:value } unique r0, r1, rEmpty: valRet;
function valid_val(v: val) returns (bool) { v == v0 || v == v1 }

function argToRet(v: val) returns (valRet);
axiom (argToRet(v0) == r0);
axiom (argToRet(v1) == r1);

const MAX_HOPS: int;

/*
 * struct pointer_t {
 *   <ptr, deleted, tag>: <node_t *, boolean, unsigned integer>
 * }
 */
var P.ptr: [ptr] node;
var P.deleted: [ptr] bool;
var P.tag: [ptr] int;

/* 
 * struct node_t { 
 *   data_type value;
 *   pointer_t next;
 * }
 */
var N.data: [node] val;
var N.next: [node] ptr;

/*
 * struct queue_t {
 *   pointer_t tail;
 *   pointer_t head;
 * }
 */ 
var Q.head: ptr;
var Q.tail: ptr;


var N.alloced: [node] bool;

procedure N.alloc() returns (n: node) modifies N.alloced;
{
    assume !N.alloced[n];
    assume n != N.null;
    N.alloced[n] := true;
    return;
}


procedure N.freee(n: node) modifies N.alloced;
{
    // assert alloced[n];
    N.alloced[n] := false;
    return;
}

procedure L.Init() modifies P.ptr, P.deleted, P.tag, N.alloced;
{
    var h: node;
    
    assume (Q.head != Q.tail);
    
    call h := N.alloc();
    P.ptr[ N.next[h] ] := N.null;
    P.deleted[ N.next[h] ] := false;
    P.tag[ N.next[h] ] := 0;
    
    P.ptr[ Q.head ] := h;
    P.deleted[ Q.head ] := false;
    P.tag[ Q.head ] := 0;
    
    P.ptr[ Q.tail ] := h;
    P.deleted[ Q.tail ] := false;
    P.tag[ Q.tail ] := 0;
    
    return;
}

// procedure CAS( p1: ptr, p2: ptr, p3: ptr )
// returns (r: bool)
// {
//     if ( p1 == p2 ) {
//         r := true;
//         P.ptr[ p1 ] := P.ptr[ p3 ];
//         P.deleted[ p1 ] := P.deleted[ p3 ];
//         P.tag[ p1 ] := P.tag[ p3 ];
//         
//     } else {
//         r := false;
//     }
//     return;
// }

procedure CAS(p1: ptr, p2: ptr, n: node, b: bool, i: int) returns (r: bool)
modifies P.ptr, P.deleted, P.tag; {
    if ( p1 == p2 ) {
        // the swap
        P.ptr[ p1 ] := n;
        P.deleted[ p1 ] := b;
        P.tag[ p1 ] := i;
        r := true;
        return;
    } else {
        r := false;
        return;
    }
}
    

procedure backoff_scheme();
procedure free_chain(headi: ptr, new_head: ptr) 
modifies P.ptr, P.deleted, P.tag, N.alloced;
{
    var r: bool;
    var next: ptr;
    var head: ptr;
    head := headi;
    call r := CAS(Q.head,head,P.ptr[new_head],false,P.tag[head]+1);
    if (r) {
        while (P.ptr[head] != P.ptr[new_head]) {
            next := N.next[P.ptr[head]];
            call N.freee(P.ptr[head]);
            head := next;
        }
    }
}

procedure {: method} Enqueue( v: val )
modifies N.alloced, N.next, N.data, P.ptr, P.deleted, P.tag;
{
    var nd: node;
    var tail: ptr;
    var next: ptr;
    
    var b: bool;
    var p: ptr;
    
    var r: bool;
    
    call nd := N.alloc();
//     N.next[nd] := N.null;
    N.data[nd] := v;
    
    while (true) {
        tail := Q.tail;
        next := N.next[ P.ptr[ tail ] ];
        if ( tail == Q.tail ) {
            if ( P.ptr[ next ] == N.null ) {
                P.ptr[ N.next[nd] ] := N.null;
                P.deleted[ N.next[nd] ] := false;
                P.tag[ N.next[nd] ] := P.tag[ tail ] + 2;
                
                call r := CAS(N.next[P.ptr[tail]],next,nd,false,P.tag[tail]+1);
                if ( r ) {
                    call r := CAS(Q.tail,tail,nd,false,P.tag[tail]+1);
                    
//                     ret := true;
                    return;
                }
                
                next := N.next[ P.ptr[tail] ];
                
                while ( P.tag[next] == P.tag[tail] + 1 && !P.deleted[next] ) {
                    call backoff_scheme();
                    N.next[ nd ] := next;
                    call r := CAS(N.next[P.ptr[tail]],next,nd,false,P.tag[tail]+1);
                    if (r) {
//                         ret := true;
                        return;
                    }
                    next := N.next[ P.ptr[tail] ];
                }                
            } else {
                while ( P.ptr[ N.next[P.ptr[next]] ] != N.null && Q.tail == tail ) {
                    next := N.next[ P.ptr[next] ];
                }
                call r := CAS(Q.tail,tail,P.ptr[next],false,P.tag[tail]+1);
            }
        }
    }
    
    return;
}

procedure {: method} Dequeue() returns (v: valRet)
modifies P.ptr, P.deleted, P.tag, N.alloced;
{
    var head: ptr;
    var tail: ptr;
    var next: ptr;
    var iter: ptr;
    
    var r: bool;
    
    var hops: int;
    
    while (true) {
        beginloop:
        head := Q.head;
        tail := Q.tail;
        next := N.next[P.ptr[head]];
        if (head == Q.head) {
            if (P.ptr[head] == P.ptr[tail]) {
                if (P.ptr[next] == N.null) {
                    v := rEmpty;
                    return;
                }
                while ((P.ptr[N.next[P.ptr[next]]] != N.null) && (Q.tail == tail)) {
                    next := N.next[P.ptr[next]];
                }
                call r := CAS(Q.tail,tail,P.ptr[next],false,P.tag[tail]+1);
            } else {
                iter := head;
                hops := 0;
                while (P.deleted[next] && P.ptr[iter] != P.ptr[tail] && Q.head == head) {
                    iter := next;
                    next := N.next[P.ptr[iter]];
                    hops := hops + 1;
                }
                if (Q.head != head) {
                    goto beginloop;
                } else if (P.ptr[iter] == P.ptr[tail]) {
                    call free_chain(head,iter);
                } else {
                    v := argToRet(N.data[P.ptr[next]]);
                    call r := CAS(N.next[P.ptr[iter]],next,P.ptr[next],true,P.tag[next]+1);
                    if (r) {
                        if (hops >= MAX_HOPS) {
                            call free_chain(head,next);
                        }
                        return;
                    }
                    call backoff_scheme();
                }
            }
        }
    }
    return;
}