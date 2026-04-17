.globl _start
_start:
    call main
    li a7, 93
    .word 0x73 # ecall
