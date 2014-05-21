#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "violin.h"

bool cas(void **p, void* t, void *x) {
  if (*p == t) {
    *p = x;
    return true;
  } else return false;
}

// #define CAS(t,x,y,z) __atomic_compare_exchange_n(x,&(y),z,true,0,0)
#define CAS(t,x,y,z) cas((void**) x, (void*) y, (void*) z)

#define LOCATION_ARRAY_SIZE 14
#define COLLISION_ARRAY_SIZE 4

enum stack_op { POP = 0, PUSH };

struct Cell {
	struct Cell *pnext;
	int          pdata;
};

struct ThreadInfo {
	unsigned int id;
  char         op;
  struct Cell  cell;
  int          spin;
};

struct Simple_Stack {
	struct Cell *ptop;
};

struct Cell EMPTY = { .pnext = NULL, .pdata = -1 };
struct Simple_Stack S = { .ptop = NULL };
struct ThreadInfo *location[LOCATION_ARRAY_SIZE];
int collision[COLLISION_ARRAY_SIZE];

int unique_id = 0;

void StackOp(struct ThreadInfo *p);
void LesOP(struct ThreadInfo *p);
bool TryPerformStackOp(struct ThreadInfo *p);
void FinishCollision(struct ThreadInfo *p);
bool TryCollision(struct ThreadInfo *p, struct ThreadInfo *q, int him);

int GetPosition(struct ThreadInfo *p) {
  static int pos = 0;
  return pos;
}
void delay(int d) {
  
}

void StackOp(struct ThreadInfo *p) {
	if (TryPerformStackOp(p) == false)
		LesOP(p);
	return;
}

void LesOP(struct ThreadInfo *p) {
  int mypid = p->id;

  while (1) {
    Yield();
		location[mypid] = p;
    int pos = GetPosition(p); // CONSTANTIN: pos = random() % MAX_THREADS

    Yield();

		int him = collision[pos];

    Yield();

		while (!CAS(int,&collision[pos],him,mypid))
			him = collision[pos];

    Yield();

		if (him > 0) {
      
			struct ThreadInfo* q = location[him];

      Yield();

			if (q != NULL && q->id == him && q->op != p->op) {

        Yield();

				if (CAS(ti,&location[mypid],p,NULL)) {

          Yield();

					if (TryCollision(p,q,him) == true) {
						return;

					} else {
						goto stack;
          }
				}
				else {
          Yield();
					FinishCollision(p);
					return;
				}
			}
		}

		delay(p->spin); // CONSTANTIN: sleep(p->spin)

    Yield();

		if (!CAS(ti,&location[mypid],p,NULL)) {
      Yield();
			FinishCollision(p);
			return;
		}
  // } // The wrong place!
stack:
		if (TryPerformStackOp(p) == true)
			return;
  } // The right place
}

bool TryPerformStackOp(struct ThreadInfo* p) {
	struct Cell *phead, *pnext;
  
	if (p->op==PUSH) {
		phead = S.ptop;
		p->cell.pnext = phead;

    Yield();

		if(CAS(c,&S.ptop,phead,&p->cell)) {
      Yield();
			return true;
		} else {
      Yield();
			return false;
    }
	}
	if (p->op==POP) {

		phead = S.ptop;
		if (phead == NULL) {
      Yield();
      p->cell = EMPTY;
			return true;
		}
		pnext = phead->pnext;

    Yield();

		if (CAS(c,&S.ptop,phead,pnext)) {
      Yield();
      p->cell = *phead;
			return true;
		}
		else {
      Yield();
      p->cell = EMPTY;
			return false;
		}
	}
	return false;
}

void FinishCollision(struct ThreadInfo *p) {
  int mypid = p->id;
  
	if (p->op == POP) {
    Yield();
    p->cell = location[mypid]->cell;
		location[mypid] = NULL;
	}
}

bool TryCollision(struct ThreadInfo *p, struct ThreadInfo *q, int him) {
  int mypid = p->id;
	if (p->op == PUSH) {

    Yield();

		if (CAS(ti,&location[him],q,p)) {
      Yield();
			return true;
		} else
			return false;
	}
	if (p->op == POP) {

    Yield();

		if (CAS(ti,&location[him],q,NULL)) {
      Yield();
      p->cell = q->cell;
			location[mypid] = NULL;
			return true;
		}
		else {
			return false;
    }
	}
	return false;
}

void Reset() {
  EMPTY.pnext = NULL; EMPTY.pdata = -1;
  S.ptop = NULL;
  for (int i=0; i<LOCATION_ARRAY_SIZE; i++) location[i] = 0;
  for (int i=0; i<COLLISION_ARRAY_SIZE; i++) collision[i] = 0;
  unique_id = 0;
}

void Push(int x) {
	struct ThreadInfo *ti = (struct ThreadInfo*) my_malloc(sizeof(struct ThreadInfo));
  ti->id = ++unique_id;
	ti->op = PUSH;
  ti->cell.pdata = x;
  ti->spin = 1;
  StackOp(ti);
}

int Pop() {
	struct ThreadInfo *ti = (struct ThreadInfo*) my_malloc(sizeof(struct ThreadInfo));
  ti->id = ++unique_id;
  ti->op = POP;
  ti->spin = 1;
  StackOp(ti);
  return ti->cell.pdata;
}

int main() {
  add_init_fn(Reset);
  violin_init(Push,Pop);
  set_alloc(FIFO);
  declare_operation(Add,1);
  declare_operation(Add,2);
  declare_operation(Add,3);
  declare_operation(Add,4);
  declare_operation(Remove,0);
  declare_operation(Remove,0);
  violin_run(7);
  return 0;
}
