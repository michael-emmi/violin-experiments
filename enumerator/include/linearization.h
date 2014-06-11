/*****************************************************************************/
/** LINEARIZATION                                                           **/
/*****************************************************************************/

#include <vector>
#include <queue>
#include <unordered_set>
#include <set>
#include <regex>

int SpecAdd(int op_id, int v) {
  hout << op_id << ":Add(" << v << ")? ";
  violin_spec_object.add(v);
  hout << op_id << ":Add! ";
  return 0;
}

int SpecRemove(int op_id, int v) {
  hout << op_id << ":Rem? ";
  int r = violin_spec_object.remove();
  hout << op_id << ":Rem" << "!";
  if (r == EMPTY_VAL) hout << "E"; else hout << r;
  hout << " ";
  return r;
}

class SequentialExecutionListener : public ExecutionListener {
  unordered_set<string> &histories;
public:
  SequentialExecutionListener(unordered_set<string> &hs) : histories(hs) { }
  void onPreExecute() {
    hout.str("");
    hout.clear();
    violin_spec_object.initialize();
  }
  void onPostExecute() {
    static regex op_id("[0-9]+:");
    static regex add_call("Add[(]([0-9]+)[)][?]");
    static regex add_ret("Add! ");
    static regex rem_call("Rem[?] ");
    static regex rem_ret("Rem!([0-9E]+)");
    static regex marks("[?!]");

    string h = hout.str();
    h = regex_replace(h,op_id,"");
    h = regex_replace(h,add_call,"Add($1)");
    h = regex_replace(h,add_ret,"");
    h = regex_replace(h,rem_call,"");
    h = regex_replace(h,rem_ret,"Rem($1)");
    // h = regex_replace(h,marks,"");
    histories.insert(h);
  }
};

class LinearizationMonitor : public Monitor {
  unordered_set<string> linear_histories;
  int num_adds, num_removes;

public:
  LinearizationMonitor(int A, int R)
    : Monitor("Line-Up"), num_adds(A), num_removes(R) {
    computeSequentialHistories();
  }
  void onPreExecute() {
    Monitor::onPreExecute();
  }
  void onPostExecute() {
    check_violations();
    Monitor::onPostExecute();
  }
  void onCall(int op_id, violin_op_t op, int val, int start_time) { }
  void onReturn(
    int op_id, violin_op_t op, int val, int ret_val,
    int start_time, int end_time) { }

private:
  struct OP {
    Operation *op;
    int start() { return op->start_time; }
    int finish() { return op->end_time; }
    string to_string() {
      stringstream s;
      if (op->proc == Add)
        s << "Add(" << op->parameter << ")";
      else if (op->proc == Remove) {
        s << "Rem(";
        if (op->result == EMPTY_VAL) s << "E";
        else s << op->result;
        s << ")";
      } else
        s << "????";
      return s.str();
    }
    bool operator< (const OP& o) const {
      return op->id < o.op->id;
    }
    bool operator== (const OP& o) const {
      return op->id == o.op->id;
    }
  };

  void computeSequentialHistories() {
    AtomicThreadEnumerator e;
    e.addListener(new SequentialExecutionListener(linear_histories));
    for (int i=0; i<num_adds; i++)
      e.addThread(&Operation::run, (void*) new Operation(SpecAdd,i+1,0));
    for (int i=0; i<num_removes; i++)
      e.addThread(&Operation::run, (void*) new Operation(SpecRemove,0,0));

    cout << "Computing sequential histories... ";
    time_t start_time, end_time;
    time(&start_time);
    e.run();
    time(&end_time);

    cout << linear_histories.size() << " histories computed in " 
         << difftime(end_time,start_time) << "s." << endl;
    for (unordered_set<string>::iterator h = linear_histories.begin();
         h != linear_histories.end(); ++h) {
      cout << *h << endl;
    }
  }

  string linearization_to_string(vector<OP> l) {
    stringstream s;
    for (vector<OP>::iterator op = l.begin(); op != l.end(); ++op)
      s << op->to_string() << " ";
    return s.str();
  }

  void check_violations() {
    list<OP> operations;
    queue< pair< vector<OP>, list<OP> > > work_list;
    vector< vector<OP> > linearizations;
    vector<OP> empty_seq;

    for (vector<Operation*>::iterator op = violin_operations.begin();
        op != violin_operations.end(); ++op)
      operations.push_back({.op = *op});

    work_list.push(make_pair(empty_seq,operations));

    while (!work_list.empty()) {
      vector<OP> sequence = work_list.front().first;
      list<OP> remaining = work_list.front().second;
      work_list.pop();

      if (remaining.empty()) {
        if (linear_histories.find(linearization_to_string(sequence))
            != linear_histories.end()) {
          // cout << ":-) " << linearization_to_string(sequence) << endl;
          return;
        } else {
          // cout << ":-X " << linearization_to_string(sequence) << endl;
          linearizations.push_back(sequence);
          continue;
        }
      }

      int min = INFINITY;
      for (list<OP>::iterator o = remaining.begin(); o != remaining.end(); ++o)
        if (o->finish() < min)
          min = o->finish();

      list<OP> minimals;
      for (list<OP>::iterator o = remaining.begin(); o != remaining.end(); ++o) {
        if (o->start() <= min) {
          minimals.push_back(*o);
        }
      }

      for (list<OP>::iterator o = minimals.begin(); o != minimals.end(); ++o) {
        vector<OP> sequence2(sequence);
        list<OP> remaining2(remaining);
        sequence2.push_back(*o);
        remaining2.remove(*o);
        work_list.push(make_pair(sequence2,remaining2));
      }
    }
  
    hout << "(Lv) ";
    violationFound = true;
    violationCount++;

    // hout << "(" << linearizations.size() << "Ls) ";

    // cout << endl;
    // for (vector< vector<OP> >::iterator lin = linearizations.begin(); lin != linearizations.end(); ++lin) {
    //   for (vector<OP>::iterator o = lin->begin(); o != lin->end(); ++o)
    //     cout << o->id << "(" << o->start << "," << o->finish << ")";
    //   cout << endl;
    // }
  }
};