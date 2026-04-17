.globl main
main:

    la a0, my_global
    ld a0, 0(a0)
    li a7, 93
    .word 0x73
