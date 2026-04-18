  .text
  .globl main

main:
  # --- Prologue (Frame Size: 112) ---
  addi sp, sp, -112
  sd ra, 104(sp)
  sd s0, 96(sp)
  addi s0, sp, 112

  # Line 9: label
L0:
  # Line 6: if (...) goto L2
  mv t0, t4
  li t1, 10
  bge t0, t1, L2
  # Line 9: label
L1:
  # Line 7: t0 = ... + ...
  mv t0, t3
  mv t1, t4
  add t2, t0, t1
  mv t3, t2
  # Line 8: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 7: sum = ...
  mv t0, t3
  mv t3, t0
  # Line 8: i = ...
  mv t0, t4
  mv t4, t0
  # Line 9: goto L0
  j L0
  # Line 9: label
L2:
  # Line 11: return
  mv a0, t3
  j .L_exit_main

.L_exit_main:
  # --- Epilogue ---
  addi sp, s0, -112
  ld ra, 104(sp)
  ld s0, 96(sp)
  addi sp, sp, 112
  jr ra

