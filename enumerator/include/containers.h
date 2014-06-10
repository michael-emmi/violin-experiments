/*****************************************************************************/
/** COUNTING FOR CONTAINERS                                                 **/
/*****************************************************************************/

int num_values;

// THE NUMBERING
// idx    operation
// ----------------
// 0      add(1)
// 1      add(2)
// ...
// V-1    add(V)
// V      rem(1)
// V+1    rem(2)
// ...
// 2V-1   rem(V)
// 2V     rem(-1)
// 2V+1   rem(?)
// 2V+2   add(*)
// 2V+3   rem(*)

int container_method(violin_op_t op, int v, int r) {
  if (op == ADD_OP)
    return (v <= num_values) ? v-1 : (2 * num_values + 2);

  if (r == EMPTY_VAL)
    return 2 * num_values;
  
  if (r == UNKNOWN_VAL)
    return 2 * num_values + 1;

  return (r <= num_values) ? (num_values + r-1) : (2 * num_values + 3);
}

void init_container_counters(int N, int V) {
  num_values = V;
  __init_counters(N, 2*V+4, container_method);
}

bool __exists(int m, int L=0, int R=interval_bound) {
  for (int i=L; i<R; i++)
    for (int j=L; j<R; j++)
      if (counters[m][i][j])
        return true;
  return false;
}

bool __is_added(int v) {
  return __exists(container_method(ADD_OP,v,0));
}

bool __is_removed(int r) {
  return __exists(container_method(REMOVE_OP,0,r));
}

int __total(int m, int L=0, int R=interval_bound) {
  int count = 0;
  for (int i=L; i<R; i++)
    for (int j=L; j<R; j++)
      count += counters[m][i][j];
  return count;
}

int __num_added(int v) {
  return __total(container_method(ADD_OP,v,0));
}

int __num_removed(int r) {
  return __total(container_method(REMOVE_OP,0,r));
}

bool __remove_violation(int r) {
  if (r == EMPTY_VAL || r == UNKNOWN_VAL) return false;
  return __num_added(r) < __num_removed(r);
}

pair<int,int> __span(int m, int L=0, int R=interval_bound) {
  int lhs = INFINITY;
  int rhs = 0;
  for (int i=L; i<R; i++) {
    if (counters[m][i][interval_bound] > 0)
      return make_pair(lhs,INFINITY);

    for (int j=i; j<R; j++) {
      if (counters[m][i][j] > 0) {
        lhs = i;
        rhs = j;
        goto FOUND_LHS;
      }
    }
  }
FOUND_LHS:
  if (lhs == INFINITY)
    return make_pair(lhs,rhs);

  for (int j=R-1; j>=0; j--) {
    for (int i=j; i>=0; i--) {
      if (counters[m][i][j] > 0) {
        rhs = (j == interval_bound) ? INFINITY : j;
        goto FOUND_RHS;
      }
    }
  }
FOUND_RHS:
  return make_pair(lhs,rhs);
}

bool before(pair<int,int> fst, pair<int,int> snd) {
  return fst.second < snd.first;
}

bool __present_during(int v, pair<int,int> span) {
  return __is_added(v)
      && before(__span(container_method(ADD_OP,v,0)), span)
      && before(span, __span(container_method(REMOVE_OP,0,v)));
}

// TWO barriers required to observe this one.
bool __remove_empty_violation(int L=0, int R=interval_bound) {
  int m = container_method(REMOVE_OP,0,EMPTY_VAL);
  if (!__exists(m)) return false;
  for (int i=L; i<R; i++)
    for (int j=L; j<R; j++)
      if (counters[m][i][j] > 0)
        for (int v=1; v<=num_values; v++)
          if (__present_during(v,make_pair(i,j)))
            return true;
  return false;
}

// THREE barriers required to observe this one.
bool __order_violation(int u, int v) {
  pair<int,int>
    addu = __span(container_method(ADD_OP,u,0)),
    remu = __span(container_method(REMOVE_OP,0,u)),
    addv = __span(container_method(ADD_OP,v,0)),
    remv = __span(container_method(REMOVE_OP,0,v));

  switch (violin_order) {
  case LIFO_ORDER:
    return u != v && __is_removed(u) && __is_removed(v)
      && before(addu,addv) && before(addv,remu) && before(remu,remv);
  case FIFO_ORDER:
    return u != v && __is_removed(u) && __is_removed(v)
      && before(addv,addu) && before(addu,remu) && before(remu,remv);
  default:
    return false;
  }
}

void __check_counting_violations() {

  for (int v=1; v<=num_values; v++) {
    if (__remove_violation(v)) {
      hout << "(Rv:" << v << ") ";
      if (!violation_happened) {
        num_violations++;
        violation_happened = true;
      }
    }
    if (current_time()-time_offset < 3) continue;
    for (int u=1; u<=num_values; u++) {
      if (u == v) continue;
      if (__order_violation(u,v)) {
        hout << "(Ov: " << u << ":" << v << ") ";
        if (!violation_happened) {
          num_violations++;
          violation_happened = true;
        }
      }
    }
  }

  if (current_time() >= 2 && __remove_empty_violation()) {
    hout << "(Ev) ";
    if (!violation_happened) {
      num_violations++;
      violation_happened = true;
    }
  }
}
