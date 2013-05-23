/******************************************************************************
    Copyright 2012-2013 Martin Karsten

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

#include "Output.h"

template<typename T, int ID = 0> class EmbeddedQueue;

template<typename T, int ID = 0> class EmbeddedElement {
  friend class EmbeddedQueue<T,ID>;
  T* next;
};

template<typename T, int ID> class EmbeddedQueue {
  T* head;
  T* tail;
public:
  EmbeddedQueue() : head(nullptr) {}
  bool empty() const { return head == nullptr; }
  void push(T* e) {
    e->EmbeddedElement<T,ID>::next = nullptr;
    if unlikely(empty()) head = e;
    else tail->EmbeddedElement<T,ID>::next = e;
    tail = e;
  }
  T* front() {
    KASSERT0(!empty());
    return head;
  }
  const T* front() const {
    KASSERT0(!empty());
    return head;
  }
  void pop() {
    KASSERT0(!empty());
    head = head->EmbeddedElement<T,ID>::next;
  }
};

#endif
