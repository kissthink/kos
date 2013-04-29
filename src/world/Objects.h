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
#ifndef _Object_h_
#define _Object_h_ 1

#include "util/basics.h"
#include "util/Log.h"

#include <vector>

class ExpandableObject {
  static const mword csize = pagesize<1>();
  struct Chunk {
    buf_t data[csize];
  };
  std::vector<Chunk*> contents;
  size_t length;

public:
  ~ExpandableObject() {
    for (Chunk* c : contents) globaldelete(c,sizeof(Chunk));
  }

  void init(bufptr_t start, size_t len) {
    size_t chunks = 1 + (len - 1) / csize;                   // number of chunks
    contents.reserve(chunks);
    size_t i = 0;
    size_t offset = 0;
    for (; i < chunks - 1; i += 1) {                         // full chunks
      contents[i] = new Chunk;                               // allocate chunks
      KASSERT( contents[i], "out of memory" );
      memcpy(contents[i]->data, start + offset, csize);
      offset += csize;
    }
    memcpy(contents[i]->data, start + offset, len % csize);  // last chunk
  }

  size_t read(bufptr_t buf, size_t len, size_t offset) {
    if (offset + len > length) len = length - offset;        // trim request
    size_t i = offset / csize;
    size_t copied = csize - offset;
    memcpy(buf, contents[i]->data + offset % csize, copied); // first chunk
    for (;;) {                                               // full chunks
      i += 1;
      if unlikely(copied + csize > len) break;
      memcpy(buf + copied, contents[i]->data, csize);
      copied += csize;
    }
    memcpy(buf + copied, contents[i]->data, len - copied);   // last chunk
    return len;
  }

  size_t write(bufptr_t buf, size_t len, size_t offset) {
    size_t chunks = 1 + (offset + len - 1) / csize;          // number of chunks
    for (size_t j = contents.size(); j < chunks; j += 1) {
      contents[j] = new Chunk;                               // allocate chunks
      KASSERT( contents[j], "out of memory" );
    }
    if (offset + len > length) length = offset + len;        // expand object
    size_t i = offset / csize;
    size_t copied = csize - offset;
    memcpy(contents[i]->data + offset % csize, buf, copied); // first chunk
    for (;;) {                                               // full chunks
      i += 1;
      if unlikely(copied + csize > len) break;
      memcpy(contents[i]->data, buf + copied, csize);
      copied += csize;
    }
    memcpy(contents[i]->data, buf + copied, len - copied);   // last chunk
    return len;
  }

};

template<typename T, typename Alloc = allocator<T>>
class ObjectStore {
  std::vector<T,Alloc> store;
public:
  size_t insert(const T& object) {
    store.push_back(object);
    return store.size() - 1;
  }
  T& access(size_t idx) {
    KASSERT(idx < store.size(), idx);
    return store[idx];
  }
};

template<typename T>
struct ObjectHandle {
  T& object;
  size_t offset;
};

#endif /* _Object_h_ */
