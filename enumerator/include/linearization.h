/*****************************************************************************/
/** LINEARIZATION                                                           **/
/*****************************************************************************/

#include <vector>
#include <queue>
#include <unordered_set>
#include <set>
#include <regex>

class SequentialExecutionCollector : public ExecutionListener {
  Object spec_object;
  vector<Operation*> &operations;
  unordered_set<string> &histories;
  stringstream hout;
public:
  SequentialExecutionCollector(Object spec_obj, vector<Operation*> &ops, unordered_set<string> &hs)
    : spec_object(spec_obj), operations(ops), histories(hs) {}

  void onPreExecute() {
    hout.str("");
    hout.clear();
    spec_object.initialize();
  }
  void onComplete(int t) {
    hout << operations[t]->toString() << " ";
  }
  void onPostExecute() {
    histories.insert(hout.str());
  }
};

class LinearizationMonitor : public Monitor {
  Object spec_object;
  vector<Operation*> &operations;
  vector<Operation*> &spec_operations;
  unordered_set<string> valid_linear_histories;
  const bool debug = false;

  unsigned max_num_linearizations;
  unsigned total_num_linearizations;
  unsigned num_queries;

public:
  LinearizationMonitor(
    Object spec_obj, vector<Operation*> &ops,
    vector<Operation*> &spec_ops, bool collect,
    bool dont_compute_atomic_histories = false)
    : Monitor("Line-Up", collect), spec_object(spec_obj),
      operations(ops), spec_operations(spec_ops),
      max_num_linearizations(0), total_num_linearizations(0), num_queries(0) {

    if (!dont_compute_atomic_histories)
      computeSequentialHistories();
  }
  void onPostExecute() {
    check_violations();
  }
  void onCall(Operation *op) { }
  void onReturn(Operation *op) { }
  
  string extraInfo() {
    stringstream s;
    s << getName() << " performed "
      << avgLinearizations() << " (avg) / " << maxLinearizations() << " (max)"
      << " linearizations.";
    return s.str();
  }

  unsigned avgLinearizations() {
    if (num_queries > 0)
      return total_num_linearizations / num_queries;
    else
      return 0;
  }

  unsigned maxLinearizations() {
    return max_num_linearizations;
  }

private:
  void computeSequentialHistories() {
    AtomicThreadEnumerator e;
    for (vector<Operation*>::iterator op = spec_operations.begin();
        op != spec_operations.end(); ++op) {
      e.addThread(&Operation::run, (void*) (*op));
    }
    SequentialExecutionCollector sel(spec_object, spec_operations, valid_linear_histories);
    e.addListener(&sel);

    cout << "Computing sequential histories... ";
    timeval start_time, end_time;
    gettimeofday(&start_time,0);
    e.run();
    gettimeofday(&end_time,0);

    float diff = round(
      difftime(end_time.tv_sec, start_time.tv_sec)*100 +
      difftime(end_time.tv_usec, start_time.tv_usec)/10000) / 100;

    cout << valid_linear_histories.size() << " histories computed in " 
         << diff << "s." << endl;

    if (debug) {
      for (unordered_set<string>::iterator h = valid_linear_histories.begin();
           h != valid_linear_histories.end(); ++h) {
        cout << *h << endl;
      }
    }
  }

  string linearization_to_string(list<Operation*> l) {
    stringstream s;
    for (list<Operation*>::iterator op = l.begin(); op != l.end(); ++op)
      s << (*op)->toString() << " ";
    return s.str();
  }

  void check_violations() {
    bool is_violation = false;
    queue< pair< list<Operation*>, list<Operation*> > > work_list;
    list<Operation*> empty_seq;
    list<Operation*> ops;
    vector< list<Operation*> > linearizations;
    for (vector<Operation*>::iterator o = operations.begin(); o != operations.end(); ++o)
      ops.push_back(*o);

    work_list.push(make_pair(empty_seq,ops));

    list<string> tries; // for debugging

    unsigned num_linearizations = 0;
    num_queries++;

    while (!work_list.empty()) {
      list<Operation*> sequence = work_list.front().first;
      list<Operation*> remaining = work_list.front().second;
      work_list.pop();

      if (remaining.empty()) {
        num_linearizations++;
        if (valid_linear_histories.find(linearization_to_string(sequence))
            != valid_linear_histories.end()) {
          // cout << ":-) " << linearization_to_string(sequence) << endl;
          goto DONE;
        } else {
          // cout << ":-X " << linearization_to_string(sequence) << endl;
          if (debug)
            tries.push_back(linearization_to_string(sequence));
          linearizations.push_back(sequence);
          continue;
        }
      }

      int min = OMEGA;
      for (list<Operation*>::iterator o = remaining.begin(); o != remaining.end(); ++o)
        if ((*o)->endTime() < min)
          min = (*o)->endTime();

      list<Operation*> minimals;
      for (list<Operation*>::iterator o = remaining.begin(); o != remaining.end(); ++o) {
        if ((*o)->startTime() <= min) {
          minimals.push_back(*o);
        }
      }

      for (list<Operation*>::iterator o = minimals.begin(); o != minimals.end(); ++o) {
        list<Operation*> sequence2(sequence);
        list<Operation*> remaining2(remaining);
        sequence2.push_back(*o);
        remaining2.remove(*o);
        work_list.push(make_pair(sequence2,remaining2));
      }
    }
    
    if (debug) {
      cout << "[failed linearizations]" << endl;
      for (list<string>::iterator i = tries.begin(); i != tries.end(); ++i)
        cout << "X. " << (*i) << endl;
    }
  
    vstring = "(Lv)";
    violationCount++;
    is_violation = true;

  DONE:
    logHistory(new History(operations),is_violation);
    total_num_linearizations += num_linearizations;
    if (num_linearizations > max_num_linearizations)
      max_num_linearizations = num_linearizations;

    // hout << "(" << linearizations.size() << "Ls) ";

    // cout << endl;
    // for (vector< vector<History::Operation> >::iterator lin = linearizations.begin(); lin != linearizations.end(); ++lin) {
    //   for (vector<History::Operation>::iterator o = lin->begin(); o != lin->end(); ++o)
    //     cout << o->id << "(" << o->start << "," << o->finish << ")";
    //   cout << endl;
    // }
  }
};