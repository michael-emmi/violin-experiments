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
#include <stack>
#include <deque>
#include <queue>
#include <set>
#include <map>
#include <time.h>
using namespace std;

/*****************************************************************************/
/** FIBER MAGIC                                                             **/
/*****************************************************************************/

#include "Coro.h"

Coro *scheduler;
Coro *current;
bool completed;

void Yield() {
  Coro_switchTo_(current, scheduler);
}

void Complete() {
  completed = true;
  Coro_switchTo_(current, scheduler);
}

bool Resume(Coro *c) {
  completed = false;
  current = c;
  Coro_switchTo_(scheduler, current);
  return completed;
}

/*****************************************************************************/
/** SCHEDULE EXPLORATION                                                    **/
/*****************************************************************************/

vector<void(*)()> pre_listeners, delay_listeners, post_listeners;
deque<Coro*> schedule;

void register_thread(Coro *c, void (*run)(void*), void *arg) {
  schedule.push_back(c);
  Coro_startCoro_(scheduler, current = c, arg, run);
}

void notify(vector<void(*)()> listeners) {
  for (vector<void(*)()>::iterator i = listeners.begin();
       i != listeners.end(); ++i)
    (*i)();
}

void register_pre(void (*fn)()) {
  pre_listeners.push_back(fn);
}

void register_delay(void (*fn)()) {
  delay_listeners.push_back(fn);
}

void register_post(void (*fn)()) {
  post_listeners.push_back(fn);
}

int search(int num_delays) {
  int delays[num_delays];

  scheduler = Coro_new();
  Coro_initializeMainCoro(scheduler);

  for (int i=0; i<num_delays; i++)
    delays[i] = i;

  while (true) {
    int d = 0;

    notify(pre_listeners);

    for (int step=0; !schedule.empty(); step++) {
      if (schedule.size() > 1 && d < num_delays && delays[d] == step) {
        notify(delay_listeners);
        schedule.push_back(schedule.front());
        schedule.pop_front();
        d++;
      } else if (Resume(schedule.front())) {
        schedule.pop_front();
      }
    }

    notify(post_listeners);

    if (d == 0) break;
    
    delays[d-1]++;
    for (int i=d; i<num_delays; i++) {
      delays[i] = delays[i-1] + 1;
    }
  }
  return 0;
}

/*****************************************************************************/
/** MEMORY ALLOCATION (TO CATCH ABA)                                        **/
/*****************************************************************************/

enum violin_alloc_policy_t { DEFAULT_ALLOC, LRF_ALLOC, MRF_ALLOC };
violin_alloc_policy_t alloc_policy;

deque<void*> free_pool;
set<void*> alloc_set;

void* violin_malloc(int size) {
  void *x;
  if (free_pool.empty()) {
    x = malloc(size);
    alloc_set.insert(x);
  } else {
    x = free_pool.front();
    free_pool.pop_front();
  }
  return x;
}

void violin_free(void *x) {
  if (alloc_set.find(x) == alloc_set.end())
    return;

  switch (alloc_policy) {
  case LRF_ALLOC:
    free_pool.push_back(x);
    break;
  case MRF_ALLOC:
    free_pool.push_front(x);
    break;
  default:
    free(x);
    alloc_set.erase(x);
  }
}

void violin_clear_alloc_pool() {
  for (set<void*>::iterator p = alloc_set.begin(); p != alloc_set.end(); ) {
    // Tricky tricky! can't free(*p) before ++p
    void *ptr = *p;
    ++p;
    free(ptr);
  }
  alloc_set.clear();
  free_pool.clear();
}

/*****************************************************************************/
/** OPERATIONS                                                              **/
/*****************************************************************************/

struct Operation {
  Operation(int(*f)(int,int), int p, int r)
    : proc(f), parameter(p), result(r) {
    id = unique_id++;
    coroutine = Coro_new();
  }
  static int unique_id;
  int id;
  int (*proc)(int,int);
  int parameter;
  int result;
  Coro *coroutine;
  static void run(void *context);
};
int Operation::unique_id = 0;
void Operation::run(void *context)  {
  Operation *op = (Operation*) context;
  Yield();
  op->result = op->proc(op->id,op->parameter);
  Complete();
}

/*****************************************************************************/
/** COUNTING                                                                **/
/*****************************************************************************/

vector<Operation*> violin_operations;
stringstream hout;
bool deterministic_monitor;
bool return_happened, violation_happened;
int absolute_time, relative_time, time_bound;

void increment_time() {
  absolute_time++;

  if (relative_time < time_bound) {
    relative_time++;
  } else {
    // shift everything...
  }
}

int current_time() {
  return relative_time;
}

const int INFINITY = 9999;
const int EMPTY_VAL = -1;
const int UNKNOWN_VAL = -2;
int num_executions;
int num_violations;

void (*add_function)(int);
int (*remove_function)(void);

enum violin_order_t { NO_ORDER, LIFO_ORDER, FIFO_ORDER };
violin_order_t violin_order;

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

struct OP {
  int id;
  int start;
  int finish;
  bool operator< (const OP& o) const {
    return id < o.id;
  }
};
multiset<OP> operations;

void compute_linearizations() {
  queue< pair< vector<OP>, multiset<OP> > > work_list;
  vector< vector<OP> > linearizations;
  vector<OP> empty_seq;

  work_list.push(make_pair(empty_seq,operations));

  while (!work_list.empty()) {
    vector<OP> sequence = work_list.front().first;
    multiset<OP> remaining = work_list.front().second;
    work_list.pop();

    if (remaining.empty()) {
      linearizations.push_back(sequence);
      continue;
    }

    int min = 9999;
    for (multiset<OP>::iterator o = remaining.begin(); o != remaining.end(); ++o)
      if (o->start < min)
        min = o->start;

    multiset<OP> minimals;
    for (multiset<OP>::iterator o = remaining.begin(); o != remaining.end(); ++o) {
      if (o->start == min) {
        minimals.insert(*o);
      }
    }

    for (multiset<OP>::iterator o = minimals.begin(); o != minimals.end(); ++o) {
      vector<OP> sequence2(sequence);
      multiset<OP> remaining2(remaining);
      sequence2.push_back(*o);
      remaining2.erase(*o);
      work_list.push(make_pair(sequence2,remaining2));
    }
  }

  hout << "(" << linearizations.size() << "Ls) ";

  // cout << endl;
  // for (vector< vector<OP> >::iterator lin = linearizations.begin(); lin != linearizations.end(); ++lin) {
  //   for (vector<OP>::iterator o = lin->begin(); o != lin->end(); ++o)
  //     cout << o->id << "(" << o->start << "," << o->finish << ")";
  //   cout << endl;
  // }
}


enum violin_mode_t { NOTHING_MODE, COUNTING_MODE, LINEARIZATIONS_MODE  };
violin_mode_t violin_mode;

enum violin_show_t { SHOW_NONE, SHOW_VIOLATIONS, SHOW_ALL };
violin_show_t show_histories;

int Add(int op_id, int v) {

  if (deterministic_monitor && return_happened) {
    increment_time();
    return_happened = false;
  }

  int start_time = current_time();

  switch (violin_mode) {
  case COUNTING_MODE:
    added[v][make_pair(start_time,INFINITY)]++;
    break;
  case LINEARIZATIONS_MODE:
    operations.insert({.id = op_id, .start = start_time, .finish = INFINITY});
    break;
  case NOTHING_MODE:
    break;
  }

  hout << op_id << ":Add(" << v << ")? ";

  add_function(v);

  hout << op_id << ":Add! ";

  int end_time = current_time();

  switch (violin_mode) {
  case COUNTING_MODE:
    added[v][make_pair(start_time,INFINITY)]--;
    added[v][make_pair(start_time,end_time)]++;
    break;
  case LINEARIZATIONS_MODE:
    operations.erase({.id = op_id, .start = start_time, .finish = INFINITY});
    operations.insert({.id = op_id, .start = start_time, .finish = end_time});
    break;
  case NOTHING_MODE:
    break;
  }

  return_happened = true;

  return 0;
}

int Remove(int op_id, int v) {

  if (deterministic_monitor && return_happened) {
    increment_time();
    return_happened = false;
  }

  int start_time = current_time();

  switch (violin_mode) {
  case COUNTING_MODE:
    removed[UNKNOWN_VAL][make_pair(start_time,INFINITY)]++;
    break;
  case LINEARIZATIONS_MODE:
    operations.insert({.id = op_id, .start = start_time, .finish = INFINITY});
    break;
  case NOTHING_MODE:
    break;
  }

  hout << op_id << ":Rem? ";

  int r = remove_function();

  hout << op_id << ":Rem" << "!";
  if (r == EMPTY_VAL) hout << "E"; else hout << r;
  hout << " ";

  int end_time = current_time();

  switch (violin_mode) {
  case COUNTING_MODE:
    removed[UNKNOWN_VAL][make_pair(start_time,INFINITY)]--;
    removed[r][make_pair(start_time,end_time)]++;
    if (remove_violation(r)) {
      hout << "(Rv) ";
      num_violations++;
      violation_happened = true;
    }
    if (start_time >= 3 && order_violation(true,r)) {
      hout << "(Ov)";
      num_violations++;
      violation_happened = true;
    }
    break;
  case LINEARIZATIONS_MODE:
    operations.erase({.id = op_id, .start = start_time, .finish = INFINITY});
    operations.insert({.id = op_id, .start = start_time, .finish = end_time});
    break;
  case NOTHING_MODE:
    break;
  }

  return_happened = true;

  return r;
}

int Tick(int op_id, int v) {
  hout << "|B| ";
  increment_time();
  return 0;
}

void violin_add_threads() {
  for (vector<Operation*>::iterator op = violin_operations.begin(); op != violin_operations.end(); ++op)
    register_thread((*op)->coroutine, (*op)->run, (void*) *op);
}

void violin_pre() {
  absolute_time = 0;
  relative_time = 0;
  return_happened = false;
  violation_happened = false;

  switch (violin_mode) {
  case COUNTING_MODE:
    added.clear();
    removed.clear();
    break;
  case LINEARIZATIONS_MODE:
    operations.clear();
    break;
  case NOTHING_MODE:
    break;
  }
}

void violin_delay() {
  hout << "* ";
}

void violin_post() {
  num_executions++;

  switch (violin_mode) {
  case COUNTING_MODE:
    check_for_violations();
    break;
  case LINEARIZATIONS_MODE:
    compute_linearizations();
    break;
  case NOTHING_MODE:
    break;
  }

  if (show_histories == SHOW_ALL ||
      (show_histories == SHOW_VIOLATIONS && violation_happened)) {

    cout << num_executions << ". " << hout.str() << endl;

    hout.str("");
    hout.clear();
  }
}

int violin(void (*init_fn)(void),
    void (*add_fn)(int), int num_adds,
    int (*rem_fn)(void), int num_removes,
    violin_mode_t mode,
    violin_alloc_policy_t allocation_policy,
    violin_order_t container_order,
    int num_barriers, int num_delays,
    violin_show_t show) {

  register_pre(violin_clear_alloc_pool);
  register_pre(violin_add_threads);
  register_pre(violin_pre);
  register_pre(init_fn);
  register_delay(violin_delay);
  register_post(violin_post);

  deterministic_monitor = true;
  show_histories = show;
  time_bound = (mode == COUNTING_MODE) ? num_barriers : INFINITY;

  add_function = add_fn;
  for (int i=0; i<num_adds; i++)
    violin_operations.push_back(new Operation(Add,i+1,0));

  remove_function = rem_fn;
  for (int i=0; i<num_removes; i++)
    violin_operations.push_back(new Operation(Remove,0,0));

  violin_mode = mode;
  alloc_policy = allocation_policy;
  violin_order = container_order;

  for (int i=0; i<num_barriers; i++)
    violin_operations.push_back(new Operation(Tick,0,0));

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
