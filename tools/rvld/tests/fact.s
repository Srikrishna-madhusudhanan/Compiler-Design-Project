.globl _start
_start:
    li a0, 5
    call fact
    # result in a0 (120)
    li a7, 93
    .word 0x00000073 # ecall

fact:
    addi sp, sp, -16
    sd ra, 8(sp)
    sd s0, 0(sp)
    
    li t0, 1
    bgt a0, t0, recursive
    li a0, 1
    j done

recursive:
    mv s0, a0
    addi a0, a0, -1
    call fact
    mul a0, a0, s0

done:
    ld ra, 8(sp)
    ld s0, 0(sp)
    addi sp, sp, 16
    jr ra
