/******************************************************************************
    Copyright 2013 Behrooz Shafia

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
#ifndef ELFLoader_h_
#define ELFLoader_h_

#include "mach/platform.h" //for vaddr

//Forward declare AddressSpace and File
class AddressSpace;
class File;

class ELFLoader {
private:
  bool initialized;
public:
  ELFLoader() : initialized(false) { };
  bool loadAndMapELF(File* elfFile, AddressSpace* addSpace);
  vaddr findMainAddress();
  vaddr findEntryAddress();
};

#endif /* ELFLoader_h_ */
