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
#include "kern/Kernel.h"
#include "world/ELFLoader.h"
#include "world/File.h"
#include "extern/elfio/elfio.hpp"

ELFIO::elfio elfReader; //elfio Instance

bool ELFLoader::loadAndMapELF(File* elfFile, AddressSpace* addSpace) {
  if (!elfReader.load("testprogram2")) return false;
  initialized = true;
  KASSERT1(elfReader.get_class() == ELFCLASS64, elfReader.get_class());

  for (int i = 0; i < elfReader.segments.size(); ++i) { // iterate segments
    const ELFIO::segment* pseg = elfReader.segments[i];

    if (pseg->get_type() != PT_LOAD) continue; // not a loadable segment
    if (pseg->get_file_size() == 0)  continue; // empty segment

    KASSERTN(pseg->get_file_size() <= pseg->get_memory_size(),
        pseg->get_file_size(), ' ', pseg->get_memory_size());

    vaddr offset = vaddr(elfFile->startptr()) + pseg->get_data_offset(); // get segment offset in file
    offset = align_down(offset, pagesize<1>()); // align it
    vaddr seg_la = PageManager::vtol(offset);   // get linear address
    seg_la = align_down(seg_la, pagesize<1>()); // align it

    vaddr seg_va = pseg->get_virtual_address(); // get virtual address
    seg_va = align_down(seg_va, pagesize<1>()); // align it
    size_t padding = pseg->get_virtual_address() & 0xfff;
    size_t size = align_up(pseg->get_file_size() + padding, pagesize<1>());

    /* ELF segments contain several sections, while permissions are
     * assigned to the pages.  If .rodata and .text are in the same
     * segment and small enough to be in the same page, then .rodata ends
     * up executable.  */
    AddressSpace::PageType permType = AddressSpace::RoData; // most restrictive
    if (!(pseg->get_flags() & PF_W))
      permType = AddressSpace::Data;
    if (pseg->get_flags() & PF_X)
      permType = AddressSpace::Code;
    addSpace->mapPages<1>(seg_la, seg_va, size, permType);

    if (pseg->get_memory_size() > pseg->get_file_size()) { //zero out BSS
      vaddr bssStart = seg_va + pseg->get_file_size() + 1;
      memset((void*) bssStart, 0,
          pseg->get_memory_size() - pseg->get_file_size());
    }
  }
  return true;
}

vaddr ELFLoader::findMainAddress() {
  if (!initialized) return topaddr;
  ELFIO::Elf_Half sec_num = elfReader.sections.size();
  for (int i = 0; i < sec_num; ++i) {
    ELFIO::section* psec = elfReader.sections[i];
    if (psec->get_type() == SHT_SYMTAB) { // Check section type
      const ELFIO::symbol_section_accessor symbols(elfReader, psec);
      for (unsigned int j = 0; j < symbols.get_symbols_num(); ++j) {
        string name;
        ELFIO::Elf64_Addr value = 0;
        ELFIO::Elf_Xword size;
        unsigned char bind;
        unsigned char type;
        ELFIO::Elf_Half sec_idx;
        unsigned char other;

        // Read symbol properties
        symbols.get_symbol(j, name, value, size, bind, type, sec_idx, other);
        if (!strcmp(name.c_str(), "main")) { return value; }
      }
    }
  }
  return topaddr;
}

vaddr ELFLoader::findEntryAddress() {
  if (!initialized)
    return topaddr;
  else
    return elfReader.get_entry();
}
