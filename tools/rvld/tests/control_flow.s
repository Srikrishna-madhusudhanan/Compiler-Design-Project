  .text
  .globl main

main:
  # --- Prologue (Frame Size: 96) ---
  addi sp, sp, -96
  sd ra, 88(sp)
  sd s0, 80(sp)
  addi s0, sp, 96

  # Line 3: i$3 = ...
  li t0, 0
  mv t5, t0
  # Line 9: label
L0:
  # Line 5: if (...) goto L2
  mv t0, t5
  li t1, 5
  bge t0, t1, L2
  # Line 9: label
L1:
  # Line 6: t0 = ... + ...
  mv t0, t5
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 6: i$3 = ...
  mv t0, t3
  mv t5, t0
  # Line 7: if (...) goto L5
  mv t0, t3
  li t1, 3
  bne t0, t1, L5
  # Line 8: label
L3:
  # Line 7: goto L0
  j L0
  # Line 8: label
L5:
  # Line 8: t1 = ... + ...
  mv t0, t5
  li t1, 10
  add t2, t0, t1
  mv t3, t2
  # Line 8: i$3 = ...
  mv t0, t3
  mv t5, t0
  # Line 9: goto L0
  j L0
  # Line 9: label
L2:
  # Line 12: j$4 = ...
  li t0, 0
  mv t4, t0
  # Line 15: label
L6:
  # Line 15: label
L13:
  # Line 13: if (...) goto L15
  mv t0, t4
  li t1, 2
  bne t0, t1, L15
  # Line 14: label
L14:
  # Line 13: goto L16
  j L16
  # Line 14: label
L15:
  # Line 14: t2 = ... + ...
  mv t0, t5
  mv t1, t4
  add t2, t0, t1
  mv t3, t2
  # Line 14: i$3 = ...
  mv t0, t3
  mv t5, t0
  # Line 15: label
L16:
  # Line 12: t3 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 12: j$4 = ...
  mv t0, t3
  mv t4, t0
  # Line 15: label
L17:
  # Line 13: if (...) goto L19
  mv t0, t4
  li t1, 2
  bne t0, t1, L19
  # Line 14: label
L18:
  # Line 13: goto L20
  j L20
  # Line 14: label
L19:
  # Line 14: t2 = ... + ...
  mv t0, t5
  mv t1, t4
  add t2, t0, t1
  mv t3, t2
  # Line 14: i$3 = ...
  mv t0, t3
  mv t5, t0
  # Line 15: label
L20:
  # Line 12: t3 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 12: j$4 = ...
  mv t0, t3
  mv t4, t0
  # Line 15: label
L21:
  # Line 13: if (...) goto L23
  mv t0, t4
  li t1, 2
  bne t0, t1, L23
  # Line 14: label
L22:
  # Line 13: goto L24
  j L24
  # Line 14: label
L23:
  # Line 14: t2 = ... + ...
  mv t0, t5
  mv t1, t4
  add t2, t0, t1
  mv t3, t2
  # Line 14: i$3 = ...
  mv t0, t3
  mv t5, t0
  # Line 15: label
L24:
  # Line 12: t3 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 12: j$4 = ...
  mv t0, t3
  mv t4, t0
  # Line 15: label
L25:
  # Line 13: if (...) goto L27
  mv t0, t4
  li t1, 2
  bne t0, t1, L27
  # Line 14: label
L26:
  # Line 13: goto L28
  j L28
  # Line 14: label
L27:
  # Line 14: t2 = ... + ...
  mv t0, t5
  mv t1, t4
  add t2, t0, t1
  mv t3, t2
  # Line 14: i$3 = ...
  mv t0, t3
  mv t5, t0
  # Line 15: label
L28:
  # Line 12: t3 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 12: j$4 = ...
  mv t0, t3
  mv t4, t0
  # Line 15: label
L29:
  # Line 13: if (...) goto L31
  mv t0, t4
  li t1, 2
  bne t0, t1, L31
  # Line 14: label
L30:
  # Line 13: goto L32
  j L32
  # Line 14: label
L31:
  # Line 14: t2 = ... + ...
  mv t0, t5
  mv t1, t4
  add t2, t0, t1
  mv t3, t2
  # Line 14: i$3 = ...
  mv t0, t3
  mv t5, t0
  # Line 15: label
L32:
  # Line 12: t3 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 12: j$4 = ...
  mv t0, t3
  mv t4, t0
  # Line 15: label
L9:
  # Line 17: t4 = ... + ...
  mv t0, t5
  mv t1, t4
  add t2, t0, t1
  mv t3, t2
  # Line 17: return
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

