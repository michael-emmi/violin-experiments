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

struct Thread {
  Coro *coro;
  void (*run)(void*);
  void *obj;
  static void execute(void*);
};

void Thread::execute(void* context) {
  Thread *t = (Thread*) context;
  Yield();
  t->run(t->obj);
  Complete();
}

/*****************************************************************************/
/** SCHEDULE EXPLORATION                                                    **/
/*****************************************************************************/

vector<Thread> threads;
vector<void(*)()> pre_listeners, delay_listeners, post_listeners;

void register_thread(void (*run)(void*), void *obj) {
  threads.push_back({.coro = Coro_new(), .run = run, .obj = obj});
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
void notify(vector<void(*)()> listeners) {
  for (vector<void(*)()>::iterator i = listeners.begin();
       i != listeners.end(); ++i)
    (*i)();
}

int search(int num_delays) {
  deque<Coro*> schedule;
  int delays[num_delays];

  scheduler = Coro_new();
  Coro_initializeMainCoro(scheduler);

  for (int i=0; i<num_delays; i++)
    delays[i] = i;

  while (true) {
    int d = 0;

    for (vector<Thread>::iterator t = threads.begin(); t != threads.end(); ++t) {
      schedule.push_back(t->coro);
      Coro_startCoro_(scheduler, current = t->coro, &(*t), &Thread::execute);
    }

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
