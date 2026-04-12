  .text
  .globl main

main:
  # --- Prologue (Frame Size: 96) ---
  addi sp, sp, -96
  sd ra, 88(sp)
  sd s0, 80(sp)
  addi s0, sp, 96

  # Line 7: x$1 = ...
  li t0, 11
  mv t4, t0
  # Line 8: y$2 = ...
  li t0, 20
  mv t3, t0
  # Line 10: t0 = ... * ...
  mv t0, t3
  mv t1, t4
  mul t2, t0, t1
  mv t3, t2
  # Line 10: t1 = ... - ...
  mv t0, t4
  mv t1, t3
  sub t2, t0, t1
  mv t3, t2
  # Line 10: result$3 = ...
  mv t0, t3
  mv t3, t0
  # Line 12: return
  mv a0, t3
  addi sp, s0, -96
  ld ra, 88(sp)
  ld s0, 80(sp)
  addi sp, sp, 96
  jr ra

  # --- Default Epilogue ---
  addi sp, s0, -96
  ld ra, 88(sp)
  ld s0, 80(sp)
  addi sp, sp, 96
  jr ra

