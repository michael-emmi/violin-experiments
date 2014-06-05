/*****************************************************************************/
/** COUNTING                                                                **/
/*****************************************************************************/

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

void check_for_violations() {
  if (current_time() >= 2 && remove_empty_violation()) {
    hout << "(Ev)";
    num_violations++;
    violation_happened = true;
  }
}
