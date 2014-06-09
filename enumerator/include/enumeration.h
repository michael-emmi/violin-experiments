/*****************************************************************************/
/** FIBER MAGIC                                                             **/
/*****************************************************************************/

#include <vector>
#include <deque>
#include "Coro.h"

using namespace std;

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

#include "Scheduler.h"

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
  scheduler = Coro_new();
  Coro_initializeMainCoro(scheduler);

  RoundRobinScheduler s(threads, num_delays);

  while (s.nextSchedule()) {
    for (vector<Thread>::iterator t = threads.begin(); t != threads.end(); ++t) {
      Coro_startCoro_(scheduler, current = t->coro, &(*t), &Thread::execute);
    }

    notify(pre_listeners);

    while (true) {
      int current_thread = s.nextStep();

      if (current_thread == Scheduler::DONE)
        break;

      else if (current_thread == Scheduler::DELAY)
        notify(delay_listeners);

      else if (Resume(threads[current_thread].coro))
        s.completed();
    }

    notify(post_listeners);
  }
  return 0;
}
