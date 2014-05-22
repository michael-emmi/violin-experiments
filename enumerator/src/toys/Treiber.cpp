#include "violin.h"

struct cell {
  int data;
  struct cell *next;
};

struct cell *s;

void reset() {
  s = 0;
}

bool cas(struct cell **p, struct cell* t, struct cell *x) {
  if (*p == t) {
    *p = x;
    return true;
  } else return false;
}

void push (int v) {
  struct cell *t;
  struct cell *x = (struct cell*) violin_malloc(sizeof *x);
  x->data = v;

  do {
    t = s;
    x->next = t;
    Yield();
  } while (!cas(&s,t,x));
}

int pop () {
  struct cell *t;
  struct cell *x;
  do {
    t = s;
    if(t == 0) {
      return -1;
    }
    x = t->next;
    Yield();
  } while (!cas(&s,t,x));
  int data = t->data;
  violin_free(t);
  return data;
}

int main() {
  violin(reset,push,3,pop,4,LRF_ALLOC,LIFO_ORDER,0,3);
  return 0;
}
