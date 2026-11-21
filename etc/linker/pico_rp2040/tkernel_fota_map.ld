ENTRY(Reset_Handler)

INCLUDE fota_layout.ld

MEMORY {
	ROM (rx)	: ORIGIN = __FLASH_APP_START, LENGTH = __FLASH_SLOT_LENGTH
	RAM (xrw)	: ORIGIN = 0x20000000, LENGTH = 256K
}

SECTIONS {
  /DISCARD/ : { *(.boot2) }

	.text : {
    . = ALIGN(4);
		__vector_org = .;
		KEEP (*(.vector))

		*(EXCLUDE_FILE(*libgcc.a: *libc.a:*lib_a-mem*.o *libm.a:) .text*)

		__start = .;
		*(.text)
		*(.text.*)
		*(.rodata)
		*(.rodata.*)
	} >ROM

  ASSERT(__vector_org == ORIGIN(ROM), "FOTA application vector table misplaced")

	.ARM.extab :
	{
		*(.ARM.extab* .gnu.linkonce.armextab.*)
	} > ROM

	__exidx_start = .;
	.ARM.exidx :
	{
		*(.ARM.exidx* .gnu.linkonce.armexidx.*)
	} > ROM
	__exidx_end = .;

	/* Reset_Handler copies .data from this ROM address to RAM a 32-bit word
	   at a time.  ARMv6-M has no unaligned access support, so an unaligned
	   __data_org HardFaults on the first load.  Nothing above forces the
	   location counter to a word boundary, so whether this faults depends
	   on the size of the preceding sections. */
	. = ALIGN(4);
	__data_org = .;
	vector 0x20000000 (NOLOAD) : {
		__vector_start = .;
		*(.data_vector)
		__vector_end = .;
	} >RAM
	.data : AT(__data_org) {
		__data_start = .;
		*(.data)
		*(.data.*)
    *(.time_critical .time_critical.*)
		. = ALIGN(4);
		__data_end = .;
	} >RAM
	ASSERT(LOADADDR(.data) + SIZEOF(.data) <= ORIGIN(ROM) + LENGTH(ROM),
      "FOTA application flash image exceeds its slot")
	.bss ALIGN(4) (NOLOAD) : {
		__bss_start = .;
		PROVIDE (__noinit_start = .);
		*(.noinit)
		PROVIDE (__noinit_end = .);
		*(.bss)
		*(.bss.*)
		*(COMMON)
		. = ALIGN(4);
		__bss_end = .;
		__end = .;
	} >RAM
}
