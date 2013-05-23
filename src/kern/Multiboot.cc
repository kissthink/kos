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
#include "extern/multiboot/multiboot2.h"
#include "kern/FrameManager.h"
#include "kern/Multiboot.h"
#include "kern/Kernel.h"
#include "world/File.h"

#define FORALLTAGS(tag,start,end) \
  for (multiboot_tag* tag = (multiboot_tag*)(start); \
  vaddr(tag) < (end) && tag->type != MULTIBOOT_TAG_TYPE_END; \
  tag = (multiboot_tag*)(vaddr(tag) + ((tag->size + 7) & ~7)))

vaddr Multiboot::init( mword magic, vaddr mbi ) {
  KASSERT1(magic == MULTIBOOT2_BOOTLOADER_MAGIC, magic);
  KASSERT1( !(mbi & 7), FmtHex(mbi) );
  // mbiEnd = mbi + ((multiboot_header_tag*)mbi)->size;
  mbiStart = mbi + sizeof(multiboot_header_tag);
  mbiEnd = mbi + *(uint32_t*)mbi;
  return mbiEnd;
}

void Multiboot::initDebug( bool msg ) {
  FORALLTAGS(tag,mbiStart,mbiEnd) {
    if (tag->type == MULTIBOOT_TAG_TYPE_CMDLINE) {
      DBG::init( ((multiboot_tag_string*)tag)->string, msg );
    }
  }
}

// cf. 'multiboot_mmap_entry' in extern/multiboot/multiboot2.h
static const char* memtype[] __section(".boot.data") = {
  "unknown", "free", "resv", "acpi", "nvs", "bad"
};

void Multiboot::parseAll(laddr& modStart, laddr& modEnd) {
  modStart = topaddr;
  modEnd = 0;
  FORALLTAGS(tag,mbiStart,mbiEnd) {
    switch (tag->type) {
    case MULTIBOOT_TAG_TYPE_CMDLINE:
      DBG::outln(DBG::Boot, "command line: ", ((multiboot_tag_string*)tag)->string);
      break;
    case MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME:
      DBG::outln(DBG::Boot, "boot loader: ", ((multiboot_tag_string*)tag)->string);
      break;
    case MULTIBOOT_TAG_TYPE_MODULE: {
      multiboot_tag_module* tm = (multiboot_tag_module*)tag;
      DBG::outln(DBG::Boot, "module at ", FmtHex(tm->mod_start),
        '-', FmtHex(tm->mod_end), ": ", tm->cmdline);
      if (tm->mod_start < modStart) modStart = tm->mod_start;
      if (tm->mod_end > modEnd) modEnd = tm->mod_end;
    } break;
    case MULTIBOOT_TAG_TYPE_BASIC_MEMINFO: {
      multiboot_tag_basic_meminfo* tm = (multiboot_tag_basic_meminfo*)tag;
      DBG::outln(DBG::Boot, "memory low: ", tm->mem_lower, " / high: ", tm->mem_upper);
    } break;
    case MULTIBOOT_TAG_TYPE_BOOTDEV: {
      multiboot_tag_bootdev* tb = (multiboot_tag_bootdev*)tag;
      DBG::outln(DBG::Boot, "boot device: ", FmtHex(tb->biosdev), ',', tb->slice, ',', tb->part);
    } break;
    case MULTIBOOT_TAG_TYPE_MMAP: {
      multiboot_tag_mmap* tmm = (multiboot_tag_mmap*)tag;
      vaddr end = vaddr(tmm) + tmm->size;
      DBG::out(DBG::Boot, "mmap:");
      for (mword mm = (mword)tmm->entries; mm < end; mm += tmm->entry_size ) {
        multiboot_memory_map_t* mmap = (multiboot_memory_map_t*)mm;
        KASSERT1(mmap->type <= MULTIBOOT_MEMORY_BADRAM, mmap->type);
        DBG::out(DBG::Boot, ' ', FmtHex(mmap->addr), '/', FmtHex(mmap->len), '/', memtype[mmap->type]);
      }
      DBG::out(DBG::Boot, kendl);
    } break;
    case MULTIBOOT_TAG_TYPE_FRAMEBUFFER:
      DBG::outln(DBG::Boot, "framebuffer info present");
      break;
    case MULTIBOOT_TAG_TYPE_VBE:
      DBG::outln(DBG::Boot, "vbe info present");
      break;
    case MULTIBOOT_TAG_TYPE_ELF_SECTIONS:
      DBG::outln(DBG::Boot, "elf section info present");
      break;
    case MULTIBOOT_TAG_TYPE_APM:
      DBG::outln(DBG::Boot, "APM info present");
      break;
    case MULTIBOOT_TAG_TYPE_EFI32:
      DBG::outln(DBG::Boot, "efi32 info present");
      break;
    case MULTIBOOT_TAG_TYPE_EFI64:
      DBG::outln(DBG::Boot, "efi64 info present");
      break;
    case MULTIBOOT_TAG_TYPE_SMBIOS:
      DBG::outln(DBG::Boot, "smbios info present");
      break;
    case MULTIBOOT_TAG_TYPE_ACPI_OLD: {
      multiboot_tag_old_acpi* ta = (multiboot_tag_old_acpi*)tag;
      DBG::outln(DBG::Boot, "acpi/old: ", FmtHex(ta->rsdp), '/', FmtHex(ta->size));
    } break;
    case MULTIBOOT_TAG_TYPE_ACPI_NEW: {
      multiboot_tag_new_acpi* ta = (multiboot_tag_new_acpi*)tag;
      DBG::outln(DBG::Boot, "acpi/new: ", FmtHex(ta->rsdp), '/', FmtHex(ta->size));
    } break;
    case MULTIBOOT_TAG_TYPE_NETWORK:
      DBG::outln(DBG::Boot, "network info present");
      break;
    default:
      DBG::outln(DBG::Boot, "unknown tag: ", tag->type);
    }
  }
}

vaddr Multiboot::getRSDP() {
  FORALLTAGS(tag,mbiStart,mbiEnd) {
    switch (tag->type) {
    case MULTIBOOT_TAG_TYPE_ACPI_OLD:
      return vaddr(((multiboot_tag_old_acpi*)tag)->rsdp);
    case MULTIBOOT_TAG_TYPE_ACPI_NEW:
      return vaddr(((multiboot_tag_new_acpi*)tag)->rsdp);
    }
  }
  ABORT1("RSDP not found");
  return 0;
}

void Multiboot::getMemory(FrameManager& fm) {
  FORALLTAGS(tag,mbiStart,mbiEnd) {
    if (tag->type == MULTIBOOT_TAG_TYPE_MMAP) {
      multiboot_tag_mmap* tm = (multiboot_tag_mmap*)tag;
      vaddr end = vaddr(tm) + tm->size;
      for (vaddr mm = (laddr)tm->entries; mm < end; mm += tm->entry_size ) {
        multiboot_memory_map_t* mmap = (multiboot_memory_map_t*)mm;
        if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE) {
          laddr addr = align_up(laddr(mmap->addr), pagesize<1>() );
          size_t len = align_down(size_t(mmap->len - (addr - mmap->addr)), pagesize<1>() );
          if ( len > 0 ) fm.release(addr, len);
        }
      }
    }
  }
}

void Multiboot::getModules(FrameManager& fm, size_t alignment) {
  FORALLTAGS(tag,mbiStart,mbiEnd) {
    if (tag->type == MULTIBOOT_TAG_TYPE_MODULE) {
      multiboot_tag_module* tm = (multiboot_tag_module*)tag;
      laddr start = align_down(laddr(tm->mod_start), alignment);
      laddr end = align_up(laddr(tm->mod_end), alignment);
      fm.reserve(start, end - start);
    }
  }
}

void Multiboot::readModules(vaddr disp, FrameManager& fm, size_t alignment) {
  FORALLTAGS(tag,mbiStart,mbiEnd) {
    if (tag->type == MULTIBOOT_TAG_TYPE_MODULE) {
      multiboot_tag_module* tm = (multiboot_tag_module*)tag;
      kstring cmd = tm->cmdline;
      kstring name = cmd.substr(0, cmd.find_first_of(' '));
      File* file = new File(tm->mod_start + disp, tm->mod_end - tm->mod_start);
      kernelFS.insert( {name, file} );
      laddr start = align_down(laddr(tm->mod_start), alignment);
      laddr end = align_up(laddr(tm->mod_end), alignment);
      fm.release(start, end - start);
    }
  }
}
