#include<stdio.h>
#include<stdlib.h>

typedef int bool;
#define true 1
#define false 0

struct node_t {
    int value;
    struct node_t *next;
};
struct queue_t {
    struct node_t *head;
    struct node_t *tail;
    bool hlock;
    bool tlock;
};

struct queue_t q;

struct node_t* createNode(int value, struct node_t *next) {
    struct node_t *node = malloc (sizeof *node);
    node->value = value;
    node->next = next;
    return node;
}

void printNode(struct node_t n) {
   printf("(%i,%p)",n.value,n.next);
}

void printQueue() {
    printf("head: ");
    printNode(*q.head);
    printf("\ntail: ");
    printNode(*q.tail);
    printf("\n");
    struct node_t *current = q.head;
    while (current != 0) {
        printNode(*current);
        current = current->next;
    }
    printf("\n-----------------\n");
}
    
    

void initialize() {
    struct node_t *node = createNode(-1,0);    // Allocate a free node
    q.head = node;
    q.tail = node; // Both Head and Tail point to it
    q.hlock = false;
    q.tlock = false;
}

void locktail() {
    while(q.tlock) ;
    q.tlock = true;
}

void lockhead() {
    while(q.hlock) ;
    q.hlock = true;
}

void unlocktail() {
    q.tlock = false;
}

void unlockhead() {
    q.hlock = false;
}

void enqueue(int value) {
    struct node_t *node = createNode(value,0);       // Allocate a new node from the free list
    // Copy enqueued value into node
    // Set next pointer of node to NULL
    locktail();   // Acquire T_lock in order to access Tail
    q.tail->next = node;  // Link node at the end of the linked list
    q.tail = node;    // Swing Tail to node
    unlocktail();   // Release T_lock
}


int dequeue() {
    lockhead();         // Acquire H_lock in order to access Head
    struct node_t *node = q.head;   // Read Head
    struct node_t *new_head = node->next;  // Read next pointer
    if (new_head == 0) { // Is queue empty?
        unlockhead(); // Release H_lock before return
        return -1; // Queue was empty
    }
    int pvalue = new_head->value; // read head
    q.head = new_head ; // Swing Head to next node
    unlockhead();  // Release H_lock
    //     free(node);     // Free node
    return pvalue;    // Queue was not empty, dequeue succeeded
}

int main() {
    initialize();
    enqueue(3);
    enqueue(4);
    enqueue(5);
    printf("dequeued: %i\n",dequeue());
    printf("dequeued: %i\n",dequeue());
    printf("dequeued: %i\n",dequeue());
    printf("dequeued: %i\n",dequeue());
}