  .text
  .globl main

main:
  # --- Prologue (Frame Size: 96) ---
  addi sp, sp, -96
  sd ra, 88(sp)
  sd s0, 80(sp)
  addi s0, sp, 96

  # Line 21: a$9 = ...
  li t0, 10
  mv t3, t0
  # Line 30: label
L10:
  # Line 23: return
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

foo:
  # --- Prologue (Frame Size: 96) ---
  addi sp, sp, -96
  sd ra, 88(sp)
  sd s0, 80(sp)
  addi s0, sp, 96

  # Move param x$3 from a0 to t3
  mv t3, a0
  # Move param y$4 from a1 to t4
  mv t4, a1
  # Line 5: label
L0:
  # Line 3: if (...) goto L2
  mv t0, t3
  mv t1, t4
  bge t0, t1, L2
  # Line 5: label
L1:
  # Line 4: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 4: x$3 = ...
  mv t0, t3
  mv t3, t0
  # Line 5: goto L0
  j L0
  # Line 5: label
L2:
  # Line 7: i$5 = ...
  li t0, 0
  mv t4, t0
  # Line 14: label
L3:
  # Line 7: if (...) goto L6
  mv t0, t4
  li t1, 5
  bge t0, t1, L6
  # Line 14: label
L4:
  # Line 10: t1 = ... + ...
  mv t0, t4
  mv t1, t4
  add t2, t0, t1
  mv t3, t2
  # Line 11: if (...) goto L9
  mv t0, t3
  li t1, 5
  ble t0, t1, L9
  # Line 14: label
L7:
  # Line 12: goto L6
  j L6
  # Line 14: label
L9:
  # Line 7: t2 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: i$5 = ...
  mv t0, t3
  mv t4, t0
  # Line 14: goto L3
  j L3
  # Line 14: label
L6:
  # Line 16: return
  mv a0, t5
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

