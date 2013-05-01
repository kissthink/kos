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
#ifndef _File_h_
#define _File_h_ 1

#include "util/Log.h"
#include "mach/platform.h"

#include <errno.h>
#include <unistd.h>

class File {
  static const size_t BlockSize = pagesize<1>();

  bufptr_t start;
  size_t length;
  size_t offset;

public:
  File(vaddr s, size_t l) : length(l), offset(0) {
    start = new char[align_up(l, BlockSize)];
    KASSERT( vaddr(start) != topaddr && start != nullptr, "out of memory" );
    kcout << FmtHex(start) << kendl;
    memcpy(start, bufptr_t(s), l);
  }

  size_t read(bufptr_t buf, size_t len) {
    if (offset + len > length) len = length - offset;
    memcpy( buf, start + offset, len );
    offset += len;
    return len;
  }

  off_t lseek(off_t o, int whence) {
    switch (whence) {
      case SEEK_SET: offset = o; break;
      case SEEK_CUR: offset += o; break;
      case SEEK_END: *__errno() = EINVAL; return -1;
      default: *__errno() = EINVAL; return -1;
    }
    if (offset > length) offset = length;
    return offset;
  }

};

#endif /* _File_h_ */
