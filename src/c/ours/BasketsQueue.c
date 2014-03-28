#include <stdio.h>
#include <stdlib.h>
#include <smack.h>
#include <violin.h>

struct node_t;

struct pointer_t {
    struct node_t *ptr;
    bool deleted;
    int tag;
};
struct node_t {
    int value;
    struct pointer_t next;
};


struct pointer_t tail;
struct pointer_t head;

struct node_t* createNode(int value, struct pointer_t next) {
    struct node_t *result = malloc(sizeof *result);
    result->value = value;
    result->next = next;
    return result;
}

struct pointer_t* createPt(struct node_t *ptr, bool deleted, int tag) {
    struct pointer_t *result = malloc(sizeof *result);
    result->ptr = ptr;
    result->deleted = deleted;
    result->tag = tag;
    return result;
}

void initialize() {
    struct pointer_t p;
    struct node_t *new_node = createNode(-1,p);
    p = *createPt(0,false,0);
    new_node->next = p;
    struct pointer_t *ht = createPt(new_node,false,0);
    tail = *ht;
    head = *ht;
}

bool cas(struct pointer_t *p, struct pointer_t t, struct pointer_t x) {
    if (equalPointers(*p,t)) {
        *p = x;
        return true;
    } else return false;
}

void printPointer(struct pointer_t p) {
    printf("(%p,%i,%i)",p.ptr,p.deleted,p.tag);
}

void printNode(struct node_t node) {
    printf("(%i,",node.value);
    printPointer(node.next);
    printf(")");
}

void printQueue() {
    printf("head:\n");
    printPointer(head);
    printf("\n");
    printNode(*head.ptr);
    printf("\n\n");
    printf("tail:\n");
    printPointer(tail);
    printf("\n");
    printNode(*tail.ptr);
    printf("\n\n");
    struct pointer_t *current = &head;
    while (current->ptr != 0) {
        printPointer(*current);
        printf("\n");
        printNode(*current->ptr);
        printf("\n\n");
        current = &current->ptr->next;
    }
    printf("\n-----------------\n");
}

void backoff_scheme() { }

bool equalPointers(struct pointer_t p1, struct pointer_t p2) {
    return p1.ptr == p2.ptr && p1.deleted == p2.deleted && p1.tag == p2.tag;
}

bool equalNodes(struct node_t n1, struct node_t n2) {
    return n1.value == n2.value && equalPointers(n1.next,n2.next);
}

void enqueue(int val) {
  VIOLIN_PROC;
  VIOLIN_OP;
  VIOLIN_OP_START(add,v);

    struct node_t *nd = malloc(sizeof *nd);
    nd->value = val;
    while (true) {
        struct pointer_t tl = tail;
        struct pointer_t next = tail.ptr->next;
        if (equalPointers(tl,tail)) {
            if (next.ptr == 0) {
                nd->next = *createPt(0,false,tl.tag+2);
                struct pointer_t pt = *createPt(nd,0,tl.tag+1);
                if (cas(&(tail.ptr->next), next, pt)) {
                    cas(&tail, tl, pt);
                    VIOLIN_OP_FINISH(add,0);
                    return ;
                }
                next = tail.ptr->next;
                while (next.tag==tail.tag+1 && !next.deleted) {
                    backoff_scheme();
                    nd->next = next;
                    if (cas(&tail.ptr->next, next, pt)) {
                        VIOLIN_OP_FINISH(add,0);
                        return ;
                    }
                    next = tail.ptr->next;
                }
            }
            else {
                while (next.ptr->next.ptr != 0 && &tail == &tl) {
                    next = next.ptr->next;
                }
                cas(&tail, tl, *createPt(next.ptr,0,tail.tag+1));
            }
        }
    }
}

const int MAX_HOPS = 3;

void free_chain(struct pointer_t head, struct pointer_t next) {
}

int dequeue() {
  VIOLIN_PROC;
  VIOLIN_OP;
  VIOLIN_OP_START(remove,0);

    while (true) {
        struct pointer_t hd = head;
        struct pointer_t tl = tail;
        struct pointer_t next = head.ptr->next;
        if (equalPointers(hd,head)) {
            if (equalNodes(*head.ptr,*tail.ptr)) {
                if (next.ptr == 0) {
                    VIOLIN_OP_FINISH(remove,-1);
                    return -1;
                }
                while (next.ptr->next.ptr != 0 && equalPointers(tail,tl)) {
                    next = next.ptr->next;
                }
                cas(&tail, tl, *createPt(next.ptr,0,tl.tag+1));
            }
            else {
                struct pointer_t iter = head;
                int hops = 0;
                while (next.deleted && !equalNodes(*iter.ptr,*tl.ptr) && equalPointers(hd,head)) {
                    iter = next;
                    next = iter.ptr->next;
                    hops++;
                }
                if (!equalPointers(head,hd)) {
                    continue;
                }
                else if (equalNodes(*iter.ptr,*tail.ptr)) {
                    free_chain(head, iter);
                }
                else {
                    int value = next.ptr->value;
                    if (cas(&iter.ptr->next, next, *createPt(next.ptr, 1, next.tag+1))) {
                        if (hops >= MAX_HOPS)
                            free_chain(head, next);
                        VIOLIN_OP_FINISH(remove,value);
                        return value;
                    }
                    backoff_scheme();
                }
            }
        }
    }
}

int main() {
  VIOLIN_PROC;
  VIOLIN_INIT;
  initialize();

  // __SMACK_top_decl("axiom {:static_threads} true;");
  __SMACK_decl("var x: int;");
  __SMACK_code("call {:async} @(@);", enqueue, 1);
  __SMACK_code("call {:async} x := @();", dequeue);
  __SMACK_code("call {:async} x := @();", dequeue);

  __SMACK_code("assume {:yield} true;");
  VIOLIN_CHECK(queue);
  return 0;
}