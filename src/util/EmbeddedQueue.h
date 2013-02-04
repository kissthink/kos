/******************************************************************************
    Copyright 2012 Martin Karsten

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/
#ifndef _EmbeddedQueue_h_
#define _EmbeddedQueue_h_ 1

#include "Log.h"

template<typename T> class EmbeddedQueue;

template<typename T> class EmbeddedElement {
  friend class EmbeddedQueue<T>;
  T* next;
};

template<typename T> class EmbeddedQueue {
  T* head;
  T* tail;
public:
  EmbeddedQueue() : head(nullptr) {}
  bool empty() const { return head == nullptr; }
  void push(T* e) {
    e->EmbeddedElement<T>::next = nullptr;
    if unlikely(empty()) head = e;
    else tail->EmbeddedElement<T>::next = e;
    tail = e;
  }
  T* front() {
    KASSERT(!empty(), this);
    return head;
  }
  const T* front() const {
    KASSERT(!empty(), this);
    return head;
  }
  void pop() {
    KASSERT(!empty(), this);
    head = head->EmbeddedElement<T>::next;
  }
};

#endif
