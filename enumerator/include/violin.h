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
violin_order_t violin_order;

enum violin_mode_t { NOTHING_MODE, COUNTING_MODE, LINEARIZATIONS_MODE, VERSUS_MODE };
violin_mode_t violin_mode;

enum violin_show_t { SHOW_NONE, SHOW_WINS, SHOW_VIOLATIONS, SHOW_ALL };
violin_show_t show_histories;

enum violin_op_t { ADD_OP, REMOVE_OP };

const int INFINITY = 9999;
const int EMPTY_VAL = -1;
const int UNKNOWN_VAL = -2;
int absolute_time, time_bound;
bool return_happened;
bool deterministic_monitor;
int num_executions;
int num_violations;
stringstream hout;

class Monitor : public ExecutionListener {
protected:
  string name;
  bool violationFound;
  int violationCount;

public:
  Monitor(string n) : name(n), violationCount(0) { }
  string getName() { return name; }
  bool violation() { return violationFound; }
  int numViolations() { return violationCount; }
  virtual void onPreExecute() {
    violationFound = false;
  };
  virtual void onCall(int op_id, violin_op_t op, int val, int start_time) {};
  virtual void onReturn(
    int op_id, violin_op_t op, int val, int ret_val,
    int start_time, int end_time) {};
};

vector<Monitor*> monitors;

void increment_time() {
  absolute_time++;
}

int current_time() {
  return absolute_time;
}

struct Operation {
  Operation(int(*f)(int,int), int p, int r)
    : proc(f), parameter(p), result(r) {
    id = unique_id++;
  }
  static int unique_id;
  int id;
  int start_time, end_time;
  int (*proc)(int,int);
  int parameter, result;
  static void run(void *context);
};

int Operation::unique_id = 0;

void Operation::run(void *context)  {
  Operation *op = (Operation*) context;
  if (deterministic_monitor && return_happened) {
    increment_time();
    return_happened = false;
  }
  op->start_time = current_time();
  op->end_time = INFINITY;
  op->result = op->proc(op->id,op->parameter);
  op->end_time = current_time();
  return_happened = true;
}

vector<Operation*> violin_operations;

#include "counting.h"
#include "containers.h"

struct Object {
  void (*initialize)(void);
  void (*add)(int v);
  int (*remove)(void);
} violin_object, violin_spec_object;

int Add(int op_id, int v) {
  int start_time = current_time();

  for (vector<Monitor*>::iterator m = monitors.begin(); m != monitors.end(); ++m)
    (*m)->onCall(op_id,ADD_OP,v,start_time);

  hout << op_id << ":Add(" << v << ")? ";
  violin_object.add(v);
  hout << op_id << ":Add! ";

  int end_time = current_time();

  for (vector<Monitor*>::iterator m = monitors.begin(); m != monitors.end(); ++m)
    (*m)->onReturn(op_id,ADD_OP,v,0,start_time,end_time);

  return 0;
}

int Remove(int op_id, int v) {
  int start_time = current_time();
  for (vector<Monitor*>::iterator m = monitors.begin(); m != monitors.end(); ++m)
    (*m)->onCall(op_id,REMOVE_OP,v,start_time);

  hout << op_id << ":Rem? ";
  int r = violin_object.remove();
  hout << op_id << ":Rem" << "!";
  if (r == EMPTY_VAL) hout << "E"; else hout << r;
  hout << " ";

  int end_time = current_time();
  for (vector<Monitor*>::iterator m = monitors.begin(); m != monitors.end(); ++m)
    (*m)->onReturn(op_id,REMOVE_OP,v,r,start_time,end_time);

  return r;
}

int Tick(int op_id, int v) {
  hout << "|B| ";
  increment_time();
  return 0;
}

#include "linearization.h"

class ViolinListener : public ExecutionListener {

public:
  ViolinListener() { }

  void onPreExecute() {
    absolute_time = 0;
    return_happened = false;

    hout.str("");
    hout.clear();

    violin_object.initialize();

    for (vector<Monitor*>::iterator m = monitors.begin(); m != monitors.end(); ++m)
      (*m)->onPreExecute();
  }

  void onPostExecute() {
    int violations = 0;

    for (vector<Monitor*>::iterator m = monitors.begin(); m != monitors.end(); ++m) {
      (*m)->onPostExecute();
      if ((*m)->violation())
        violations++;
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

  violin_object = obj;
  violin_spec_object = spec_obj;
  deterministic_monitor = true;

  for (int i=0; i<num_adds; i++)
    violin_operations.push_back(new Operation(Add,i+1,0));

  for (int i=0; i<num_removes; i++)
    violin_operations.push_back(new Operation(Remove,0,0));

  if (!deterministic_monitor)
    for (int i=0; i<num_barriers; i++)
      violin_operations.push_back(new Operation(Tick,0,0));

  violin_mode = mode;
  alloc_policy = allocation_policy;
  violin_order = container_order;
  time_bound = (mode == COUNTING_MODE || mode == VERSUS_MODE) ? num_barriers : INFINITY;
  show_histories = show;

  DelayBoundedEnumerator e(num_delays);

  for (vector<Operation*>::iterator op = violin_operations.begin();
      op != violin_operations.end(); ++op)
    e.addThread(&Operation::run, (void*) *op);
  e.addListener(new ViolinListener());

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

  if (violin_mode == LINEARIZATIONS_MODE || violin_mode == VERSUS_MODE)
    monitors.push_back(new LinearizationMonitor(num_adds, num_removes));

  if (violin_mode == COUNTING_MODE || violin_mode == VERSUS_MODE)
    monitors.push_back(new CollectionCountingMonitor(num_barriers, num_adds));

  time_t start_time, end_time;
  time(&start_time);
  cout << "Enumerating schedules with "
       << num_adds + num_removes + num_barriers << " threads "
       << "and " << num_delays << " delays..." << endl;
  e.run();
  time(&end_time);
  cout << num_executions << " schedules enumerated in "
       << difftime(end_time,start_time) << "s." << endl;
  for (vector<Monitor*>::iterator m = monitors.begin(); m != monitors.end(); ++m)
    cout << (*m)->getName() << " found "
         << (*m)->numViolations() << " violations." << endl;
  return 0;
}
