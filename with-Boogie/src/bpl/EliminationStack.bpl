type val;
type threadInfo;
type pid;
type cell;
type op;

procedure corral_atomic_begin();
procedure corral_atomic_end();

var location: [pid] threadInfo;
var collision: [int] pid;

var ti_id: [threadInfo] pid;
var ti_op: [threadInfo] op;
var ti_cell: [threadInfo] cell;

var c_next: [cell] cell;
var c_data: [cell] val;

var ptop: cell;

const nullTI: threadInfo;
const nullCell: cell;
const empty: pid;

const PUSH: op;
const POP: op;

procedure lesOP(p: threadInfo, mypid: pid) modifies location, collision, c_next, ptop, ti_cell; {
  var pos: int;
  var cas: bool;
  var him: pid;
  var q: threadInfo;
  var trycollision: bool;
  var tryperform: bool;
  
  while (true) {
    location[mypid] := p;
    havoc pos;
    
    while (!cas) {
      him := collision[pos];
      call corral_atomic_begin();
      if (him == collision[pos]) {
        collision[pos] := mypid;
        cas := true; 
      }
      call corral_atomic_end();
    }
    if (him != empty) {
      q := location[him];
      if (q != nullTI && ti_id[q] == him && ti_op[q] == ti_op[p]) {
        cas := false;
        call corral_atomic_begin();
        if (location[mypid] == p) { location[mypid] := nullTI; cas := true; }
        call corral_atomic_end();
        if (cas) {
          call trycollision := tryCollision(p,q,him,mypid);
          if (trycollision) { return; }
          else { goto stack; }
        } else {
          call finishCollision(p,mypid);
          return;
        }
      }
    }
  
    cas := false;
    call corral_atomic_begin();
    if (location[mypid] == p) { location[mypid] := nullTI; cas := true; }
    call corral_atomic_end();
    
    if (!cas) {
      call finishCollision(p,mypid);
      return;
    }
    
    stack:
    call tryperform := tryPerformStackOp(p,mypid);
    if (tryperform) { return; }
  }
}

procedure tryPerformStackOp(p: threadInfo, pid: pid) returns (b: bool) modifies c_next, ptop, ti_cell; {
  var phead: cell;
  var pnext: cell;
  var cas: bool;
  
  if (ti_op[p] == PUSH) {
    phead := ptop;
    c_next[ti_cell[p]] :=  phead;
    b := false;
    call corral_atomic_begin();
    if (ptop == phead) { ptop := ti_cell[p]; b := true; }
    call corral_atomic_end();
    return;
  }
  
  if (ti_op[p] == POP) {
    phead := ptop;
    if (phead == nullCell) {
      ti_cell[p] := phead;
      b := true; return;
    }
    pnext := c_next[phead];
    b := false;
    call corral_atomic_begin();
    if (ptop == phead) { ptop := pnext; b := true; }
    call corral_atomic_end();
    
    if (b) {
      ti_cell[p] := phead;
      return;
    } else {
      ti_cell[p] := nullCell;
      return;
    }
  }
    
    
}

procedure stackOp(p: threadInfo, mypid: pid) modifies location, collision, c_next, ptop, ti_cell; {
  var b: bool;
  call b := tryPerformStackOp(p,mypid);
  if (!b) { call lesOP(p,mypid); }
  return;
}

procedure finishCollision(p: threadInfo, mypid: pid) modifies location, ti_cell; {
  if (ti_op[p] == POP) {
    ti_cell[p] := ti_cell[location[mypid]];
    location[mypid] := nullTI;
  }
}

procedure tryCollision(p: threadInfo, q: threadInfo, him: pid, mypid: pid) returns (b: bool) modifies location, ti_cell; {
  b := false;
  if (ti_op[p] == PUSH) {
    call corral_atomic_begin();
    if (location[him] == q) { location[him] := p; b := true; }
    call corral_atomic_end();
    return;
  }
  if (ti_op[p] == POP) {
    call corral_atomic_begin();
    if (location[him] == q) { location[him] := nullTI; b := true; }
    call corral_atomic_end();
    if (b) {
      ti_cell[p] := ti_cell[q];
      location[mypid] := nullTI;
      return;
    } else { return; }
  }
}

procedure Main () {
  assume (PUSH != POP);
}
    
    
    
    