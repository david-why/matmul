
    .equ      ROM_START, 0x800000

    .text
    .global   _start
    .type     _start, @function

    _start:

    li        sp, ROM_START

# zero-init bss section:
# clears from _sbss to _ebss
# _sbss and _ebss are defined by linker script (spiflash.ld)
    la        a0, _sbss
    la        a1, _ebss
    loop_init_bss:
    bge       a0, a1, end_init_bss
    sw        zero, 0(a0)
    addi      a0, a0, 4
    j         loop_init_bss
    end_init_bss:

# copy data section from SPI Flash to BRAM:
# copies from _sidata (in flash) to _sdata ... _edata (in BRAM)
# _sidata, _sdata and _edata are defined by linker script (spiflash.ld)
    la        a0, _sidata
    la        a1, _sdata
    la        a2, _edata
    loop_init_data:
    bge       a1, a2, end_init_data
    lw        a3, 0(a0)
    sw        a3, 0(a1)
    addi      a0, a0, 4
    addi      a1, a1, 4
    j         loop_init_data
    end_init_data:

    la        a0, jump_ram
    la        a1, _j_offset             # if runs on PRO device, have enough RAM
    sub       a0, a0, a1                # copy text and data sections to RAM
    jr        0(a0)                     # then jump to RAM
    jump_ram:

    call      main
    tail      exit
