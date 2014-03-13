#include <stdio.h>
#include <stdlib.h>
#include <smack.h>
#include <violin.h>

struct cell {
    int data;
    struct cell *next;
};

struct cell *s;

void initialize() {
    s = 0;
}

bool cas(struct cell **p, struct cell* t, struct cell *x) {
  if (*p == t) {
    *p = x;
    return true;
  } else return false;
}


void push (int v) {
  VIOLIN_PROC;
  VIOLIN_OP;
  VIOLIN_OP_START(add,v);

  struct cell *t;
  struct cell *x = malloc(sizeof *x);
  x->data = v;
  
  do {
      __SMACK_code("assume {:yield} true;");
      t = s;
      x->next = t;
  } while (!cas(&s,t,x));
    
  VIOLIN_OP_FINISH(add,0);
}

int pop () {
  VIOLIN_PROC;
  VIOLIN_OP;
  VIOLIN_OP_START(remove,0);

  struct cell *t;
  struct cell *x = malloc(sizeof *x);
  do {
      __SMACK_code("assume {:yield} true;");
      t = s;
      if(t == 0) {
          return -1;
      }
      x = t->next;
  } while (!cas(&s,t,x));
  
  VIOLIN_OP_FINISH(remove,t->data);
  return t->data;
}

int main() {
  VIOLIN_PROC;
  VIOLIN_INIT;
  initialize();

  __SMACK_decl("var x: int;");  
  __SMACK_code("call {:async} @(@);", push, 1);
  __SMACK_code("call {:async} @(@);", push, 2);
  __SMACK_code("call {:async} x := @();", pop);
  __SMACK_code("call {:async} x := @();", pop);

  __SMACK_code("assume {:yield} true;");
  VIOLIN_CHECK(stack);
  return 0;
}