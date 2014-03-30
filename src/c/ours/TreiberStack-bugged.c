#include <stdio.h>
#include <stdlib.h>
#include <smack.h>
#include <violin.h>
#include <c2s.h>

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
    __SMACK_code("assume {:yield} true;");
    BOOKMARK("X");
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
      BOOKMARK("Y");
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
      BOOKMARK("Y");
      t = s;
      if(t == 0) {
          VIOLIN_OP_FINISH(remove,-1);
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

  // __SMACK_top_decl("axiom {:static_threads} true;");
  __SMACK_decl("var x: int;");
  __SMACK_decl("var t1, t2, t3, t4: int;");
  __SMACK_code("call {:async t1} @(@);", push, 1);
  __SMACK_code("call {:async t2} @(@);", push, 2);
  __SMACK_code("call {:async t3} x := @();", pop);
  __SMACK_code("call {:async t4} x := @();", pop);

  __SMACK_code("assume {:yield} true;");

  BOOKMARK("here");
  ROUND(0,"here",1,2);
  ROUND(t1,"Y",1,0); ROUND(t1,"X",1,0);
  ROUND(t2,"Y",1,0); ROUND(t2,"X",1,0);
  ROUND(t3,"Y",1,0); ROUND(t3,"X",1,1);
  ROUND(t4,"Y",1,0); ROUND(t4,"X",1,0);

  VIOLIN_CHECK(stack);
  return 0;
}