#define MEMORY_MODEL_NO_REUSE 1
#define VIOLIN_COUNTING 0

//#include <boost/interprocess/detail/atomic.hpp>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <smack.h>
#include <violin.h>
#include <c2s.h>

// XXX this translates to LLVM's cmpxchg instruction, but is really
// broken in SMACK.
// #define CAS(x,y,z) __atomic_compare_exchange_n(x,&(y),z,true,0,0)
  
#define CASDEF(t,ty) \
  bool t ## _cas(ty *p, ty cmp, ty new) { \
    if (*p == cmp) { \
      *p = new; \
      return true; \
    } \
    return false; \
  }
#define CAS(t,x,y,z) t ## _cas(x,y,z)
// #define CAS(t,x,y,z) __atomic_compare_exchange_n(x,&(y),z,true,0,0)

// NOTE changing these constants has a noticable impact on verification time,
// in very unexpected ways, i.e., smaller not better.
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

CASDEF(int,int)
CASDEF(ti,struct ThreadInfo*)
CASDEF(c,struct Cell*)

int GetPosition(struct ThreadInfo *p) {
  int pos;
  pos = __SMACK_nondet();
  __SMACK_assume(0 <= pos && pos < COLLISION_ARRAY_SIZE);
  return pos;
}
void delay(int d);

void StackOp(struct ThreadInfo *p) {

  // REACH(p->id == 1); // 3s/3R/1U
  // REACH(p->id == 2); // 3s/3R/1U
  // REACH(p->id == 3); // 3s/3R/1U
  // REACH(p->id == 4); // 3s/3R/1U
  // REACH(p->id == 5); // 6s/3R/1U
  // REACH(p->id == 6); // 25s/3R/1U (WHY??)

	if (TryPerformStackOp(p) == false)
		LesOP(p);
	return;
}

void LesOP(struct ThreadInfo *p) {
  int mypid = p->id;

  // REACH(p->id == 6); // _s/3R/1U (WHY??)

  while (1) {
		location[mypid] = p;
    int pos = GetPosition(p); // CONSTANTIN: pos = random() % MAX_THREADS
		int him = collision[pos];

    // REACH(him == 4); // 6s/3R/1U

		while (!CAS(int,&collision[pos],him,mypid))
			him = collision[pos];

    // REACH(mypid == 5 && him == 4); // 7s/3R/1U

		if (him > 0) {
			struct ThreadInfo* q = location[him];

      // REACH(mypid == 5 && him == 4); // 7s/3R/1U

			if (q != NULL && q->id == him && q->op != p->op) {

        // REACH(true); // 3s/3R/1U

        // REACH(mypid == 5 && him == 4); // step 7 -- 7s/3R/1U

        // REACH(mypid == 6 && him == 5); // step 8 -- ??

        __SMACK_code("assume {:yield} true;");

				if (CAS(ti,&location[mypid],p,NULL)) {

          // REACH(true); // 3s/3R/1U
            
					if (TryCollision(p,q,him) == true) {
            // REACH(mypid == 5); // step 9 -- 8s/3R/1U
						return;

					} else {
            // REACH(mypid == 6); // step 10 -- ??

						goto stack;
          }
				}
				else {
					FinishCollision(p);
					return;
				}
			}
		}

		delay(p->spin); // CONSTANTIN: sleep(p->spin)

    // REACH(mypid == 4); // step 6 -- 4s/3R/1U

    __SMACK_code("assume {:yield} true;");

    // REACH(mypid == 4 && location[mypid] != p); // step 12

		if (!CAS(ti,&location[mypid],p,NULL)) {
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

    // REACH(p->id == 5); // step 3 -- 5s/3R/1U

    __SMACK_code("assume {:yield} true;");

		if(CAS(c,&S.ptop,phead,&p->cell)) {
      
      // REACH(p->id == 1); // step 1 -- 3s/3R/1U
      // REACH(p->id == 2); // step 5 -- 3s/3R/1U

			return true;
		} else
			return false;
	}
	if (p->op==POP) {
    // REACH(p->id == 6); // 35s/3R/1U (WHY??)

		phead = S.ptop;
		if (phead == NULL) {
      // p->cell = EMPTY; // actual code, modified to avoid memcpy
      p->cell.pdata = EMPTY.pdata;
			return true;
		}
		pnext = phead->pnext;

    // REACH(p->id == 4); // step 2 -- 4s/3R/1U
    // REACH(p->id == 6); // step 4 -- 44s/3R/1U (WHY??)

    __SMACK_code("assume {:yield} true;");

    // REACH(p->id == 6); // 42s/3R/1U (WHY??)
    // REACH(p->id == 6 && S.ptop != phead); // 

		if (CAS(c,&S.ptop,phead,pnext)) {

      // REACH(p->id == 6); // 46s/3R/1U (WHY??)

			// p->cell = *phead; // actual code, modified to avoid memcpy
      p->cell.pdata = phead->pdata;
			return true;
		}
		else {

      // REACH(p->id == 4); // 4s/3R/1U 
      // REACH(p->id == 6); // _s/3R/1U (WHY??) -- WORKING HERE

      // p->cell = EMPTY; // actual code, modified to avoid memcpy
      p->cell.pdata = EMPTY.pdata;
			return false;
		}
	}
	return false;
}

void FinishCollision(struct ThreadInfo *p) {
  int mypid = p->id;
  
	if (p->op == POP) {
    // p->cell = location[mypid]->cell; // actual code, modified to avoid memcpy
    p->cell.pdata = location[mypid]->cell.pdata;
		location[mypid] = NULL;
	}
}

bool TryCollision(struct ThreadInfo *p, struct ThreadInfo *q, int him) {
  int mypid = p->id;

	if (p->op == PUSH) {

    __SMACK_code("assume {:yield} true;");

		if (CAS(ti,&location[him],q,p))
			return true;
		else
			return false;
	}
	if (p->op == POP) {

    __SMACK_code("assume {:yield} true;");

    // REACH(true); // 4s/3R/1U
    // REACH(location[him] == q); // 4s/3R/1U
    // REACH(location[him] != q); // ???

		if (CAS(ti,&location[him],q,NULL)) {
			// p->cell = q->cell; // actual code, modified to avoid memcpy
      p->cell.pdata = q->cell.pdata;

			location[mypid] = NULL;
			return true;
		}
		else {
      // REACH(true); // ???
			return false;
    }
	}
	return false;
}

void Push(int x) {

  __SMACK_code("assume {:yield} true;");

  VIOLIN_PROC;
  VIOLIN_OP;
  VIOLIN_OP_START(add,x);
  
	struct ThreadInfo *ti = malloc(sizeof(struct ThreadInfo));
  ti->id = ++unique_id;
	ti->op = PUSH;
  ti->cell.pdata = x;
  ti->spin = 1;
	StackOp(ti);
  
  VIOLIN_OP_FINISH(add,0);
}

int Pop() {

  __SMACK_code("assume {:yield} true;");

  VIOLIN_PROC;
  VIOLIN_OP;
  VIOLIN_OP_START(remove,0);
  
	struct ThreadInfo *ti = malloc(sizeof(struct ThreadInfo));
  ti->id = ++unique_id;
  ti->op = POP;
  ti->spin = 1;

  // REACH(ti->id == 4); // 3s/3R/1U
  // REACH(ti->id == 6); // 10s/3R/1U

  StackOp(ti);
  
  VIOLIN_OP_FINISH(remove,ti->cell.pdata);
  return ti->cell.pdata;
}

int main() {
  VIOLIN_PROC;
  VIOLIN_INIT;

  VALUES(2);

  __SMACK_decl("var x: int;");
  __SMACK_code("call {:async} @(@);", Push, 1);
  __SMACK_code("call {:async} x := @();", Pop);
  __SMACK_code("call {:async} x := @();", Pop);
  __SMACK_code("call {:async} @(@);", Push, 2);
  __SMACK_code("assume {:yield} true;");

  VIOLIN_CHECK(stack);
  return 0;
}
