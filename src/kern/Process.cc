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
#include "kern/Kernel.h"
#include "kern/Process.h"
#include "world/File.h"
#include "extern/elfio/elfio.hpp"

void Process::execElfFile(const kstring& fileName) {
  File* f = kernelFS.find(fileName)->second;
  KASSERT0(f);
  laddr file_lma = PageManager::vtol(vaddr(f->startptr()));
  KASSERT1(aligned(file_lma, pagesize<1>()), file_lma);

  ELFIO::elfio elfReader;
  bool check = elfReader.load(fileName.c_str());
  KASSERT0(check);
  KASSERT1(elfReader.get_class() == ELFCLASS64, elfReader.get_class());

  as.clonePagetable(kernelSpace);
  as.activate();
  for (int i = 0; i < elfReader.segments.size(); ++i) {
    const ELFIO::segment* pseg = elfReader.segments[i];
    if (pseg->get_type() != PT_LOAD) continue;  // not a loadable segment
    if (pseg->get_file_size() == 0)  continue;  // empty segment
    KASSERTN(pseg->get_file_size() <= pseg->get_memory_size(), pseg->get_file_size(), ' ', pseg->get_memory_size());

    size_t offset = align_down(pseg->get_file_offset(), pagesize<1>());
    size_t end_offset = align_up(pseg->get_file_offset() + pseg->get_file_size(), pagesize<1>());
    vaddr seg_vma = align_down(pseg->get_virtual_address(), pagesize<1>());

    /* If .rodata and .text are in the same segment and small enough to be
     * in the same page, then .rodata ends up executable.  */
    AddressSpace::PageType pageType = AddressSpace::RoData;
    if (pseg->get_flags() & PF_W) pageType = AddressSpace::Data;
    if (pseg->get_flags() & PF_X) pageType = AddressSpace::Code;
    as.mapPages<1>(file_lma + offset, seg_vma, end_offset - offset, pageType);

    size_t bssLength = pseg->get_memory_size() - pseg->get_file_size();
    if (bssLength > 0) {
      vaddr bssStart = pseg->get_virtual_address() + pseg->get_file_size();
      memset((void*) bssStart, 0, bssLength);
    }
  }

  DBG::outln(DBG::Process, "User AS: ", as);
  function_t entry = (function_t)elfReader.get_entry();
  thread = Thread::create(as, fileName.c_str());
  kernelSpace.activate();
  kernelScheduler.run(*thread, entry, nullptr);
}
