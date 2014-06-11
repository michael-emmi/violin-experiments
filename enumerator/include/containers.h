/*****************************************************************************/
/** COUNTING FOR CONTAINERS                                                 **/
/*****************************************************************************/

class CollectionCountingMonitor : public CountingMonitor {
  const int num_values;

public:
  CollectionCountingMonitor(int N, int V)
    : CountingMonitor(N+1,(2*V+4)), num_values(V) {
  }
  void onPreExecute() {
    CountingMonitor::onPreExecute();
  }
  void onPostExecute() {
    check_violations();
    CountingMonitor::onPostExecute();
  }

private:

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

  int method(violin_op_t op, int v, int r) {
    if (op == ADD_OP)
      return (v <= num_values) ? v-1 : (2 * num_values + 2);

    if (r == EMPTY_VAL)
      return 2 * num_values;
  
    if (r == UNKNOWN_VAL)
      return 2 * num_values + 1;

    return (r <= num_values) ? (num_values + r-1) : (2 * num_values + 3);
  }

  bool exists(int m) {
    int L=0, R=interval_bound;
    for (int i=L; i<R; i++)
      for (int j=L; j<R; j++)
        if (counters[m][i][j])
          return true;
    return false;
  }

  bool is_added(int v) {
    return exists(method(ADD_OP,v,0));
  }

  bool is_removed(int r) {
    return exists(method(REMOVE_OP,0,r));
  }

  int total(int m) {
    int L=0, R=interval_bound;
    int count = 0;
    for (int i=L; i<R; i++)
      for (int j=L; j<R; j++)
        count += counters[m][i][j];
    return count;
  }

  int num_added(int v) {
    return total(method(ADD_OP,v,0));
  }

  int num_removed(int r) {
    return total(method(REMOVE_OP,0,r));
  }

  pair<int,int> span(int m) {
    int L=0, R=interval_bound;
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

  bool present_during(int v, pair<int,int> intv) {
    return is_added(v)
        && before(span(method(ADD_OP,v,0)), intv)
        && before(intv, span(method(REMOVE_OP,0,v)));
  }

  bool remove_violation(int r) {
    if (r == EMPTY_VAL || r == UNKNOWN_VAL) return false;
    return num_added(r) < num_removed(r);
  }

  // TWO barriers required to observe this one.
  bool remove_empty_violation() {
    int L=0, R=interval_bound;
    int m = method(REMOVE_OP,0,EMPTY_VAL);
    if (!exists(m)) return false;
    for (int i=L; i<R; i++)
      for (int j=L; j<R; j++)
        if (counters[m][i][j] > 0)
          for (int v=1; v<=num_values; v++)
            if (present_during(v,make_pair(i,j)))
              return true;
    return false;
  }

  // THREE barriers required to observe this one.
  bool order_violation(int u, int v) {
    pair<int,int>
      addu = span(method(ADD_OP,u,0)),
      remu = span(method(REMOVE_OP,0,u)),
      addv = span(method(ADD_OP,v,0)),
      remv = span(method(REMOVE_OP,0,v));

    switch (violin_order) {
    case LIFO_ORDER:
      return u != v && is_removed(u) && is_removed(v)
        && before(addu,addv) && before(addv,remu) && before(remu,remv);
    case FIFO_ORDER:
      return u != v && is_removed(u) && is_removed(v)
        && before(addv,addu) && before(addu,remu) && before(remu,remv);
    default:
      return false;
    }
  }

  void check_violations() {
    bool found = false;

    for (int v=1; v<=num_values; v++) {
      if (remove_violation(v)) {
        hout << "(Rv:" << v << ") ";
        found = true;
      }
      if (current_time()-time_offset < 3) continue;
      for (int u=1; u<=num_values; u++) {
        if (u == v) continue;
        if (order_violation(u,v)) {
          hout << "(Ov:" << u << "," << v << ") ";
          found = true;
        }
      }
    }

    if (current_time() >= 2 && remove_empty_violation()) {
      hout << "(Ev) ";
      found = true;
    }

    if (found) {
      violationCount++;
      violationFound = true;
    }
  }
};
