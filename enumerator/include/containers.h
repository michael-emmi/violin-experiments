/*****************************************************************************/
/** COUNTING FOR CONTAINERS                                                 **/
/*****************************************************************************/

class CollectionCountingMonitor : public CountingMonitor {
  const int num_values;
  violin_order_t violin_order;
  bool check_empty_violations;
  bool check_order_violations;
  bool check_remove_violations;

public:
  CollectionCountingMonitor(
    int N, int V, violin_order_t ord,
    bool verify, bool collect)
    : CountingMonitor(N,(2*V+5),collect),
      num_values(V),
      violin_order(ord),
      check_empty_violations(verify),
      check_order_violations(verify),
      check_remove_violations(verify)
    { }
  void onReturn() {
    if (vstring == "")
      check_violations();
  }
  void onPostExecute() {
    if (vstring == "")
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
  // 2V+4   ???
  
  int add_method(int v) {
    return (v <= num_values) ? (v - 1) : (2 * num_values + 2);
  }

  int remove_method(int r, bool pending=false) {
    if (r == UNKNOWN_VAL) pending = true;
    if (pending) return 2 * num_values + 1;
    if (r == EMPTY_VAL) return 2 * num_values;
    return (r <= num_values) ? (num_values + r - 1) : (2 * num_values + 3);
  }

  int method(Operation *op) {
    AddOperation *add = dynamic_cast<AddOperation*>(op);
    if (add) return add_method(add->getParameter());

    RemoveOperation *rem = dynamic_cast<RemoveOperation*>(op);
    if (rem) return remove_method(rem->getResult(), rem->endTime() == OMEGA);

    return 2 * num_values + 4;
  }

  int total(int m) {
    int count = 0;
    int n = idx(m,interval_bound-1,interval_bound);
    for (int i=idx(m,0,0); i <= n; i++)
      count += counters[i];
    return count;
  }

  pair<int,int> span(int m) {
    int min = OMEGA;
    int max = -1;

    for (int i = 0, offset = idx(m,0,0);
         i < interval_bound;
         i++, offset += (interval_bound+1) ) {

      if (counters[offset+interval_bound] > 0) {
        min = i;
        max = OMEGA;
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
    int n = total(remove_method(r));
    return n > 0 && n > total(add_method(r));
  }

  // TWO barriers required to observe this one.
  bool remove_empty_violation() {
    int m = remove_method(EMPTY_VAL);
    pair<int,int> reme = span(m);
    if (!exists(reme)) return false;

    for (int v=1; v<=num_values; v++) {
      pair<int,int>
        addv = span(add_method(v)),
        remv = span(remove_method(v));
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
      addu = span(add_method(u)),
      remu = span(remove_method(u)),
      addv = span(add_method(v)),
      remv = span(remove_method(v)),
      remuu = span(remove_method(UNKNOWN_VAL,true));
      
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
    stringstream s;
    bool is_violation = false;

    if (check_remove_violations || check_order_violations) {
      for (int v = 1; v <= num_values; v++) {
        if (check_remove_violations && remove_violation(v)) {
          s << "(Rv:" << v << ")";
          goto FOUND;
        }

        if (!check_order_violations || last_time-time_offset < 1)
          continue;

        for (int u = 1; u <= num_values; u++) {
          if (u == v) continue;
          if (order_violation(u,v)) {
            s << "(Ov:" << u << "," << v << ")";
            goto FOUND;
          }
        }
      }
    }

    if (check_empty_violations
        && last_time-time_offset > 1
        && remove_empty_violation()) {
      s << "(Ev)";
      goto FOUND;
    }

    goto NOT_FOUND;

  FOUND:
    vstring = s.str();
    violationCount++;
    is_violation = true;

  NOT_FOUND:
    if (is_violation) // XXX needed to avoid memory exhaustion
      logHistory(historyOfCounters(), is_violation);
    return;
  }

  History *historyOfCounters() {
    vector<Operation*> ops;
    for (int i = 0; i < interval_bound; i++) {
      for (int j = 0; j <= interval_bound; j++) {
        int m;

        for (int v = 1; v <= num_values; v++) {
          m = add_method(v);
          for (int k = 0; k < counters[idx(m,i,j)]; k++) {
            Operation *op = new AddOperation(NULL,v);
            op->start(i);
            op->end(j == interval_bound ? OMEGA : j);
            ops.push_back(op);
          }
        }

        for (int v = 1; v <= num_values; v++) {
          m = remove_method(v);
          for (int k = 0; k < counters[idx(m,i,j)]; k++) {
            RemoveOperation *op = new RemoveOperation(NULL);
            op->start(i);
            op->end(j == interval_bound ? OMEGA : j);
            op->setResult(v);
            ops.push_back(op);
          }
        }

        m = remove_method(EMPTY_VAL);
        for (int k = 0; k < counters[idx(m,i,j)]; k++) {
          RemoveOperation *op = new RemoveOperation(NULL);
          op->start(i);
          op->end(j == interval_bound ? OMEGA : j);
          op->setResult(EMPTY_VAL);
          ops.push_back(op);
        }

        m = remove_method(UNKNOWN_VAL);
        for (int k = 0; k < counters[idx(m,i,j)]; k++) {
          Operation *op = new RemoveOperation(NULL);
          op->start(i);
          op->end(j == interval_bound ? OMEGA : j);
          ops.push_back(op);
        }
      }
    }
    return new History(ops);
  }

};
