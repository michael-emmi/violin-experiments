/*****************************************************************************/
/** COUNTING FOR CONTAINERS                                                 **/
/*****************************************************************************/

class CollectionCountingMonitor : public CountingMonitor {
  const int num_values;
  string vstring;

public:
  CollectionCountingMonitor(int N, int V)
    : CountingMonitor(N,(2*V+4)), num_values(V) {
  }
  void onPreExecute() {
    CountingMonitor::onPreExecute();
    vstring = "";
  }
  void onPostExecute() {
    if (vstring != "") {
      hout << vstring;
      violationCount++;
      violationFound = true;
    }
    CountingMonitor::onPostExecute();
  }

  void onReturn(
      int op_id, violin_op_t op, int val, int ret_val,
      int start_time, int end_time) {
    CountingMonitor::onReturn(op_id,op,val,ret_val,start_time,end_time);
    if (op == REMOVE_OP)
      check_violations(ret_val);
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

  int total(int m, bool includePending=true) {
    int L = 0, R = interval_bound;
    int count = 0;
    for (int i=L; i<R; i++) {
      for (int j=L; j<R; j++)
        count += counters[m][i][j];
      if (includePending)
        count += counters[m][i][interval_bound];
    }
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
    int min = INFINITY;
    int max = -1;
    
    for (int i=L; i<R; i++) {
      if (counters[m][i][interval_bound] > 0) {
        min = i;
        max = INFINITY;
        goto DONE;
      }
      for (int j=i; j<R; j++) {
        if (counters[m][i][j] > 0) {
          min = i;
          goto FOUND_MIN;
        }
      }
    }
    goto DONE;
  FOUND_MIN:
    for (int j=R-1; j>=0; j--) {
      for (int i=j; i>=0; i--) {
        if (counters[m][i][j] > 0) {
          max = j;
          goto DONE;
        }
      }
    }
  DONE:
    return make_pair(min,max);
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
        && before(addu,addv) && before(remv,remu);
    default:
      return false;
    }
  }
  
  void check_violations(int v) {
    stringstream s;

    if (vstring != "")
      return;
    
    if (v == EMPTY_VAL) {
      if (current_time()-time_offset >= 2 && remove_empty_violation())
        vstring = "(Ev) ";
      return;
    }

    if (remove_violation(v)) {
      s << "(Rv:" << v << ") ";
      vstring = s.str();
      return;
    }

    if (current_time()-time_offset < 2)
      return;

    for (int u=1; u<=num_values; u++) {
      if (u==v) continue;
      if (order_violation(u,v)) {
        s << "(Ov:" << u << "," << v << ") ";
        vstring = s.str();
        return;
      } else if (order_violation(v,u)) {
        s << "(Ov:" << v << "," << u << ") ";
        vstring = s.str();
        return;        
      }
    }
  }

};
