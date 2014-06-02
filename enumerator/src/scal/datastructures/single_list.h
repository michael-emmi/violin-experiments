// Copyright (c) 2012-2013, the Scal Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

// A singly-linked list using a sentinel node that can only be used in
// single-threaded.

#ifndef SCAL_DATASTRUCTURES_SINGLE_LIST_H_
#define SCAL_DATASTRUCTURES_SINGLE_LIST_H_

#include <stdlib.h>

#include "datastructures/queue.h"
#include "util/malloc.h"

template<typename T>
class SingleList : public Queue<T> {
 public:
  SingleList();
  bool enqueue(T item);
  bool dequeue(T *item);
  bool is_empty() const;

 private:
  template<typename S>
  struct Node {
    Node *next;
    S value;
  };

  Node<T> *head_;
  Node<T> *tail_;
};

template<typename T>
SingleList<T>::SingleList() {
  Node<T> *n = scal::get<Node<T> >(0);
  head_ = n;
  tail_ = n;
}

template<typename T>
bool SingleList<T>::is_empty() const {
  //Yield();
  if (head_ == tail_) {
    return true;
  } else {
    return false;
  }
}

template<typename T>
bool SingleList<T>::enqueue(T item) {
  Node<T> *n = scal::tlget<Node<T> >(0);
  n->value = item;
  //Yield();
  tail_->next = n;
  //Yield();
  tail_ = n;
  return true;
}

template<typename T>
bool SingleList<T>::dequeue(T *item) {
  if (head_ == tail_) {
    Yield();
    *item = (T)NULL;
    return false;
  } else {
    //Yield(); 
    *item = head_->next->value;
    Yield();
    head_ = head_->next;
    //Yield(); //without this yield it finds less violations; for 2d1a2r goes from 10 to 8
    return true;
  }
}

#endif  // SCAL_DATASTRUCTURES_SINGLE_LIST_H_
