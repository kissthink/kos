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
#ifndef _IndexMap_h_
#define _IndexMap_h_ 1

#include "util/basics.h"
#include "mach/platform.h"

template<typename Type, size_t BlockSize = pagesize<1>()>
class IndexMap {
  struct Chunk;
  static_assert( sizeof(Type*) == sizeof(Chunk*), "sizeof(Type*) != sizeof(Chunk*)" );
  static const size_t ptrCnt = BlockSize / sizeof(Chunk*);
  static const size_t ptrCntLog = floorlog2_c(ptrCnt);
  static_assert( ptrCnt = pow2(ptrCntLog), "ptrCnt not a power of 2");
  struct Chunk {
    Chunk* ptr[ptrCnt];
    buf_t padding[BlockSize - ptrCnt * sizeof(Chunk*)];
  };
  Chunk* top;
  size_t depth;
  size_t count;
  size_t cap;
  void expand() {
    Chunk* old = top;
    top = new Chunk;
    memset(top, 0, sizeof(Chunk));
    top.ptr[0] = old;
    cap = cap * ptrCnt;
    depth += 1;
  }
  Chunk** getref(size_t idx) {
    Chunk** ptr = &top;
    for (size_t d = depth; d > 0; d = d / ptrCnt) {
      ptr = &(ptr[count >> (d * ptrCntLog)].ptr);
    }
    return ptr;
  }
public:
  IndexMap() : top(nullptr), depth(0), count(0), cap(1) {}
  size_t append(Type* o) {
    if unlikely(count == cap) expand();
    *getref(count) = reinterpret_cast<Chunk*>(o);
    count += 1;
    return count;
  }
  Type* get(size_t idx) {
    if unlikely(idx >= count) return nullptr;
    return reinterpret_cast<Type*>(*getref(idx));
  }
  bool trim(size_t idx) {
    // TODO: remove everything starting at idx
  }
  size_t capacity() const { return capacity; }
  bool empty() const { return count == 0; }
  size_t size() const { return count; }
  // forall? need iterator?
  // operator[]
};

template<typename Type, size_t BlockSize = pagesize<1>()>
class SparseIndexMap {
  struct Chunk;
  static_assert( sizeof(Type*) == sizeof(Chunk*), "sizeof(Type*) != sizeof(Chunk*)" );
  static const size_t maskCnt = divup(BlockSize / sizeof(Chunk*), sizeof(mword));
  static const size_t  ptrCnt = (BlockSize - maskCnt) / sizeof(Chunk*);
  struct Chunk {
    mword mask[maskCnt];
    Chunk* ptr[ptrCnt];
    char padding[BlockSize - (ptrCnt * sizeof(Chunk*) + maskCnt * sizeof(mword))];
  };
  Chunk* top;
  size_t depth;
  size_t count;
  size_t cap;
  void expand() {
    Chunk* old = top;
    top = new Chunk;
    memset(top, 0, sizeof(Chunk));
    top.mask[0] = 1;
    top.ptr[0] = old;
    cap = cap * ptrCnt;
    depth += 1;
  }
public:
  SparseIndexMap() : top(nullptr), depth(0), count(0), cap(1) {}
  size_t insert(Type* o) {
    if unlikely(count == cap) expand();
    return 0;
  }
  bool insert(size_t idx, Type* o) {
    while (idx < cap) expand();
    // use multiscan<maskCnt,true>(mask);
    return false;
  }
  bool erase(size_t idx) {
    // use multiscan<maskCnt>(mask);
    // automatically trim?
    return false;
  }
  Type* get(size_t idx) {
    // use multiscan<maskCnt>(mask);
    return nullptr;
  }
  size_t capacity() const { return capacity; }
  bool empty() const { return count == 0; }
  size_t size() const { return count; }
  // forall? need iterator?
  // operator[]
};

#endif /* _IndexMap_h_ */
