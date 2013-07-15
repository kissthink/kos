ENTRY(entryPoint)

SECTIONS
{
	. = 0x100000;

	.bootstrap : {
		*(.multiboot.header)
	}

	. += KERNBASE;

	.boot.text ALIGN(0x1000) : AT(ADDR(.boot.text) - KERNBASE) {
		*(.boot.text*)
	}

	.boot.data ALIGN(0x1000) : AT(ADDR(.boot.data) - KERNBASE) {
		*(.boot.data*)
	}

	.ctors ALIGN(0x1000) : AT(ADDR(.ctors) - KERNBASE) {
		*(.ctors*)
	}

	.text ALIGN(0x200000) : AT(ADDR(.text) - KERNBASE) {
		*(.text*)
		*(.gnu.linkonce.text*)
	}

	.rodata ALIGN(0x1000) : AT(ADDR(.rodata) - KERNBASE) {
		*(.rodata*)
		*(.gnu.linkonce.rodata*)
	}

	.data ALIGN(0x200000) : AT(ADDR(.data) - KERNBASE) {
		*(.data*)
		*(.gnu.linkonce.data*)
	}

	.bss ALIGN(0x1000) : AT(ADDR(.bss) - KERNBASE) {
		*(COMMON)
		*(.bss*)
		*(.gnu.linkonce.b*)
	}

	.boot.heap ALIGN(0x1000) : AT(ADDR(.boot.heap) - KERNBASE) {
		. += 0x10000;
	}

	/DISCARD/ : {
		*(.comment*)
		*(.eh_frame*)
		*(.gcc_except_table*)
	}

	__Boot          = ADDR(.bootstrap) + KERNBASE;
	__BootEnd       = ADDR(.boot.data) + SIZEOF(.boot.data);
	__ctors_start   = ADDR(.ctors);
	__ctors_end     = ADDR(.ctors) + SIZEOF(.ctors);
	__KernelCode    = ADDR(.text);
	__KernelCodeEnd = ADDR(.text) + SIZEOF(.text);
	__KernelRO	= ADDR(.rodata);
	__KernelRO_End  = ADDR(.rodata) + SIZEOF(.rodata);
	__KernelData	= ADDR(.data);
	__KernelDataEnd = ADDR(.data) + SIZEOF(.data);
	__KernelBss	= ADDR(.bss);
	__KernelBssEnd  = ADDR(.bss) + SIZEOF(.bss);
	__BootHeap	= ADDR(.boot.heap);
	__BootHeapEnd   = ADDR(.boot.heap) + SIZEOF(.boot.heap);
}

/* make sure that kernel memory is aligned to 1GB */
ASSERT(KERNBASE % 0x40000000 == 0, "KERNBASE: kernel not 1G-aligned");

/* make sure that kernel size is aligned to 2MB */
ASSERT(BOOTMEM % 0x200000 == 0, "BOOTMEM: boot memory not 2M-aligned");

/* make sure that kernel memory is large enough */
ASSERT(BOOTMEM >= __BootHeapEnd - KERNBASE, "BOOTMEM: not enough memory");
