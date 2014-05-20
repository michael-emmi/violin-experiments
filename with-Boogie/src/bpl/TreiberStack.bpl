type val;
var stack: [int] val;
var head: int;

procedure Push ( v: val )
{
	var h: int;
	var done: bool;
	
	done := false;
	
	while (!done) {
		h := head;
		
		call corral_atomic_begin();
		if (h == head) {
			if (h < SIZE) {
				stack[h] := v;
				head := h+1;
				done := true;
			}
		}
		call corral_atomic_end();
	}
	return;
}

procedure Pop ( v: val )
{
	var h: int;
	var done: bool;
	
	done := false;
	
	while (!done) {
		h := head;
		
		call corral_atomic_begin();
		if (h == 0) {
			done := true;

		} else if (h == head) {
			if (h > 0) {
				assume stack[h] == v;
				head := h-1;
				done := true;
			}
		}
		call corral_atomic_end();
	}
	return;
}

procedure Main ()
{
	var v: val;
	head := 0;
	
	while (*) {
		if (*) {
			havoc v;
			call {:async 0} Push (v);
		} else if (*) {
			call {:async 0} v := Pop ();
		}
	}
	return;
}