  .text
  .globl main

  .section .rodata
.LC0:
  .asciz "Sum: %d\n"
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

  # Line 7: sum$4 = ...
  li t0, 0
  mv s1, t0
  # Line 8: i$3 = ...
  li t0, 0
  mv s3, t0
  # Line 10: label
L0:
  # Line 8: if (...) goto L3
  mv t0, s3
  li t1, 10
  bge t0, t1, L3
  # Line 10: label
L1:
  # Line 9: Param
  mv t0, s3
  mv a0, t0
  # Line 9: Call helper
  call helper
  mv t3, a0
  # Line 9: t1 = ... + ...
  mv t0, s1
  mv t1, t3
  add t2, t0, t1
  mv t3, t2
  # Line 8: t2 = ... + ...
  mv t0, s3
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 9: sum$4 = ...
  mv t0, t3
  mv s1, t0
  # Line 8: i$3 = ...
  mv t0, t4
  mv s3, t0
  # Line 10: goto L0
  j L0
  # Line 10: label
L3:
  # Line 11: Param
  la t0, .LC0
  mv a0, t0
  # Line 11: Param
  mv t0, s1
  mv a1, t0
  # Line 11: Call printf
  call printf
  # Line 12: return
  mv a0, s1
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

helper:
  # --- Prologue (Frame Size: 80) ---
  addi sp, sp, -80
  sd ra, 72(sp)
  sd s0, 64(sp)
  addi s0, sp, 80

  # Move param x$1 from a0 to t3
  mv t3, a0
  # Line 2: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 2: return
  mv a0, t3
  addi sp, s0, -80
  ld ra, 72(sp)
  ld s0, 64(sp)
  addi sp, sp, 80
  jr ra

  # --- Default Epilogue ---
  addi sp, s0, -80
  ld ra, 72(sp)
  ld s0, 64(sp)
  addi sp, sp, 80
  jr ra

