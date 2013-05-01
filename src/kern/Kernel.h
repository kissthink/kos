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
#ifndef _Kernel_h_
#define _Kernel_h_ 1

#include "kern/AddressSpace.h"
#include "kern/KernelVM.h"
#include "kern/Scheduler.h"

#include <map>
#include <string>

extern AddressSpace kernelSpace;
extern KernelVM kernelVM;
extern Scheduler kernelScheduler;

class File;
typedef basic_string<char,char_traits<char>,KernelAllocator<char>> kstring;
extern map<kstring,File*,std::less<kstring>,KernelAllocator<kstring>> kernelFS;

#endif /* _Kernel_h_ */
