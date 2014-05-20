#include <stdio.h>
#include <stdlib.h>
#include "unistd.h"

#include "queue.h"

#define QUEUE_POISON1 ((void*)0xCAFEBAB5)

typedef int bugged_mutex_t;

void bugged_mutex_init(bugged_mutex_t* lock, void *unused) {
    *lock = 0;
}

void bugged_mutex_lock(bugged_mutex_t* lock) {
    while (*lock != 0) ;
    sleep(1);
    *lock = 1;
}

void bugged_mutex_unlock(bugged_mutex_t* lock) {
    *lock = 0;
}
    
struct queue_root {
	struct queue_head *head;
	bugged_mutex_t head_lock;

	struct queue_head *tail;
	bugged_mutex_t tail_lock;

	struct queue_head divider;
};

struct queue_root *ALLOC_QUEUE_ROOT()
{
	struct queue_root *root = \
		malloc(sizeof(struct queue_root));
	bugged_mutex_init(&root->head_lock, NULL);
	bugged_mutex_init(&root->tail_lock, NULL);

	root->divider.next = NULL;
	root->head = &root->divider;
	root->tail = &root->divider;
	return root;
}

void INIT_QUEUE_HEAD(struct queue_head *head)
{
	head->next = QUEUE_POISON1;
}

void queue_put(struct queue_head *new,
	       struct queue_root *root)
{
	new->next = NULL;

	bugged_mutex_lock(&root->tail_lock);
	root->tail->next = new;
  sleep(1);
	root->tail = new;
	bugged_mutex_unlock(&root->tail_lock);
}

struct queue_head *queue_get(struct queue_root *root)
{
	struct queue_head *head, *next;

	while (1) {
		bugged_mutex_lock(&root->head_lock);
		head = root->head;
		next = head->next;
		if (next == NULL) {
			bugged_mutex_unlock(&root->head_lock);
			return NULL;
		}
		root->head = next;
		bugged_mutex_unlock(&root->head_lock);
		
		if (head == &root->divider) {
			queue_put(head, root);
			continue;
		}

		head->next = QUEUE_POISON1;
		return head;
	}
}


struct queue_root *queue;

void Enqueue(int v) {
    struct queue_head *item = malloc(sizeof(struct queue_head));
    INIT_QUEUE_HEAD(item);
    item->value = v;
    queue_put(item,queue);
}

int Dequeue() {
    struct queue_head *item = queue_get(queue);
    if (item == NULL) return 2;
    else return item->value;
}

void Init() {
    queue = ALLOC_QUEUE_ROOT();
}
