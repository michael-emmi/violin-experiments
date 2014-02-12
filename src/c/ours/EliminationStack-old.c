//#include <boost/interprocess/detail/atomic.hpp>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#define MAX_THREADS 10

typedef enum stack_op
  {
    POP = 0,
    PUSH
  } stack_op;

typedef struct Cell {
	struct Cell* pnext;
	size_t pdata;
} Cell;

typedef  struct ThreadInfo {
	size_t id;
	stack_op op;
	Cell *cell;
	int spin;
	bool empty;
} ThreadInfo;

typedef  struct Simple_Stack {
	Cell* ptop;
} Simple_Stack;

Simple_Stack S;
ThreadInfo** location;
size_t* collision;
size_t id_count;

void Init(){
	location = malloc(MAX_THREADS*sizeof(ThreadInfo*));
	collision = malloc(MAX_THREADS*sizeof(size_t));
	memset(collision,0,MAX_THREADS*sizeof(size_t));
	id_count = 1;
}

bool TryPerformStackOp(ThreadInfo* p){
	Cell *phead,*pnext;
	if(p->op==PUSH) {
		phead=S.ptop;
		p->cell->pnext=phead;
		if(__atomic_compare_exchange_n(&S.ptop,&phead,p->cell,true,0,0))
			return true;
		else
			return false;
	}
	if(p->op==POP) {
		phead=S.ptop;
		if(phead==NULL) {
			p->empty=true;
			return true;
		}
		pnext=phead->pnext;
		if(__atomic_compare_exchange_n(&S.ptop,&phead,pnext,true,0,0)) {
			p->cell=phead;
			return true;
		}
		else {
			p->empty=true;
			return false;
		}
	}
	return false;
}

void FinishCollision(ThreadInfo *p) {
	if (p->op==POP) {
		p->cell=((ThreadInfo*)location[p->id])->cell;
		location[p->id]=NULL;
	}
}

bool TryCollision(ThreadInfo* p,ThreadInfo* q,size_t mypid,size_t him) {
	if(p->op == PUSH) {
		if(__atomic_compare_exchange_n(&location[him],&q,p,true,0,0))
			return true;
		else
			return false;
	}
	if(p->op == POP) {
		if(__atomic_compare_exchange_n(&location[him],&q,NULL,true,0,0)){
			p->cell=q->cell;
			location[mypid]=NULL;
			return true;
		}
		else
			return false;
	}
	return false;
}



void LesOP(ThreadInfo *p) {
	while (1) {
		location[p->id]=p;
		size_t pos=random()%MAX_THREADS;
		size_t him=collision[pos];
		while(!__atomic_compare_exchange_n(&collision[pos],&him,p->id,true,0,0))
			him=collision[pos];
		if (him!=0) { // 0 means empty
			ThreadInfo* q=(ThreadInfo*)location[him];
			if(q != NULL && q->id==him && q->op!=p->op) {
				if(__atomic_compare_exchange_n(&location[p->id],&p,NULL,true,0,0)) {
					if(TryCollision(p,q,p->id,him)==true)
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
		sleep(p->spin);
		if (!__atomic_compare_exchange_n(&location[p->id],&p,NULL,true,0,0)) {
			FinishCollision(p);
			return;
		}
		stack:
		if (TryPerformStackOp(p)==true)
			return;
	}
}

int f(int x,int y) {
	printf("%d %d",x,y);
}

void StackOp(ThreadInfo* pInfo) {
	if(TryPerformStackOp(pInfo)==false)
		LesOP(pInfo);
	return;
}

void Push(size_t x) {
	ThreadInfo* temp = malloc(sizeof(ThreadInfo));
	temp->id = id_count;
	id_count++;
	temp->op = PUSH;
	Cell* c1 = malloc(sizeof(Cell));
	c1->pdata = x;
	temp->cell = c1;
	temp->spin = 1;
	temp->empty = false;
	StackOp(temp);
}

int Pop( ) {
	ThreadInfo* temp = malloc(sizeof(ThreadInfo));
	temp->id = id_count;
	id_count++;
	temp->op = POP;
/*	Cell* c1 = malloc(sizeof(Cell));
	c1->pdata = x;
	temp->cell = *c1; */
	temp->spin = 1;
	temp->empty = false;
	StackOp(temp);
	int m0,m1;
	if (temp->empty == false)
		return -1;
	else if (temp->cell->pdata == 0)
		m0=1;
	else if (temp->cell->pdata == 1)
		m1=1;
	f(m0,m1);
	return temp->cell->pdata;
}

int main() {

}

