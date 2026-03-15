  .text
  .globl main

main:
  # --- Prologue ---
  addi sp, sp, -1024
  sw ra, 1020(sp)
  sw s0, 1016(sp)
  addi s0, sp, 1024

  # Line 7: x = ...
  li t0, 11
  sw t0, -32(s0)
  # Line 8: y = ...
  li t0, 20
  sw t0, -36(s0)
  # Line 10: t0 = ... + ...
  lw t0, -32(s0)
  lw t1, -36(s0)
  add t2, t0, t1
  sw t2, -40(s0)
  # Line 10: result = ...
  lw t0, -40(s0)
  sw t0, -44(s0)
  # Line 12: return
  lw a0, -44(s0)
  lw ra, 1020(sp)
  lw s0, 1016(sp)
  addi sp, sp, 1024
  jr ra

  # --- Default Epilogue ---
  lw ra, 1020(sp)
  lw s0, 1016(sp)
  addi sp, sp, 1024
  jr ra

