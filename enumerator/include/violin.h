#include <iostream>
#include <vector>
#include <stack>
#include <deque>
#include <map>
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
};
int Operation::unique_id = 0;
vector<Operation*> operations;

void Run(void *context) {
  Operation *op = (Operation*) context;
  Yield();
  op->result = op->proc(op->id,op->parameter);
  Complete();
}

/*****************************************************************************/
/** SCHEDULE EXPLORATION                                                    **/
/*****************************************************************************/

vector<void(*)()> inits;
deque<Operation*> sched;
vector<int> delays;

void scheduler_init() {
  scheduler = Coro_new();
	Coro_initializeMainCoro(scheduler);
}

void add_init_fn(void (*fn)()) {
  inits.push_back(fn);
}

void declare_operation(int (*f)(int,int), int arg) {
  operations.push_back(new Operation(f,arg,0));
}

void init_run() {  
  for (vector<Operation*>::iterator op = operations.begin(); op != operations.end(); ++op) {
    sched.push_back(*op);
    Coro_startCoro_(scheduler, current = (*op)->coroutine, (void*) *op, Run);
  }
  for (vector<void(*)()>::iterator i = inits.begin(); i != inits.end(); ++i)
    (*i)();
}

int search(int num_delays) {
  for (int i=0; i<num_delays; i++)
    delays.push_back(i);

  int num_schedules = 0;
  cout << "Enumerating schedules with " << operations.size() << " operations "
       << "and " << delays.size() << " delays..." << endl;
  
  while (true) {
    init_run();
    int step;
    int d = 0;

    cout << ++num_schedules << ". ";

    for (step=0; !sched.empty(); step++) {
  
      if (d < delays.size() && step == delays[d]) {
        if (sched.size() == 1) {
          for (int i=d; i<delays.size(); i++)
            delays[i] = 9999;
        } else {
          // cout << "* ";
          sched.push_back(sched.front());
          sched.pop_front();
        }
        d++;
        continue;
      }
      // cout << sched.front()->id << " ";
      if (Resume(sched.front()->coroutine))
        sched.pop_front();
    }
    cout << endl;
    
    int i;
    for (i=d-1; i>=-1; i--) {
      if (i >= 0 && delays[i] < step-2) {
        delays[i]++;
        for (int j=i+1; j<d; j++)
          delays[j] = delays[j-1] + 1;
        break;
      }
    }
    if (i < 0)
      break;
  }

  cout << num_schedules << " schedules enumerated." << endl;
  return 0;
}

/*****************************************************************************/
/** MEMORY ALLOCATION (TO CATCH ABA)                                        **/
/*****************************************************************************/

enum { DEFAULT, FIFO, LIFO };
const int ALLOC = FIFO;

deque<void*> free_pool;
void* my_malloc(int size) {
  void *x;
  if (free_pool.empty())
    x = malloc(size);
  else {
    x = free_pool.front();
    free_pool.pop_front();
  }
  return x;
}
void my_free(void *x) {
  if (ALLOC == FIFO)
    free_pool.push_back(x);
  else if (ALLOC == LIFO)
    free_pool.push_front(x);
  else
    free(x);
}

/*****************************************************************************/
/** COUNTING                                                                **/
/*****************************************************************************/

int violin_time; // TODO add barriers
map<int,int> add_open;
map<int,int> add_done;
int rem_open;
map<int,int> rem_done;
int num_violations;

void (*add_function)(int);
int (*remove_function)(void);

void reset_counters() {
  add_open.clear();
  add_done.clear();
  rem_open = 0;
  rem_done.clear();
}

void violin_init(void (*add_fn)(int), int (*rem_fn)(void)) {
  add_function = add_fn;
  remove_function = rem_fn;
  scheduler_init();
  add_init_fn(reset_counters);
}

int violin_run() {
  search(3);
  cout << "Found " << num_violations << " violations." << endl;
  return 0;
}

int Add(int id, int v) {
  add_open[v]++;
  cout << id << ":Add(" << v << ")? ";

  add_function(v);

  add_open[v]--;
  add_done[v]++;
  cout << id << ":Add! ";
  return 0;
}

int Remove(int id, int v) {
  rem_open++;
  cout << id << ":Rem? ";

  int r = remove_function();

  rem_open--;
  cout << id << ":Rem" << "!" << r << " ";
  if (r >= 0) rem_done[r]++;
  if (rem_done[r] > add_done[r] + add_open[r]) {
    cout << "(V) ";
    num_violations++;
  }
  return r;
}
