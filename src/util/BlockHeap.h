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
#ifndef _BlockHeap_h_
#define _BlockHeap_h_ 1

#include "util/SpinLock.h"

template <typename BaseHeap, size_t allocSize, typename Lock = NoLock>
class BlockHeap {
  volatile Lock lock;   
  vaddr blockStart;
  vaddr blockEnd;
public:
  BlockHeap() : blockStart(0), blockEnd(0) {}

  vaddr alloc(size_t size) {
    LockObject<Lock> lo(lock);
    if (blockStart + size > blockEnd) {
      blockStart = BaseHeap::alloc(allocSize);
      if (blockStart == illaddr) {
        blockEnd = illaddr;
        return illaddr;
      }
      blockEnd = blockStart + allocSize;
    }
    vaddr result = blockStart;
    blockStart += size;
    return result;
  }
};

#endif /* _BlockHeap_h_ */
