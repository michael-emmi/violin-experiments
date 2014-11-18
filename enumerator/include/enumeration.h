/*****************************************************************************/
/** FIBER MAGIC                                                             **/
/*****************************************************************************/

#include <vector>
#include <deque>
#include <list>
#include "Coro.h"

using namespace std;

Coro *scheduler;
Coro *current;
bool completed;

#define Yield DoYield

void DoYield() {
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

class ExecutionListener {
public:
  ExecutionListener() {}
  virtual void onPreExecute() {}
  virtual void onPostExecute() {}
  virtual void onPause(int t) {}
  virtual void onResume(int t) {}
  virtual void onComplete(int t) {}
  virtual void onDelay() {}
};

class Enumerator {
protected:
  vector<Thread> threads;
  list<ExecutionListener*> listeners;
  enum execution_event_t { PRE_EXECUTE, POST_EXECUTE, PAUSE, RESUME, COMPLETE, DELAY };

public:
  Enumerator() {}
  Enumerator(vector<Thread> &ts) : threads(ts) {}

  vector<Thread> &getThreads() {
    return threads;
  }
  void addThread(void (*run)(void*), void *obj) {
    threads.push_back({.coro = Coro_new(), .run = run, .obj = obj});
  }
  void addListener(ExecutionListener *l) {
    listeners.push_back(l);
  }
  void removeListener(ExecutionListener *l) {
    listeners.remove(l);
  }
  virtual void run() = 0;

protected:
  void notify(execution_event_t event, int t=0) {
    for (list<ExecutionListener*>::iterator l = listeners.begin();
        l != listeners.end(); ++l) {
      switch (event) {
      case PRE_EXECUTE:
        (*l)->onPreExecute();
        break;
      case POST_EXECUTE:
        (*l)->onPostExecute();
        break;
      case PAUSE:
        (*l)->onPause(t);
        break;
      case RESUME:
        (*l)->onResume(t);
        break;
      case COMPLETE:
        (*l)->onComplete(t);
        break;
      case DELAY:
        (*l)->onDelay();
        break;
      }
    }
  }

  int search(Scheduler *s) {

    scheduler = Coro_new();
    Coro_initializeMainCoro(scheduler);

    while (s->nextSchedule()) {

      for (vector<Thread>::iterator t = threads.begin(); t != threads.end(); ++t) {
        Coro_startCoro_(scheduler, current = t->coro, &(*t), &Thread::execute);
      }

      notify(PRE_EXECUTE);

      while (true) {
        int current_thread = s->nextStep();

        if (current_thread == Scheduler::DONE)
          break;

        if (current_thread == Scheduler::DELAY) {
          notify(DELAY);
          continue;
        }

        notify(RESUME,current_thread);

        if (Resume(threads[current_thread].coro)) {
          s->completed();
          notify(COMPLETE,current_thread);

        } else {
          notify(PAUSE,current_thread);
        }

      }

      notify(POST_EXECUTE);
    }
    return 0;
  }
};

class DelayBoundedEnumerator : public Enumerator {
  int num_delays;
public:
  DelayBoundedEnumerator(int K) : Enumerator(), num_delays(K) { }
  void run() {
    search(new RoundRobinScheduler(threads, num_delays));
  }
};

class AtomicThreadEnumerator : public Enumerator {
public:
  AtomicThreadEnumerator() : Enumerator() { }
  AtomicThreadEnumerator(vector<Thread> &ts) : Enumerator(ts) { }
  void run() {
    search(new AtomicScheduler(threads));
  }
};
