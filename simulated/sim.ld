
MEMORY {
    FLASH (rx)  : ORIGIN = 0x00800000, LENGTH = 0x200000     /* 2 MB in flash */
    RAM   (rwx) : ORIGIN = 0x00000000, LENGTH = 0x800000     /* 8MB in RAM   */ 
}

SECTIONS {

    /* 
     * These are the text and data sections, It is one task of the startup 
     * (crtrv.S) to copy these two sections from FLASH to RAM. 
     */
    .text : {
        . = ALIGN(4);
        _sidata = .;
        crtrv.o(.text)  /* c runtime initialization (code) */

        /* 
        * I do not understand why, but if I do not put that here, I got
        * an overlapping sections error with some programs (for instance pi.c
        * or C++ programs) 
        */
        *(.eh_frame)
        *(.eh_frame_hdr)
        *(.init_array)
        *(.gcc_except_table*)	

        *(.text)                /* .text sections (code) */
        *(.text*)               /* .text* sections (code) */
        . = ALIGN(4);
        *(.rodata)              /* .rodata sections (constants, strings, etc.) */
        *(.rodata*)             /* .rodata* sections (constants, strings, etc.) */
        *(.srodata)             /* .rodata sections (constants, strings, etc.) */
        *(.srodata*)            /* .rodata* sections (constants, strings, etc.) */
        _etext = .;             /* define a global symbol at end of code */
    } > FLASH AT> RAM

    .data : {
        . = ALIGN(4);
        _ram_start = .;    /* create a global symbol at ram start (e.g., for garbage collector) */

        /* Initialized data */
        *(.data)                     
        *(.data*)          
        *(.sdata)          
        *(.sdata*)
    
        . = ALIGN(4);
        _eidata = .;        /* define a global symbol at data end; used by startup code in order to initialise the .data section in RAM */
    } > FLASH AT> RAM

    _sdata = LOADADDR(.text);
    _edata = _sdata + _eidata - _sidata;
    /* _j_offset = _sidata - _sdata; */
    _j_offset = 0;

    /* Uninitialized data section */
    .bss : {
        . = ALIGN(4);
        _sbss = .;         /* define a global symbol at bss start; used by startup code */
        *(.bss)
        *(.bss*)
        *(.sbss)
        *(.sbss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;         /* define a global symbol at bss end; used by startup code */
    } > RAM

    /* this is to define the start of the heap, and make sure we have a minimum size */
    .heap : {
        . = ALIGN(4);
        _heap_start = .;    /* define a global symbol at heap start */
      	_end = .;           /* as expected by syscalls.c            */
    } > RAM
}