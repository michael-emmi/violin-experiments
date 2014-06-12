/*****************************************************************************/
/** COUNTING FOR CONTAINERS                                                 **/
/*****************************************************************************/

class CollectionCountingMonitor : public CountingMonitor {
  const int num_values;

  bool check_empty_violations;
  bool check_order_violations;
  bool check_remove_violations;

public:
  CollectionCountingMonitor(int N, int V)
    : CountingMonitor(N,(2*V+4)),
      num_values(V),
      check_empty_violations(true),
      check_order_violations(true),
      check_remove_violations(true)
    { }
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
      return (v <= num_values) ? (v - 1) : (2 * num_values + 2);

    if (r == EMPTY_VAL)
      return 2 * num_values;
  
    if (r == UNKNOWN_VAL)
      return 2 * num_values + 1;

    return (r <= num_values) ? (num_values + r - 1) : (2 * num_values + 3);
  }

  int total(int m) {
    int count = 0;
    int n = idx(m,interval_bound-1,interval_bound);
    for (int i=idx(m,0,0); i <= n; i++)
      count += counters[i];
    return count;
  }

  pair<int,int> span(int m) {
    int min = INFINITY;
    int max = -1;

    for (int i = 0, offset = idx(m,0,0);
         i < interval_bound;
         i++, offset += (interval_bound+1) ) {

      if (counters[offset+interval_bound] > 0) {
        min = i;
        max = INFINITY;
        goto DONE;
      }
      for (int j = i; j < interval_bound; j++) {
        if (counters[offset + j] > 0) {
          min = i;
          goto FOUND_MIN;
        }
      }
    }
    goto DONE;

  FOUND_MIN:
    for (int j = interval_bound-1; j >= 0; j--) {
      for (int i = j, offset = idx(m,i,j);
           i >= 0;
           i--, offset -= (interval_bound+1)) {

        if (counters[offset] > 0) {
          max = j;
          goto DONE;
        }
      }
    }

  DONE:
    return make_pair(min,max);
  }
  
  bool exists(pair<int,int> intv) {
    return intv.second > -1;
  }

  bool before(pair<int,int> fst, pair<int,int> snd) {
    return fst.second < snd.first;
  }

  bool remove_violation(int r) {
    if (r == EMPTY_VAL || r == UNKNOWN_VAL) return false;
    int n = total(method(REMOVE_OP,0,r));
    return n > 0 && n > total(method(ADD_OP,r,0));
  }

  // TWO barriers required to observe this one.
  bool remove_empty_violation() {
    int m = method(REMOVE_OP,0,EMPTY_VAL);
    pair<int,int> reme = span(m);
    if (!exists(reme)) return false;

    for (int v=1; v<=num_values; v++) {
      pair<int,int>
        addv = span(method(ADD_OP,v,0)),
        remv = span(method(REMOVE_OP,0,v));
      for (int i=reme.first; i<=reme.second; i++)
        for (int j=reme.first; j<=reme.second; j++)
          if (counters[idx(m,i,j)] > 0 && addv.second < i && j < remv.first)
            return true;
    }
    return false;
  }

  // THREE barriers required to observe this one.
  bool order_violation(int u, int v) {
    if (u == v)
      return false;

    pair<int,int>
      addu = span(method(ADD_OP,u,0)),
      remu = span(method(REMOVE_OP,0,u)),
      addv = span(method(ADD_OP,v,0)),
      remv = span(method(REMOVE_OP,0,v)),
      remuu = span(method(REMOVE_OP,0,UNKNOWN_VAL));
      
    if (!exists(addu) || !exists(addv) || !exists(remv))
      return false;
      
    switch (violin_order) {
    case LIFO_ORDER:
      return before(addv,addu) && before(addu,remv) && before(remv,remu);
    case FIFO_ORDER:
      return before(addu,addv) && (
        (exists(remu) && before(remv,remu))
        || (!exists(remu) && before(remv,remuu)));
    default:
      return false;
    }
  }

  void check_violations() {

    if (check_remove_violations || check_order_violations) {
      for (int v = 1; v <= num_values; v++) {
        if (check_remove_violations && remove_violation(v)) {
          hout << "(Rv:" << v << ") ";
          goto FOUND;
        }

        if (!check_order_violations || current_time()-time_offset < 1)
          continue;

        for (int u = 1; u <= num_values; u++) {
          if (u == v) continue;
          if (order_violation(u,v)) {
            hout << "(Ov:" << u << "," << v << ") ";
            goto FOUND;
          }
        }
      }
    }

    if (check_empty_violations
        && current_time()-time_offset > 1
        && remove_empty_violation()) {
      hout << "(Ev) ";
      goto FOUND;
    }

    return;

  FOUND:
    violationFound = true;
    violationCount++;
    return;
  }

};
