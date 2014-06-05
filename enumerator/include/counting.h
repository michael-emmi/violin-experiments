/*****************************************************************************/
/** COUNTING                                                                **/
/*****************************************************************************/

#include <map>

enum violin_op_t { ADD_OP, REMOVE_OP };

int ***counters;
int num_methods;
int interval_bound;
int time_offset, last_time;

void __init_counters(int N, int M) {
  interval_bound = N+1;
  num_methods = M;
  counters = new int**[M];
  for (int m=0; m<M; m++) {
    counters[m] = new int*[interval_bound];
    for (int i=0; i<interval_bound; i++) {
      counters[m][i] = new int[interval_bound+1];
    }
  }
}

void __shift_counters() {
  for (int m=0; m<num_methods; m++) {
    for (int i=0; i<interval_bound; i++) {
      for (int j=0; j<interval_bound; j++) {
        if (i==0 && j==0)
          continue;
        int ti = i>0 ? i-1 : 0;
        int tj = j>0 ? j-1 : 0;
        counters[m][ti][tj] += counters[m][i][j];
        counters[m][i][j] = 0;
      }
    }
    for (int i=1; i<interval_bound; i++) {
      counters[m][i-1][interval_bound] += counters[m][i][interval_bound];
      counters[m][i][interval_bound] = 0;
    }
  }
}

void __reset_counters() {
  for (int m=0; m<num_methods; m++)
    for (int i=0; i<interval_bound; i++)
      for (int j=0; j<interval_bound+1; j++)
        counters[m][i][j] = 0;
  last_time = 0;
  time_offset = 0;
}

int __method(violin_op_t op, int v) {
  if (op == ADD_OP) {
    return 0;
  } else {
    return 1;
  }
}

void __count(violin_op_t op, int v, int start_time, int end_time = INFINITY) {
  int now = current_time();
  while (last_time < now) {
    last_time++;
    if (last_time > time_bound) {
      __shift_counters();
      time_offset++;
    }
  }

  start_time -= time_offset;
  if (start_time < 0) start_time = 0;

  int m = __method(op,v);
  if (end_time == INFINITY) {
    counters[m][start_time][interval_bound]++;
  } else {
    counters[m][start_time][interval_bound]--;
    counters[m][start_time][end_time-time_offset]++;
  }
}

void __print_counters() {
  cout << "--+";
  for (int j=0; j<interval_bound+1; j++)
    cout << "--";
  cout << endl;
  for (int m=0; m<num_methods; m++) {
    cout << "M" << m << "|";
    for (int j=0; j<interval_bound; j++)
      cout << " " << j;
    cout << " *" << endl;
    cout << "--+";
    for (int j=0; j<interval_bound+1; j++)
      cout << "--";
    cout << endl;
    for (int i=0; i<interval_bound; i++) {
      cout << i << " |";
      for (int j=0; j<interval_bound+1; j++) {
        if (i <= j)
          cout << " " << counters[m][i][j];
        else
          cout << " .";
      }
      cout << endl;
    }
    cout << "--+";
    for (int j=0; j<interval_bound+1; j++)
      cout << "--";
    cout << endl;
  }
}

/** CONTAINER SPECIFIC **/

bool __exists(violin_op_t op, int v, int L=0, int R=interval_bound) {
  int m = __method(op,v);
  for (int i=L; i<R; i++)
    for (int j=L; j<R; j++)
      if (counters[m][i][j])
        return true;
  return false;
}

bool __is_added(int v) {
  return __exists(ADD_OP,v);
}

bool __is_removed(int v) {
  return __exists(REMOVE_OP,v);
}

int __total(violin_op_t op, int v, int L=0, int R=interval_bound) {
  int count;
  int m = __method(op,v);
  for (int i=L; i<R; i++)
    for (int j=L; j<R; j++)
      count += counters[m][i][j];
  return count;
}

int __num_added(int v) {
  return __total(ADD_OP,v);
}

int __num_removed(int v) {
  return __total(REMOVE_OP,v);
}

bool __remove_violation(int v) {
  if (v == EMPTY_VAL || v == UNKNOWN_VAL) return false;
  return __num_added(v) < __num_removed(v);
}

pair<int,int> __span(violin_op_t op, int v, int L=0, int R=interval_bound+1) {
  int m = __method(op,v);
  int lhs = INFINITY;
  int rhs = 0;
  for (int i=L; i<R; i++) {
    for (int j=L; j<R; j++) {
      if (counters[m][i][j] > 0) {
        lhs = i;
        rhs = (j == interval_bound) ? INFINITY : j;
        goto FOUND_LHS;
      }
    }
  }
FOUND_LHS:
  if (lhs == INFINITY || rhs == INFINITY)
    return make_pair(lhs,rhs);
  OUT_AGAIN: for (int j=R-1; j>=0; j--) {
    for (int i=L; i<R; i++) {
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
      && before(__span(ADD_OP,v), span)
      && before(span, __span(REMOVE_OP,v));
}

// TWO barriers required to observe this one.
bool __remove_empty_violation(int L=0, int R=interval_bound) {
  if (!__exists(REMOVE_OP,EMPTY_VAL))
    return false;
  int m = __method(REMOVE_OP,EMPTY_VAL);
  for (int i=L; i<R; i++)
    for (int j=L; j<R; j++)
      if (counters[m][i][j] > 0)
        for (int v=0; v<0; v++) // TODO FIXME
          if (__present_during(v,make_pair(i,j)))
            return true;
  return false;
}

// THREE barriers required to observe this one.
bool __order_violation(int u, int v) {
  pair<int,int>
    addu = __span(ADD_OP,u),
    remu = __span(REMOVE_OP,u),
    addv = __span(ADD_OP,v),
    remv = __span(REMOVE_OP,v);

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
  if (current_time() >= 2 && __remove_empty_violation()) {
    hout << "(Ev)";
    num_violations++;
    violation_happened = true;
  }
}

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/

typedef pair<int,int> interval;
typedef map<interval,int> counter;
map<int,counter> added;
map<int,counter> removed;

int num_added(int v) {
  int n = 0;
  for (counter::iterator i = added[v].begin(); i != added[v].end(); ++i)
    n += i->second;
  return n;
}

int num_removed(int v) {
  int n = 0;
  for (counter::iterator i = removed[v].begin(); i != removed[v].end(); ++i)
    n += i->second;
  return n;
}

bool is_added(int v) {
  return added[v].size() > 0;
}

bool is_removed(int v) {
  return removed[v].size() > 0;
}


bool interval_before(pair<int,int> s1, pair<int,int> s2) {
  return s1.second < s2.first;
}

pair<int,int> interval_union(pair<int,int> s1, pair<int,int> s2) {
  return make_pair(min(s1.first, s2.first),  max(s1.second, s2.second));
}

pair<int,int> add_span(int v) {
  pair<int,int> s;
  s.first = INFINITY;
  s.second = 0;
  for (counter::iterator i = added[v].begin(); i != added[v].end(); ++i)
    if (i->second > 0) s = interval_union(s,i->first);
  return s;
}

pair<int,int> remove_span(int v) {
  pair<int,int> s;
  s.first = INFINITY;
  s.second = 0;
  for (counter::iterator i = removed[v].begin(); i != removed[v].end(); ++i)
    if (i->second > 0) s = interval_union(s,i->first);
  return s;
}

void print_counters() {
  cout << endl;
  cout << "Added: ";
  for (map<int,counter>::iterator i = added.begin(); i != added.end(); ++i) {
    if (i != added.begin()) cout << ", ";
    int v = i->first;
    counter c = i->second;
    for (counter::iterator j = c.begin(); j != c.end(); ++j) {
      if (j != c.begin()) cout << ", ";
      interval k = j->first;
      int n = j->second;
      cout << v << "@[" << k.first << ",";
      if (k.second < 0) cout << "_"; else cout << k.second;
      cout << "]:" << n;
    }
  }
  cout << endl;
  cout << "Removed:";
  for (map<int,counter>::iterator i = removed.begin(); i != removed.end(); ++i) {
    if (i != removed.begin()) cout << ", ";
    int v = i->first;
    counter c = i->second;
    for (counter::iterator j = c.begin(); j != c.end(); ++j) {
      if (j != c.begin()) cout << ", ";

      interval k = j->first;
      int n = j->second;
      cout << v << "@[" << k.first << ",";
      if (k.second < 0) cout << "_"; else cout << k.second;
      cout << "]:" << n;
    }
  }
  cout << endl;
}

// THREE barriers required to observe this one.
bool order_violation(int u, int v) {
  switch (violin_order) {
  case LIFO_ORDER:
    return u != v && is_removed(u) && is_removed(v)
      && interval_before(add_span(u), add_span(v))
      && interval_before(add_span(v), remove_span(u))
      && interval_before(remove_span(u), remove_span(v));
  case FIFO_ORDER:
    return u != v && is_removed(u) && is_removed(v)
      && interval_before(add_span(v), add_span(u))
      && interval_before(add_span(u), remove_span(u))
      && interval_before(remove_span(u), remove_span(v));
  default:
    return false;
  }
}

bool order_violation(int v) {
  if (v == UNKNOWN_VAL || v == EMPTY_VAL || !is_removed(v)) return false;
  for (map<int,counter>::iterator i = added.begin(); i != added.end(); ++i) {
    if (order_violation(i->first,v))
      return true;
  }
  return false;
}

bool remove_violation(int v) {
  if (v == EMPTY_VAL || v == UNKNOWN_VAL) return false;
  return num_added(v) < num_removed(v);
}

bool present_during(int v, pair<int,int> span) {
  return is_added(v)
      && interval_before(add_span(v), span)
      && interval_before(span, remove_span(v));
}

// Is there some remove-empty operation whose span is consumed by the presence
// of some element? e.g. is there some element V such that
// * V was (last) added before the remove-empty
// * V was (first) removed after the remove-empty (if ever)
// NOTE this is incomplete; in general one must see a set of elements whose
// collective presence consumes the span of the remove-empty.
bool remove_empty_violation() {
  for (counter::iterator i = removed[EMPTY_VAL].begin(); i != removed[EMPTY_VAL].end(); ++i) {
    pair<int,int> span = i->first;
    if (i->second < 1) continue;
    for (map<int,counter>::iterator j = added.begin(); j != added.end(); ++j) {
      int v = j->first;
      if (present_during(v,span)) return true;
    }
  }
  return false;
}

void check_counting_violations() {
  if (current_time() >= 2 && remove_empty_violation()) {
    hout << "(Ev)";
    num_violations++;
    violation_happened = true;
  }
}

void count(violin_op_t op, int v, int start_time, int end_time = INFINITY) {
  if (end_time == INFINITY) {
    switch (op) {
    case ADD_OP:
      added[v][make_pair(start_time,INFINITY)]++;
      break;
    case REMOVE_OP:
      removed[UNKNOWN_VAL][make_pair(start_time,INFINITY)]++;
    }
  } else {
    switch (op) {
    case ADD_OP:
      added[v][make_pair(start_time,INFINITY)]--;
      added[v][make_pair(start_time,end_time)]++;
      break;
    case REMOVE_OP:
      removed[UNKNOWN_VAL][make_pair(start_time,INFINITY)]--;
      removed[v][make_pair(start_time,end_time)]++;
      if (remove_violation(v)) {
        hout << "(Rv) ";
        num_violations++;
        violation_happened = true;
      }
      if (start_time >= 3 && order_violation(true,v)) {
        hout << "(Ov)";
        num_violations++;
        violation_happened = true;
      }
      break;
    }
  }
}

void reset_counters() {
  added.clear();
  removed.clear();
}
