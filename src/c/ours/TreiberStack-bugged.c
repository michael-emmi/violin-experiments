#include <stdio.h>
#include <stdlib.h>
#include <smack.h>

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
    *p = x;
    return true;
  } else return false;
}


void push (int v) {
    struct cell *t;
    struct cell *x = malloc(sizeof *x);
    x->data = v;
    
    do {
        __SMACK_code("assume {:yield} true;");
        t = s;
        x->next = t;
    } while (!cas(&s,t,x));
}

int pop () {
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
    return t->data;
}


int main() {
  __SMACK_top_decl("axiom {:method \"add\", \"push\"} true;");
  __SMACK_top_decl("axiom {:method \"remove\", \"pop\"} true;");

  __SMACK_decl("var x: int;");
  
  initialize();

  __SMACK_code("call {:async} @(@);", push, 1);
  __SMACK_code("call {:async} x := @();", pop);
  __SMACK_code("call {:async} x := @();", pop);

  __SMACK_code("assume {:yield} true;");
  
  // __SMACK_code("assert {:spec \"no_thinair\"} true;");
  // __SMACK_code("assert {:spec \"unique_removes\"} true;");
  // __SMACK_code("assert {:spec \"no_false_empty\"} true;");
  // __SMACK_code("assert {:spec \"bag_spec\"} true;");
  __SMACK_code("assert {:spec \"stack_spec\"} true;");

  return 0;
}