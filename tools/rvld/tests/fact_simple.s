.globl _start
_start:
    call fact
    li a7, 93
    .word 0x73

fact:
    addi sp, sp, -16
    sd ra, 8(sp)
    sd s0, 0(sp)
    li a0, 42
    ld ra, 8(sp)
    ld s0, 0(sp)
    addi sp, sp, 16
    jr ra
