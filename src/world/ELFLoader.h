/*
 * ELFLoader.h
 *
 *  Created on: May 15, 2013
 *      Author: behrooz
 */

#ifndef ELFLOADER_H_
#define ELFLOADER_H_

#include "extern/elf/elfio.hpp"


class ELFLoader {

private:
	ELFIO::elfio elfReader;
	bool initialized;
public:
	ELFLoader (){
		initialized = false;
	}

	bool loadAndMapELF(File* elfFile, AddressSpace* addSpace ){
		// Load ELF data
		if (!elfReader.load(elfFile->startptr(), elfFile->size())) {
			kcerr << "Can't find or process ELF file ";
			return false;
		} else {
			kcout << "ELF file Loaded Successfully!   ";
			initialized = true;
			if (elfReader.get_class() == ELFCLASS32)
				kcdbg << "Type: " << "32 Bit  ";
			else
				kcdbg << "Type: " << "64 Bit  ";
		}

		//ELF file segments
		int seg_num = elfReader.segments.size();
		for (int i = 0; i < seg_num; ++i) {
			const ELFIO::segment* pseg = elfReader.segments[i];

			//Not loadable Segment
			if (pseg->get_type() != PT_LOAD)
				continue;
			//Filesize > Memsize
			if (pseg->get_file_size() > pseg->get_file_size()) {
				kcerr << " image_load:: p_filesz > p_memsz ";
				//return 0;
			}
			//Empty Segment
			if (!pseg->get_file_size())
				continue;

			//Virtual Address
			vaddr seg_va = pseg->get_virtual_address();
			//Align it
			seg_va = align_down(seg_va,pagesize<1>());
			//Segment Offset in file
			vaddr offset = (vaddr) (pseg->get_data_offset()+elfFile->startptr());
			//Align File Offset
			offset = align_down(offset,pagesize<1>());//Padd offset back
			//Finally this padded offset is our linear address
			vaddr seg_la = offset;
			seg_la = PageManager::vtol(seg_la);
			seg_la = align_down(seg_la, pagesize<1>());//Align Linear address as well

			//How much we padded?
			size_t padding = pseg->get_virtual_address() & 0xfff;
			//kcdbg <<std::hex<<"la:"<<seg_la<<"  aligned:"<<aligned(seg_la,pagesize<1>())<<"   ";
			size_t aligned_size = align_up(pseg->get_file_size()+padding, pagesize<1>());
			//kcout <<std::hex<<"i is:"<<i<<"   aligned:"<<aligned_size<<"  vma:"<<seg_va<<"  lma:"<<seg_la<<"  non-aligned-size:"<<(pseg->get_file_size()+padding);

			//MapPages
			addSpace->mapPages<1>(seg_la,seg_va,aligned_size,PageManager::PageType::Code);


			//TODO zero out BSS
			if (pseg->get_memory_size() > pseg->get_memory_size()) {
				//BSS --> should be zeroed out
			}

			//TODO set permissions
			if (!(pseg->get_flags() & PF_W)) {
				// write
			}

			if (pseg->get_flags() & PF_X) {
				// Executable.
			}
		}
		return true;
	}

	vaddr findMainAddress(){
		if(!initialized)
			return NULL;
		//Find Main Method
		ELFIO::Elf_Half sec_num = elfReader.sections.size();
		for (int i = 0; i < sec_num; ++i) {
			ELFIO::section* psec = elfReader.sections[i];
			// Check section type
			if (psec->get_type() == SHT_SYMTAB) {
				const ELFIO::symbol_section_accessor symbols(elfReader, psec);
				for (unsigned int j = 0; j < symbols.get_symbols_num(); ++j) {
					kstring name;
					ELFIO::Elf64_Addr value;
					ELFIO::Elf_Xword size;
					unsigned char bind;
					unsigned char type;
					ELFIO::Elf_Half section_index;
					unsigned char other;

					// Read symbol properties
					symbols.get_symbol(j, name, value, size, bind, type,
							section_index, other);
					if (!strcmp(name.c_str(), "main"))
					{
						return value;
					}
				}
			}
		}
	}

	vaddr findEntryAddress(){
		if(!initialized)
			return NULL;
		else
			return elfReader.get_entry();
	}

};


#endif /* ELFLOADER_H_ */
