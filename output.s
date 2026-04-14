  .text
  .globl main

  .section .rodata
.LC1:
  .asciz "Next value: %d\n"
.LC0:
  .asciz "Value allocated: %d\n"
  .text

main:
  # --- Prologue (Frame Size: 96) ---
  addi sp, sp, -96
  sd ra, 88(sp)
  sd s0, 80(sp)
  sd s1, 72(sp)
  sd s2, 64(sp)
  sd s3, 56(sp)
  addi s0, sp, 96

  # Line 5: Param
  li t0, 8
  mv a0, t0
  # Line 5: Call malloc
  call malloc
  mv s3, a0
  # Line 6: Store Array/Pointer
  mv t0, s3
  li t1, 0
  slli t1, t1, 2
  li t2, 42
  add t0, t0, t1
  sw t2, 0(t0)
  # Line 7: t1 = ... + ...
  mv t0, s3
  li t1, 4
  add t2, t0, t1
  mv s2, t2
  # Line 7: Store Array/Pointer
  mv t0, s2
  li t1, 0
  slli t1, t1, 2
  li t2, 99
  add t0, t0, t1
  sw t2, 0(t0)
  # Line 9: Load Array/Pointer
  mv t0, s3
  li t1, 0
  slli t1, t1, 2
  add t2, t0, t1
  lw t2, 0(t2)
  mv t3, t2
  # Line 9: Param
  la t0, .LC0
  mv a0, t0
  # Line 9: Param
  mv t0, t3
  mv a1, t0
  # Line 9: Call printf
  call printf
  # Line 10: Load Array/Pointer
  mv t0, s2
  li t1, 0
  slli t1, t1, 2
  add t2, t0, t1
  lw t2, 0(t2)
  mv t3, t2
  # Line 10: Param
  la t0, .LC1
  mv a0, t0
  # Line 10: Param
  mv t0, t3
  mv a1, t0
  # Line 10: Call printf
  call printf
  # Line 13: Load Array/Pointer
  mv t0, s3
  li t1, 0
  slli t1, t1, 2
  add t2, t0, t1
  lw t2, 0(t2)
  mv t3, t2
  # Line 5: p$3 = ...
  mv t0, s3
  mv t4, t0
  # Line 13: if (...) goto L2
  mv t0, t3
  li t1, 42
  bne t0, t1, L2
  # Line 17: label
L0:
  # Line 14: return
  li a0, 0
  addi sp, s0, -96
  ld s1, 72(sp)
  ld s2, 64(sp)
  ld s3, 56(sp)
  ld ra, 88(sp)
  ld s0, 80(sp)
  addi sp, sp, 96
  jr ra
  # Line 17: label
L2:
  # Line 17: Param
  mv t0, t4
  mv a0, t0
  # Line 17: Call free
  call free
  # Line 18: return
  li a0, 1
  addi sp, s0, -96
  ld s1, 72(sp)
  ld s2, 64(sp)
  ld s3, 56(sp)
  ld ra, 88(sp)
  ld s0, 80(sp)
  addi sp, sp, 96
  jr ra

  # --- Default Epilogue ---
  addi sp, s0, -96
  ld s1, 72(sp)
  ld s2, 64(sp)
  ld s3, 56(sp)
  ld ra, 88(sp)
  ld s0, 80(sp)
  addi sp, sp, 96
  jr ra

