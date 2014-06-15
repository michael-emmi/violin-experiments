/*****************************************************************************/
/* VIOLIN: a detector of linearizability violations by schedule enumeration. */
/*****************************************************************************
 * Usage
 * Just call the "violin" function, e.g.,
 *
 *   int main() {
 *     violin(
 *       init_fn,
 *       add_fn, num_adds,
 *       remove_fn, num_removes,
 *       mode,
 *       allocation_policy,
 *       container_order,
 *       num_barriers, num_delays
 *     );
 *     return 0;
 *   }
 *
 * Note that you must specify a few arguments:
 * 1. void (*init_fn)(void)   for resetting your data to its initial state
 * 2. void (*add_fn)(int)     for adding an element
 * 3. int num_adds            how many add operations?
 * 4. int (*remove_fn)(void)  for removing an element
 * 5. int num_removes         how many remove operations?
 * 6. violin_mode_t mode      how to monitor?
 * 7. int allocation_policy   from DEFAULT_ALLOC, LRF_ALLOC, MRF_ALLOC
 * 8. int container_order     from NO_ORDER, LIFO_ORDER, FIFO_ORDER
 * 9. int num_barriers        how many barriers?
 * 10. int num_delays         how many delays?
 *
 * Once the "violin" function is called, every possible delay-bounded round
 * robin schedule of `num_adds` add operations followed by `num_removes`
 * remove operations with up to `num_delays` delays will be explored, and any
 * linearizability violation observable with up to `num_barriers` barries
 * witnessed by some execution will be reported.
 *
 *****************************************************************************/

#include <iostream>
#include <vector>
#include <time.h>

#include "enumeration.h"
#include "allocation.h"

using namespace std;

enum violin_order_t { NO_ORDER, LIFO_ORDER, FIFO_ORDER };
enum violin_mode_t { NOTHING_MODE, COUNTING_MODE, LINEARIZATIONS_MODE, VERSUS_MODE };
enum violin_show_t { SHOW_NONE, SHOW_WINS, SHOW_VIOLATIONS, SHOW_ALL };

const int OMEGA = 9999;
const int EMPTY_VAL = -1;
const int UNKNOWN_VAL = -2;
int num_executions;
int num_violations;

class Operation {
  static int unique_id;
protected:
  int id;
  int start_time, end_time;
  Operation() : id(unique_id++) {}
public:
  static void run(void*);
  bool operator<(const Operation &o) const { return end_time < o.start_time; }
  virtual bool equivalent(const Operation &o) const = 0;
  virtual void reset() { start_time = OMEGA; end_time = OMEGA; }
  virtual void start(int t) { start_time = t; end_time = OMEGA; }
  virtual void unend() { end_time = OMEGA; }
  virtual void end(int t) { end_time = t; }
  virtual void run() = 0;
  virtual string callString() = 0;
  virtual string retString() = 0;
  virtual string toString() = 0;
  int startTime() { return start_time; }
  int endTime() { return end_time; }
};

int Operation::unique_id = 0;

void Operation::run(void *context)  {
  Operation *op = (Operation*) context;
  op->run();
}

class AddOperation : public Operation {
  void (*addFn)(int);
  int parameter;
public:
  AddOperation(void (*add)(int), int param)
    : Operation(), addFn(add), parameter(param) {}
  int getParameter() const { return parameter; }
  bool equivalent(const Operation &o) const {
    const AddOperation *add = dynamic_cast<const AddOperation*>(&o);
    if (!add)
      return false;
    return add->parameter == parameter;
  }
  void run() {
    addFn(parameter);
  }
  string callString() {
    stringstream s;
    s << id << ":Add(" << parameter << ")?";
    return s.str();
  }
  string retString() {
    stringstream s;
    s << id << ":Add(" << parameter << ")!";
    return s.str();
  }
  string toString() {
    stringstream s;
    s << "Add(" << parameter << ")";
    return s.str();
  }
};

class RemoveOperation : public Operation {
  int (*removeFn)(void);
  int result;
public:
  RemoveOperation(int (*rem)(void))
    : Operation(), removeFn(rem), result(UNKNOWN_VAL) {}
  int getResult() const { return result; }
  bool equivalent(const Operation &o) const {
    const RemoveOperation *rem = dynamic_cast<const RemoveOperation*>(&o);
    if (!rem)
      return false;
    return rem->result == result;
  }
  void reset() { Operation::reset(); result = UNKNOWN_VAL; }
  void run() {
    result = removeFn();
  }
  string callString() {
    stringstream s;
    s << id << ":Rem()?";
    return s.str();
  }
  string retString() {
    stringstream s;
    s << id << ":Rem!";
    if (result == EMPTY_VAL) s << "E";
    else s << result;
    return s.str();
  }
  string toString() {
    stringstream s;
    s << "Rem(";
    if (result == EMPTY_VAL) s << "E";
    else if (result == UNKNOWN_VAL) s << "?";
    else s << result;
    s << ")";
    return s.str();
  }
};

// class Tick : public Operation {
// public:
//   Tick() : Operation() {}
//   void run() {
//     increment_time();
//   }
//   string toString() {
//     return "|B|";
//   }
// };

class History {
  vector<Operation> ops;
  bool violation;
public:
  History() {}
  const Operation& operator[](int idx) const { return ops[idx]; }
  unsigned size() const { return ops.size(); }
  bool operator==(const History &h) const { return compare(h,true); }
  bool operator<=(const History &h) const { return compare(h,false); }
  bool compare(const History &h, bool strict) const {
    // NOTE assume operation IDs determine methods
    if (size() != h.size()) return false;
    for (int i=0; i<size(); i++) {
      if (!(*this)[i].equivalent(h[i])) {
        return false;
      }
      for (int j=0; j<size(); j++) {
        if (i == j) continue;
        if ((*this)[i] < (*this)[j] && !(h[i] < h[j])) return false;
        if (strict && h[i] < h[j] && !((*this)[i] < (*this)[j])) return false;
      }
    }
    return true;
  }
};

namespace std {
  template<> struct hash<Operation> {
    size_t operator()(Operation const &op) const noexcept {
      return 0;
    }
  };
  template<> struct hash<History> {
    size_t operator()(History const &h) const noexcept {
      return 0;
    }
  };
}

class Monitor : public ExecutionListener {
protected:
  string name;
  string vstring;
  int violationCount;
public:
  Monitor(string n) : name(n), violationCount(0) { }
  string getName() { return name; }
  string violation() { return vstring; }
  int numViolations() { return violationCount; }
  virtual void onPreExecute() { vstring = ""; };
  virtual void onCall(Operation *op) {}
  virtual void onReturn(Operation *op) {}
};

struct Object {
  void (*initialize)(void);
  void (*add)(int v);
  int (*remove)(void);
};

#include "counting.h"
#include "containers.h"
#include "linearization.h"

class ViolinListener : public ExecutionListener {
  Object object;
  int time;
  bool return_happened;
  stringstream hout;
  violin_show_t show_histories;
  const bool deterministic_monitor;

public:
  vector<Operation*> operations;
  vector<Monitor*> monitors;

public:
  ViolinListener(Object obj, violin_show_t show)
    : object(obj), deterministic_monitor(true), show_histories(show) { }

  void addMonitor(Monitor *m) {
    monitors.push_back(m);
  }

  void onPreExecute() {
    time = 0;
    return_happened = false;

    hout.str("");
    hout.clear();

    object.initialize();

    for (int i = 0; i < operations.size(); i++)
      operations[i]->reset();

    for (int i = 0; i < monitors.size(); i++)
      monitors[i]->onPreExecute();
  }

  void onPostExecute() {
    int violations = 0;

    for (int i=0; i<monitors.size(); i++) {
      monitors[i]->onPostExecute();
      string v = monitors[i]->violation();
      if (v != "") {
        hout << v << " ";
        violations++;
      }
    }

    num_executions++;
    if (violations > 0)
      num_violations++;

    if (show_histories == SHOW_ALL
        || (show_histories == SHOW_VIOLATIONS && violations > 0)
        || (show_histories == SHOW_WINS && violations > 0 && violations != monitors.size())) {
      cout << num_executions << ". " << hout.str() << endl;
    }

    violin_clear_alloc_pool();
  }
  
  void onResume(int t) {
    Operation *op = operations[t];
    if (op->startTime() < OMEGA)
      return;

    if (deterministic_monitor && return_happened) {
      time++;
      return_happened = false;
    }

    op->start(time);
    hout << op->callString() << " ";
    for (int i=0; i<monitors.size(); i++) monitors[i]->onCall(op);
  }

  void onComplete(int t) {
    Operation *op = operations[t];
    op->end(time);
    hout << op->retString() << " ";
    for (int i=0; i<monitors.size(); i++) monitors[i]->onReturn(op);
    return_happened = true;
  }

  void onDelay() {
    hout << "* ";
  }
};

int violin(
    Object obj,
    Object spec_obj,
    int num_adds,
    int num_removes,
    violin_mode_t mode,
    violin_alloc_policy_t allocation_policy,
    violin_order_t container_order,
    int num_barriers, int num_delays,
    violin_show_t show) {

  DelayBoundedEnumerator e(num_delays);
  ViolinListener v(obj,show);
  e.addListener(&v);

  for (int i=0; i<num_adds; i++) {
    Operation *op = new AddOperation(obj.add,i+1);
    v.operations.push_back(op);
    e.addThread(&Operation::run, (void*) op);
  }
  for (int i=0; i<num_removes; i++) {
    Operation *op = new RemoveOperation(obj.remove);
    v.operations.push_back(op);
    e.addThread(&Operation::run, (void*) op);
  }

  // if (!deterministic_monitor) {
  //   for (int i=0; i<num_barriers; i++) {
  //     Operation *op = new Tick();
  //     v.operations.push_back(op);
  //     e.addThread(&Operations::run, (void*) op);
  //   }
  // }

  alloc_policy = allocation_policy;

  if (mode == LINEARIZATIONS_MODE || mode == VERSUS_MODE) {
    vector<Operation*> spec_ops;
    for (int i=0; i<num_adds; i++)
      spec_ops.push_back(new AddOperation(spec_obj.add,i+1));
    for (int i=0; i<num_removes; i++)
      spec_ops.push_back(new RemoveOperation(spec_obj.remove));
    v.monitors.push_back(
      new LinearizationMonitor(spec_obj, v.operations, spec_ops));
  }

  if (mode == COUNTING_MODE || mode == VERSUS_MODE)
    v.monitors.push_back(
      new CollectionCountingMonitor(num_barriers+1, num_adds, container_order));

  cout << "Violin, ";
  switch (mode) {
    case VERSUS_MODE: cout << "linearization vs. counting"; break;
    case COUNTING_MODE: cout << "counting"; break;
    case LINEARIZATIONS_MODE: cout << "linearization"; break;
    default: cout << "unmonitored"; break;
  }
  cout << " mode w/ "
       << num_adds << " adds, "
       << num_removes << " removes, "
       << num_barriers << " barriers, "
       << num_delays << " delays."
       << endl;

  time_t start_time, end_time;
  time(&start_time);
  cout << "Enumerating schedules with "
       << e.getThreads().size() << " threads "
       << "and " << num_delays << " delays..." << endl;
  e.run();
  time(&end_time);
  cout << num_executions << " schedules enumerated in "
       << difftime(end_time,start_time) << "s." << endl;
  for (int i=0; i<v.monitors.size(); i++)
    cout << v.monitors[i]->getName() << " found "
         << v.monitors[i]->numViolations() << " violations." << endl;
  return 0;
}
