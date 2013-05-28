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
#include "kern/FrameManager.h"

ostream& operator<<(ostream& os, const FileCache& fc) {
  fc.availableMemory.print<KernelAllocator<vaddr>>(os);
  return os;
}

ostream& operator<<(ostream& os, const FrameManager& fm) {
  fm.availableMemory.print(os);
  return os;
}

