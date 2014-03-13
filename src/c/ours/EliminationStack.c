//#include <boost/interprocess/detail/atomic.hpp>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <smack.h>

#define CAS(x,y,z) __atomic_compare_exchange_n(x,&(y),z,true,0,0)

#define N 100

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
struct ThreadInfo *location[N];
int collision[N];

int unique_id = 0;

void StackOp(struct ThreadInfo *p);
void LesOP(struct ThreadInfo *p);
bool TryPerformStackOp(struct ThreadInfo *p);
void FinishCollision(struct ThreadInfo *p);
bool TryCollision(struct ThreadInfo *p, struct ThreadInfo *q, int him);

int GetPosition(struct ThreadInfo *p) {
  int pos;
  pos = __SMACK_nondet();
  __SMACK_assume(0 <= pos && pos < N);
  return pos;
}
void delay(int d);

void StackOp(struct ThreadInfo *p) {
	if (TryPerformStackOp(p) == false)
		LesOP(p);
	return;
}

void LesOP(struct ThreadInfo *p) {
  int mypid = p->id;

	while (1) {
		location[mypid] = p;
    int pos = GetPosition(p); // CONSTANTIN: pos = random() % MAX_THREADS
		int him = collision[pos];

    __SMACK_code("assume {:yield} true;");
		while (!CAS(&collision[pos],him,mypid))
			him = collision[pos];

		if (location[him]->cell.pdata != EMPTY.pdata) {
			struct ThreadInfo* q = location[him];

			if(q != NULL && q->id == him && q->op != p->op) {
        __SMACK_code("assume {:yield} true;");
				if (CAS(&location[mypid],p,NULL)) {
					if (TryCollision(p,q,him) == true)
						return;
					else
						goto stack;
				}
				else {
					FinishCollision(p);
					return;
				}
			}
		}
		delay(p->spin); // CONSTANTIN: sleep(p->spin)

    __SMACK_code("assume {:yield} true;");
		if (!CAS(&location[mypid],p,NULL)) {
			FinishCollision(p);
			return;
		}

stack:
		if (TryPerformStackOp(p) == true)
			return;
	}
}

bool TryPerformStackOp(struct ThreadInfo* p) {
	struct Cell *phead, *pnext;
  
	if (p->op==PUSH) {
		phead = S.ptop;
		p->cell.pnext = phead;
    __SMACK_code("assume {:yield} true;");
		if(CAS(&S.ptop,phead,&p->cell))
			return true;
		else
			return false;
	}
	if (p->op==POP) {
		phead = S.ptop;
		if (phead == NULL) {
      p->cell = EMPTY;
			return true;
		}
		pnext = phead->pnext;

    __SMACK_code("assume {:yield} true;");
		if (CAS(&S.ptop,phead,pnext)) {
			p->cell = *phead;
			return true;
		}
		else {
      p->cell = EMPTY;
			return false;
		}
	}
	return false;
}

void FinishCollision(struct ThreadInfo *p) {
  int mypid = p->id;
  
	if (p->op == POP) {
    p->cell = location[mypid]->cell;
		location[mypid] = NULL;
	}
}

bool TryCollision(struct ThreadInfo *p, struct ThreadInfo *q, int him) {
  int mypid = p->id;

	if (p->op == PUSH) {
    __SMACK_code("assume {:yield} true;");
		if (CAS(&location[him],q,p))
			return true;
		else
			return false;
	}
	if(p->op == POP) {
    __SMACK_code("assume {:yield} true;");
		if(CAS(&location[him],q,NULL)){
			p->cell=q->cell;
			location[mypid]=NULL;
			return true;
		}
		else
			return false;
	}
	return false;
}

void Init() {
  // TODO the collision array should be initial clear
}

void Push(int x) {
	struct ThreadInfo *ti = malloc(sizeof(struct ThreadInfo));
  ti->id = unique_id++;
	ti->op = PUSH;
  ti->cell.pdata = x;
  ti->spin = 1;
	StackOp(ti);
}

int Pop() {
	struct ThreadInfo *ti = malloc(sizeof(struct ThreadInfo));
  ti->id = unique_id++;
  ti->op = POP;
  ti->spin = 1;
	StackOp(ti);
  return ti->cell.pdata;
}

int main() {
  __SMACK_top_decl("axiom {:method \"Push\", \"add\"} true;");
  __SMACK_top_decl("axiom {:method \"Pop\", \"remove\"} true;");

  __SMACK_decl("var x: int;");

  Init();

  __SMACK_code("call {:async} @(@);", Push, 1);
  __SMACK_code("call {:async} @(@);", Push, 2);
  __SMACK_code("call {:async} x := @();", Pop);
  __SMACK_code("call {:async} x := @();", Pop);
  
  __SMACK_code("assume {:yield} true;");
  // __SMACK_code("assert {:spec \"no_thinair\"} true;");
  // __SMACK_code("assert {:spec \"unique_removes\"} true;");
  // __SMACK_code("assert {:spec \"no_false_empty\"} true;");
  // __SMACK_code("assert {:spec \"stack_order\"} true;");
  __SMACK_code("assert {:spec \"queue_order\"} true;");

  return 0;
}

