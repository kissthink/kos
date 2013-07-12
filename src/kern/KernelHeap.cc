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
#include "kern/AddressSpace.h"
#include "kern/Kernel.h"
#include "kern/KernelVM.h"

ptr_t operator new(size_t s) { return (ptr_t)kernelVM.alloc<false>(s); }  
ptr_t operator new[](size_t s) { return (ptr_t)kernelVM.alloc<false>(s); }
void operator delete(ptr_t p) noexcept { ABORT1("delete"); }
void operator delete[](ptr_t p) noexcept { ABORT1("delete[]"); }

void globaldelete(ptr_t addr, size_t size) {
  kernelVM.release<false>((vaddr)addr, size);
}

extern "C" void* mmap(void* addr, size_t len, int, int, int, _off64_t) {
  DBG::out(DBG::VM, "VM/mmap: ", FmtHex(addr), '/', FmtHex(len));
  KASSERT1(!addr, addr);
  void* p = (void*)kernelVM.alloc(len);
  DBG::outln(DBG::Libc, " -> ", p);
  return p;
}

extern "C" int munmap(void* addr, size_t len) {
  DBG::outln(DBG::VM, "VM/munmap: ", FmtHex(addr), '/', FmtHex(len));
  kernelVM.release((vaddr)addr, len);
  return 0;
}

void KernelVM::expand(size_t size) {
  // allocate/insert pow2-aligned chunk of at least default page size
  size = std::max(pow2(ceilinglog2(size)), pagesize<dpl>());
  vaddr newmem = kernelSpace.allocPages<dpl>(size, AddressSpace::Data);
  KASSERTN(newmem != topaddr, *this);
  bool check = availableMemory.insert(newmem, size);
  KASSERT1(check, newmem);
}

void KernelVM::checkExpand(size_t size) {
  if (!availableMemory.check(DEFAULT_GRANULARITY)) {
    expand(DEFAULT_GRANULARITY);
  }
  if (!availableMemory.checkCond(size, DEFAULT_GRANULARITY)) {
    expand(size);
  }
}

vaddr KernelVM::allocInternal(size_t size) {
  KASSERT1(aligned(size, pow2(min)), FmtHex(size));
  ScopedLock<> so(lock);
  checkExpand(size);
  vaddr addr = availableMemory.retrieve(size);
  KASSERTN(addr != topaddr, *this);
  return addr;
}

void KernelVM::releaseInternal(vaddr p, size_t size) {
  KASSERTN( aligned(p, pow2(min)), FmtHex(p), '/', FmtHex(size) );
  KASSERTN( aligned(size, pow2(min)), FmtHex(p), '/', FmtHex(size) );
  ScopedLock<> so(lock);
  availableMemory.insert(p, size);
  // TODO: would be nice to have retrieveMax in BuddyMap
  while ( availableMemory.check(pagesize<dpl>()) ) {
    vaddr addr = availableMemory.retrieve(pagesize<dpl>());
    KASSERTN(addr != topaddr, *this);
    bool check = kernelSpace.releasePages<dpl>(addr, pagesize<dpl>());
    KASSERT1( check, addr );
  }
}

// memory is used to initialize dlmalloc's mspace
void KernelVM::init(vaddr start, vaddr end) {
  KASSERTN( end - start >= DEFAULT_GRANULARITY, FmtHex(start), '-', FmtHex(end) );
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

ostream& operator<<(ostream& os, const KernelVM& vm ) {
  vm.availableMemory.print<KernelAllocator<vaddr>>(os);
  return os;
}
