.globl _start

.data
array:
    .dword 5
    .dword 2
    .dword 8
    .dword 1
    .dword 9

.text
_start:
    la a0, array
    li a1, 5
    call bubble_sort
    
    # Verify first element is 1 (sorted)
    la a0, array
    ld a0, 0(a0)
    li a7, 93
    .word 0x00000073 # ecall

bubble_sort:
    # a0 = array, a1 = n
    addi t0, a1, -1 # n-1
outer_loop:
    ble t0, zero, sort_done
    li t1, 0 # i = 0
    mv t2, a0 # current ptr
inner_loop:
    bge t1, t0, inner_done
    ld t3, 0(t2)
    ld t4, 8(t2)
    ble t3, t4, no_swap
    sd t4, 0(t2)
    sd t3, 8(t2)
no_swap:
    addi t1, t1, 1
    addi t2, t2, 8
    j inner_loop
inner_done:
    addi t0, t0, -1
    j outer_loop
sort_done:
    jr ra
