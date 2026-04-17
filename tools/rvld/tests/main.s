.globl _start
_start:
    call foo
    li a7, 93
    .word 0x00000073
