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
  struct cell *x = (struct cell*) my_malloc(sizeof *x);
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
  my_free(t);
  return data;
}

int main() {
  add_init_fn(reset);
  violin_init(push,pop);
  set_alloc(FIFO);
  declare_operation(Add,1);
  declare_operation(Add,2);
  declare_operation(Add,3);
  declare_operation(Remove,0);
  declare_operation(Remove,0);
  declare_operation(Remove,0);
  declare_operation(Remove,0);
  violin_run(3);
  return 0;
}
