/******************************************************************************
    Copyright 2013 Martin Karsten

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
#ifndef _KernelQueues_h_
#define _KernelQueues_h_ 1

#include "util/Output.h"

#include <array>
#include <queue>

template<typename Element, size_t N>
class StaticArray : public array<Element,N> {
public:
  typedef Element ElementType;
  explicit StaticArray( size_t ) {}
};

template<typename Element, typename Allocator>
class DynamicArray {
  Element* buffer;
  size_t max;
public:
  typedef Element ElementType;
  explicit DynamicArray( size_t N, Element* ptr = nullptr )
    : buffer(ptr ? ptr : Allocator().allocate(N)), max(N) {
    KASSERT0(N);
  }
  size_t max_size() const { return max; }
  Element& operator[] (size_t i) { return buffer[i]; }
  const Element& operator[] (size_t i) const { return buffer[i]; }
};

template<typename Array>
class RingBuffer {
  size_t newest;
  size_t oldest;
  size_t count;
  Array array;
public:
  typedef typename Array::ElementType Element;
  explicit RingBuffer( size_t N = 0 )
    : newest(N-1), oldest(0), count(0), array(N) {}
  bool empty() const { return count == 0; }
  bool full() const { return count == array.max_size(); }
  size_t size() const { return count; }
  size_t max_size() const { return array.max_size(); }
  Element& front() {
    KASSERT0(count);
    return array[oldest];
  }
  const Element& front() const {
    KASSERT0(count);
    return array[oldest];
  }
  Element& back() {
    KASSERT0(count);
    return array[newest];
  }
  const Element& back() const {
    KASSERT0(count);
    return array[newest];
  }
  void push( const Element& x ) {
    KASSERT0(count < array.max_size());
    newest = (newest + 1) % array.max_size();
    array[newest] = x;
    count += 1;
  }
  void pop() {
    KASSERT0(count);
    oldest = (oldest + 1) % array.max_size();
    count -= 1;
  }
};

template<typename E, size_t N>
class StaticRingBuffer : public RingBuffer<StaticArray<E,N>> {
public:
  explicit StaticRingBuffer(size_t) {}
};

template<typename E, typename A>
class DynamicRingBuffer : public RingBuffer<DynamicArray<E,A>> {
public:
  explicit DynamicRingBuffer(size_t N) : RingBuffer<DynamicArray<E,A>>(N) {}
};

template<typename E, typename A>
class FiniteQueueBuffer : public queue<E,deque<E,A>> {
  size_t max;
public:
  typedef E Element;
  FiniteQueueBuffer( size_t N ) : max(N) { KASSERT0(N); }
  bool full() const { return queue<E,deque<E,A>>::size() == max; }
  size_t max_size() const { return max; }
};

// TODO: change size on-the-fly? need lock-down of synchronized queue...
// gradual vs. immediate change?
// implement change_size everywhere, but assert not called in static array...

#endif /* _KernelQueues_h_ */
