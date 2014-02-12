#include<stdio.h>
#include<stdlib.h>

typedef int bool;
#define true 1
#define false 0

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
    struct cell *t;
    struct cell *x = malloc(sizeof *x);
    x->data = v;
    do {
        t = s;
        x->next = t;
    } while (!cas(&s,t,x));
}

int pop () {
    struct cell *t;
    struct cell *x = malloc(sizeof *x);
    do {
        t = s;
        if(t == 0) {
            return -1;
        }
        x = t->next;
    } while (!cas(&s,t,x));
    return t->data;
}


int main() {
    push(3);
    push(4);
    push(5);
    push(6);
    int i;
    for (i = 0; i < 5; i++) {
        printf("popped: %i\n",pop());
    }
}