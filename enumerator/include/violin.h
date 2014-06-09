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

enum violin_mode_t { NOTHING_MODE, COUNTING_MODE, LINEARIZATIONS_MODE  };
violin_mode_t violin_mode;

enum violin_show_t { SHOW_NONE, SHOW_VIOLATIONS, SHOW_ALL };
violin_show_t show_histories;

const int INFINITY = 9999;
const int EMPTY_VAL = -1;
const int UNKNOWN_VAL = -2;
int absolute_time, time_bound;
bool return_happened, violation_happened;
bool deterministic_monitor;
int num_executions;
int num_violations;
stringstream hout;

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
#include "linearization.h"

void (*add_function)(int);
int (*remove_function)(void);

int Add(int op_id, int v) {
  int start_time = current_time();
  if (violin_mode == COUNTING_MODE)
    __count(ADD_OP, v, UNKNOWN_VAL, start_time);

  hout << op_id << ":Add(" << v << ")? ";
  add_function(v);
  hout << op_id << ":Add! ";

  if (violin_mode == COUNTING_MODE)
    __count(ADD_OP, v, 0, start_time, current_time());
  return 0;
}

int Remove(int op_id, int v) {
  int start_time = current_time();
  if (violin_mode == COUNTING_MODE)
    __count(REMOVE_OP, v, UNKNOWN_VAL, start_time);

  hout << op_id << ":Rem? ";
  int r = remove_function();
  hout << op_id << ":Rem" << "!";
  if (r == EMPTY_VAL) hout << "E"; else hout << r;
  hout << " ";

  if (violin_mode == COUNTING_MODE)
    __count(REMOVE_OP, v, r, start_time, current_time());
  return r;
}

int Tick(int op_id, int v) {
  hout << "|B| ";
  increment_time();
  return 0;
}

void violin_pre() {
  absolute_time = 0;
  return_happened = false;
  violation_happened = false;
}

void violin_delay() {
  hout << "* ";
}

void violin_post() {
  num_executions++;

  if (show_histories == SHOW_ALL ||
      (show_histories == SHOW_VIOLATIONS && violation_happened)) {

    cout << num_executions << ". " << hout.str() << endl;

    hout.str("");
    hout.clear();
  }
  
  violin_clear_alloc_pool();
}

int violin(void (*init_fn)(void),
    void (*add_fn)(int), int num_adds,
    int (*rem_fn)(void), int num_removes,
    violin_mode_t mode,
    violin_alloc_policy_t allocation_policy,
    violin_order_t container_order,
    int num_barriers, int num_delays,
    violin_show_t show) {

  deterministic_monitor = true;

  add_function = add_fn;
  for (int i=0; i<num_adds; i++)
    violin_operations.push_back(new Operation(Add,i,0));

  remove_function = rem_fn;
  for (int i=0; i<num_removes; i++)
    violin_operations.push_back(new Operation(Remove,0,0));

  if (!deterministic_monitor)
    for (int i=0; i<num_barriers; i++)
      violin_operations.push_back(new Operation(Tick,0,0));

  violin_mode = mode;
  alloc_policy = allocation_policy;
  violin_order = container_order;
  time_bound = (mode == COUNTING_MODE) ? num_barriers : INFINITY;
  show_histories = show;

  init_container_counters(num_barriers,num_adds);
  register_pre(violin_pre);
  register_pre(init_fn);

  if (violin_mode == COUNTING_MODE) {
    register_pre(__reset_counters);
    register_post(__check_counting_violations);

  } else if (violin_mode == LINEARIZATIONS_MODE) {
    register_post(compute_linearizations);
  }

  register_delay(violin_delay);
  register_post(violin_post);

  for (vector<Operation*>::iterator op = violin_operations.begin();
      op != violin_operations.end(); ++op)
    register_thread(&Operation::run, (void*) *op);

  time_t start_time, end_time;
  time(&start_time);
  cout << "Enumerating schedules with "
       << num_adds + num_removes + num_barriers << " threads "
       << "and " << num_delays << " delays..." << endl;
  search(num_delays);
  time(&end_time);
  cout << num_executions << " schedules enumerated in "
       << difftime(end_time,start_time) << "s." << endl;
  cout << "Found " << num_violations << " violations." << endl;
  return 0;
}
