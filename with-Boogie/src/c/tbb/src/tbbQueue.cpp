#include "tbb/concurrent_queue.h"
// #include <iostream>

tbb::concurrent_queue<int> queue;

void Enqueue(int i) {
    queue.push(i);
}

int Dequeue() {
    int r;
    if (queue.try_pop(r)) return r;
    else return 2;
}

void Init() {
}
/*
int main() {
    std::cout << Dequeue() << std::endl;
    Enqueue(0);
    Enqueue(1);
    std::cout << Dequeue() << std::endl;
    std::cout << Dequeue() << std::endl;
    std::cout << Dequeue() << std::endl;
    return 0;
}*/