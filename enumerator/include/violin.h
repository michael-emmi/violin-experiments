#include <iostream>
#include <vector>
#include <stack>
#include <deque>
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

vector<void(*)()> pre, post;
deque<Coro*> schedule;

void register_thread(Coro *c, void (*run)(void*), void *arg) {
  schedule.push_back(c);
  Coro_startCoro_(scheduler, current = c, arg, run);
}

void register_pre(void (*fn)()) {
  pre.push_back(fn);
}

void register_post(void (*fn)()) {
  post.push_back(fn);
}

int search(int num_delays) {
  int num_schedules = 0;
  int delays[num_delays];
  time_t start_time, end_time;

  scheduler = Coro_new();
	Coro_initializeMainCoro(scheduler);

  for (int i=0; i<num_delays; i++)
    delays[i] = i;

  time(&start_time);
  while (true) {
    int d = 0;

    for (vector<void(*)()>::iterator i = pre.begin(); i != pre.end(); ++i)
      (*i)();

    if (num_schedules == 0)
      cout << "Enumerating schedules with " << schedule.size() << " threads "
           << "and " << num_delays << " delays..." << endl;

    cout << ++num_schedules << ". ";
    for (int step=0; !schedule.empty(); step++) {
      if (schedule.size() > 1 && d < num_delays && delays[d] == step) {
        cout << "* ";
        schedule.push_back(schedule.front());
        schedule.pop_front();
        d++;
      } else if (Resume(schedule.front())) {
        schedule.pop_front();
      }
    }

    for (vector<void(*)()>::iterator i = post.begin(); i != post.end(); ++i)
      (*i)();

    cout << endl;

    if (d == 0) break;
    
    delays[d-1]++;
    for (int i=d; i<num_delays; i++) {
      delays[i] = delays[i-1] + 1;
    }
  }
  time(&end_time);

  cout << num_schedules << " schedules enumerated in "
       << difftime(end_time,start_time) << "s." << endl;
  return 0;
}

/*****************************************************************************/
/** MEMORY ALLOCATION (TO CATCH ABA)                                        **/
/*****************************************************************************/

enum { DEFAULT, FIFO, LIFO };
int alloc_policy;

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

  if (alloc_policy == FIFO)
    free_pool.push_back(x);
  else if (alloc_policy == LIFO)
    free_pool.push_front(x);
  else {
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
int violin_time;
const int INFINITY = 9999;
const int EMPTY_VAL = -1;
const int UNKNOWN_VAL = -2;
int num_violations;

void (*add_function)(int);
int (*remove_function)(void);

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

void violin_reset_counters() {
  violin_time = 0;
  added.clear();
  removed.clear();
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

bool order_violation(bool stack_order, int u, int v) {
  return u != v && is_removed(u) && is_removed(v)
      && add_span(stack_order?u:v).second < add_span(stack_order?v:u).first
      && remove_span(u).second < remove_span(v).first;
}

bool order_violation(bool stack_order, int v) {
  if (v == UNKNOWN_VAL || v == EMPTY_VAL || !is_removed(v)) return false;
  for (map<int,counter>::iterator i = added.begin(); i != added.end(); ++i) {
    if (order_violation(stack_order,i->first,v))
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
  if (remove_empty_violation()) {
    cout << "(Ev)";
    num_violations++;
  }
}

int Add(int op_id, int v) {
  int start_time = violin_time;
  added[v][make_pair(start_time,INFINITY)]++;
  cout << op_id << ":Add(" << v << ")? ";

  add_function(v);

  added[v][make_pair(start_time,INFINITY)]--;
  added[v][make_pair(start_time,violin_time)]++;
  cout << op_id << ":Add! ";
  return 0;
}

int Remove(int op_id, int v) {
  int start_time = violin_time;
  removed[UNKNOWN_VAL][make_pair(start_time,INFINITY)]++;
  cout << op_id << ":Rem? ";

  int r = remove_function();

  removed[UNKNOWN_VAL][make_pair(start_time,INFINITY)]--;
  removed[r][make_pair(start_time,violin_time)]++;
  cout << op_id << ":Rem" << "!" << r << " ";

  if (remove_violation(r)) {
    cout << "(Rv) ";
    num_violations++;
  }

  if (order_violation(true,r)) {
    cout << "(Ov)";
    num_violations++;
  }

  return r;
}

int Tick(int op_id, int v) {
  cout << "|B| ";
  violin_time++;
  return 0;
}

void violin_add_threads() {
  for (vector<Operation*>::iterator op = violin_operations.begin(); op != violin_operations.end(); ++op)
    register_thread((*op)->coroutine, (*op)->run, (void*) *op);
}

int violin(void (*init_fn)(void),
    void (*add_fn)(int), int num_adds,
    int (*rem_fn)(void), int num_removes,
    int policy,
    int num_barriers, int num_delays) {

  register_pre(violin_reset_counters);
  register_pre(violin_clear_alloc_pool);
  register_pre(violin_add_threads);
  register_pre(init_fn);
  register_post(check_for_violations);

  add_function = add_fn;
  for (int i=0; i<num_adds; i++)
    violin_operations.push_back(new Operation(Add,i+1,0));

  remove_function = rem_fn;
  for (int i=0; i<num_removes; i++)
    violin_operations.push_back(new Operation(Remove,0,0));

  alloc_policy = policy;

  for (int i=0; i<num_barriers; i++)
    violin_operations.push_back(new Operation(Tick,0,0));

  search(num_delays);
  cout << "Found " << num_violations << " violations." << endl;
  return 0;
}
