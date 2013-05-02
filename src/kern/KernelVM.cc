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
#include "extern/dlmalloc/malloc_glue.h"
#include "extern/dlmalloc/malloc.h"
#include "util/Debug.h"
#include "util/Log.h"
#include "kern/AddressSpace.h"
#include "kern/Kernel.h"
#include "kern/KernelVM.h"

ptr_t operator new(size_t s) { return (ptr_t)kernelVM.alloc(s); }  
ptr_t operator new[](size_t s) { return (ptr_t)kernelVM.alloc(s); }
void operator delete(ptr_t p) noexcept { KASSERT(false, "delete" ); }
void operator delete[](ptr_t p) noexcept { KASSERT(false, "delete[]" ); }

void globaldelete(ptr_t addr, size_t size) {
  kernelVM.release((vaddr)addr, size);
}

extern "C" void* mmap(void* addr, size_t len, int, int, int, _off64_t) {
  DBG::out(DBG::VM, "VM/mmap: ", FmtHex(addr), '/', FmtHex(len));
  KASSERT(!addr, addr);
  void* p = (void*)kernelVM.alloc(len);
  DBG::outln(DBG::Libc, " -> ", p);
  return p;
}

extern "C" int munmap(void* addr, size_t len) {
  DBG::outln(DBG::VM, "VM/munmap: ", FmtHex(addr), '/', FmtHex(len));
  kernelVM.release((vaddr)addr, len);
  return 0;
}

void KernelVM::checkExpand(size_t size) {
  if (availableMemory.check(size)) return;
  size = pow2(ceilinglog2(size));
  vaddr newmem = kernelSpace.mapPages<dpl,true>(align_up(size, pagesize<dpl>()));
  KASSERT( newmem != topaddr, "out of memory?" );
  bool check = availableMemory.insert(newmem, align_up(size, pagesize<dpl>()));
  KASSERT( check, newmem );
}

vaddr KernelVM::allocInternal(size_t size) {
  KASSERT( aligned(size, pow2(min)), size );
  ScopedLock<> so(lock);
  // TODO: problem, if size is pow2 and one chunk is all that's left?
  // -> could request larger chunk, but more fragementation?
  checkExpand(std::max(size, 2 * size_t(DEFAULT_GRANULARITY)));
  vaddr addr = availableMemory.retrieve(size);
  KASSERT( addr != topaddr, *this );
  return addr;
}

void KernelVM::releaseInternal(vaddr p, size_t size) {
  KASSERT( aligned(p, pow2(min)), p );
  KASSERT( aligned(size, pow2(min)), size );
  ScopedLock<> so(lock);
  availableMemory.insert(p, size);
  // TODO: would be nice to have retrieveMax in BuddyMap
  while ( availableMemory.check(pagesize<dpl>()) ) {
    vaddr addr = availableMemory.retrieve(pagesize<dpl>());
    KASSERT(addr != topaddr, "internal error");
    bool check = kernelSpace.unmapPages<dpl,true>(addr, pagesize<dpl>());
    KASSERT( check, addr );
  }
}

// memory is used to initialize dlmalloc's mspace
void KernelVM::init(vaddr start, vaddr end) {
  KASSERT( end - start >= DEFAULT_GRANULARITY, end - start );
  kmspace = create_mspace_with_base( (ptr_t)start, end - start, 0 );
}

// memory is marked as available
void KernelVM::addMemory(vaddr start, vaddr end) {
  availableMemory.insert(start, (end - start));
}

ptr_t KernelVM::malloc(size_t s) {
  return mspace_malloc(kmspace, s);
}

void KernelVM::free(ptr_t p) {
  mspace_free(kmspace, p);
}

std::ostream& operator<<(std::ostream& os, const KernelVM& vm ) {
  vm.availableMemory.print<KernelAllocator<vaddr>>(os);
  return os;
}
