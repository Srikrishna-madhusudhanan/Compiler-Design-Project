.globl _start
_start:
    li a0, 42
    li a7, 93
    # ecall raw encoding
    .word 0x00000073
