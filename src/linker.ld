ENTRY(entryPoint)

SECTIONS
{
	. = 0x100000;

	_loadStart = .;

	.boot : {
		*(.mbhdr)
	}

	. += KERNBASE;

	.kernel.init ALIGN(0x1000) : AT(ADDR(.kernel.init) - KERNBASE) {
		*(.kernel.init*)
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

	_loadEnd = . - KERNBASE;

	.bss ALIGN(0x1000) : AT(ADDR(.bss) - KERNBASE) {
		*(COMMON)
		*(.bss*)
		*(.gnu.linkonce.b*)
	}

	_bssEnd = . - KERNBASE;

	/DISCARD/ : {
		*(.comment*)
		*(.eh_frame*)
		*(.gcc_except_table*)
	}

	__KernelBoot    = ADDR(.boot) + KERNBASE;
	__KernelBootEnd = ADDR(.kernel.init) + SIZEOF(.kernel.init);
	__ctors_start   = ADDR(.ctors);
	__ctors_end     = ADDR(.ctors) + SIZEOF(.ctors);
	__KernelCode    = ADDR(.text);
	__KernelCodeEnd = ADDR(.text) + SIZEOF(.text);
	__KernelRO	  	= ADDR(.rodata);
	__KernelRO_End  = ADDR(.rodata) + SIZEOF(.rodata);
	__KernelData		= ADDR(.data);
	__KernelDataEnd = ADDR(.data) + SIZEOF(.data);
	__KernelBss			= ADDR(.bss);
	__KernelBssEnd  = ADDR(.bss) + SIZEOF(.bss);

  PROVIDE (__dso_handle = 0);
  PROVIDE (open = 0);
  PROVIDE (close = 0);
  PROVIDE (_exit = 0);
  PROVIDE (fstat = 0);
  PROVIDE (getpid = 0);
  PROVIDE (isatty = 0);
  PROVIDE (kill = 0);
  PROVIDE (lseek = 0);
  PROVIDE (read = 0);
  PROVIDE (write = 0);
/*  PROVIDE (__cxa_atexit = 0); */
/*  PROVIDE (end = .); */
}

/* make sure that kernel memory is aligned to 1GB */
ASSERT(KERNBASE % 0x40000000 == 0, "KERNBASE: kernel not 1G-aligned");

/* make sure that kernel size is aligned to 2MB */
ASSERT(BOOTMEM % 0x200000 == 0, "BOOTMEM: boot memory not 2M-aligned");

/* use 2 pages for bootstrapping memory management, (see KernelHeap::init in KernelHeap.cc) */
ASSERT(KERNBASE + BOOTMEM >= __KernelDataEnd + 0x2000, "BOOTMEM: no room for bootstrap");

/* use last 2M region in 1G kernel region for device mapping (see deviceAddr in Memory.h) */
ASSERT(BOOTMEM + 0x200000 <= 0x40000000, "BOOTMEM: no room for device mappings");
