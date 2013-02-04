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
#ifndef _BlindHeap_h_
#define _BlindHeap_h_ 1

#include "util/basics.h"
#include "mach/platform.h"

/* BlindHeap provides a simple front-end to another Heap class and can deal
 * with unknown sizes during free/release by storing the size of the
 * allocated chunk in the machine word at the beginning of the chunk. */

template <typename BaseHeap>
class BlindHeap {
public:
  static vaddr alloc(size_t size) {
    vaddr ptr = BaseHeap::alloc(size + sizeof(size_t));
    *(size_t*)ptr = size + sizeof(size_t);
    return ptr + sizeof(size_t);
  }
  static void release(vaddr p, size_t = 0) {
    vaddr ptr = p - sizeof(size_t);
    BaseHeap::release( ptr, *(size_t*)ptr );
  }
};

#endif /* _BlindHeap_h_ */
