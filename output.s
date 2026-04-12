  .text
  .globl main

main:
  # --- Prologue (Frame Size: 96) ---
  addi sp, sp, -96
  sd ra, 88(sp)
  sd s0, 80(sp)
  addi s0, sp, 96

  # Line 2: sum$1 = ...
  li t0, 0
  mv t4, t0
  # Line 3: i$2 = ...
  li t0, 0
  mv t5, t0
  # Line 12: label
L0:
  # Line 12: label
L6:
  # Line 6: t0 = ... % ...
  mv t0, t5
  li t1, 2
  rem t2, t0, t1
  mv t3, t2
  # Line 6: if (...) goto L8
  mv t0, t3
  li t1, 0
  bne t0, t1, L8
  # Line 10: label
L7:
  # Line 7: t1 = ... + ...
  mv t0, t4
  mv t1, t5
  add t2, t0, t1
  mv t3, t2
  # Line 7: sum$1 = ...
  mv t0, t3
  mv t4, t0
  # Line 10: goto L9
  j L9
  # Line 10: label
L8:
  # Line 9: t2 = ... + ...
  mv t0, t5
  mv t1, t5
  add t2, t0, t1
  mv t3, t2
  # Line 9: t3 = ... + ...
  mv t0, t4
  mv t1, t3
  add t2, t0, t1
  mv t3, t2
  # Line 9: sum$1 = ...
  mv t0, t3
  mv t4, t0
  # Line 10: label
L9:
  # Line 11: t4 = ... + ...
  mv t0, t5
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 11: i$2 = ...
  mv t0, t3
  mv t5, t0
  # Line 12: label
L10:
  # Line 6: t0 = ... % ...
  mv t0, t5
  li t1, 2
  rem t2, t0, t1
  mv t3, t2
  # Line 6: if (...) goto L12
  mv t0, t3
  li t1, 0
  bne t0, t1, L12
  # Line 10: label
L11:
  # Line 7: t1 = ... + ...
  mv t0, t4
  mv t1, t5
  add t2, t0, t1
  mv t3, t2
  # Line 7: sum$1 = ...
  mv t0, t3
  mv t4, t0
  # Line 10: goto L13
  j L13
  # Line 10: label
L12:
  # Line 9: t2 = ... + ...
  mv t0, t5
  mv t1, t5
  add t2, t0, t1
  mv t3, t2
  # Line 9: t3 = ... + ...
  mv t0, t4
  mv t1, t3
  add t2, t0, t1
  mv t3, t2
  # Line 9: sum$1 = ...
  mv t0, t3
  mv t4, t0
  # Line 10: label
L13:
  # Line 11: t4 = ... + ...
  mv t0, t5
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 11: i$2 = ...
  mv t0, t3
  mv t5, t0
  # Line 12: label
L14:
  # Line 6: t0 = ... % ...
  mv t0, t5
  li t1, 2
  rem t2, t0, t1
  mv t3, t2
  # Line 6: if (...) goto L16
  mv t0, t3
  li t1, 0
  bne t0, t1, L16
  # Line 10: label
L15:
  # Line 7: t1 = ... + ...
  mv t0, t4
  mv t1, t5
  add t2, t0, t1
  mv t3, t2
  # Line 7: sum$1 = ...
  mv t0, t3
  mv t4, t0
  # Line 10: goto L17
  j L17
  # Line 10: label
L16:
  # Line 9: t2 = ... + ...
  mv t0, t5
  mv t1, t5
  add t2, t0, t1
  mv t3, t2
  # Line 9: t3 = ... + ...
  mv t0, t4
  mv t1, t3
  add t2, t0, t1
  mv t3, t2
  # Line 9: sum$1 = ...
  mv t0, t3
  mv t4, t0
  # Line 10: label
L17:
  # Line 11: t4 = ... + ...
  mv t0, t5
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 11: i$2 = ...
  mv t0, t3
  mv t5, t0
  # Line 12: label
L18:
  # Line 6: t0 = ... % ...
  mv t0, t5
  li t1, 2
  rem t2, t0, t1
  mv t3, t2
  # Line 6: if (...) goto L20
  mv t0, t3
  li t1, 0
  bne t0, t1, L20
  # Line 10: label
L19:
  # Line 7: t1 = ... + ...
  mv t0, t4
  mv t1, t5
  add t2, t0, t1
  mv t3, t2
  # Line 7: sum$1 = ...
  mv t0, t3
  mv t4, t0
  # Line 10: goto L21
  j L21
  # Line 10: label
L20:
  # Line 9: t2 = ... + ...
  mv t0, t5
  mv t1, t5
  add t2, t0, t1
  mv t3, t2
  # Line 9: t3 = ... + ...
  mv t0, t4
  mv t1, t3
  add t2, t0, t1
  mv t3, t2
  # Line 9: sum$1 = ...
  mv t0, t3
  mv t4, t0
  # Line 10: label
L21:
  # Line 11: t4 = ... + ...
  mv t0, t5
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 11: i$2 = ...
  mv t0, t3
  mv t5, t0
  # Line 12: label
L22:
  # Line 6: t0 = ... % ...
  mv t0, t5
  li t1, 2
  rem t2, t0, t1
  mv t3, t2
  # Line 6: if (...) goto L24
  mv t0, t3
  li t1, 0
  bne t0, t1, L24
  # Line 10: label
L23:
  # Line 7: t1 = ... + ...
  mv t0, t4
  mv t1, t5
  add t2, t0, t1
  mv t3, t2
  # Line 7: sum$1 = ...
  mv t0, t3
  mv t4, t0
  # Line 10: goto L25
  j L25
  # Line 10: label
L24:
  # Line 9: t2 = ... + ...
  mv t0, t5
  mv t1, t5
  add t2, t0, t1
  mv t3, t2
  # Line 9: t3 = ... + ...
  mv t0, t4
  mv t1, t3
  add t2, t0, t1
  mv t3, t2
  # Line 9: sum$1 = ...
  mv t0, t3
  mv t4, t0
  # Line 10: label
L25:
  # Line 11: t4 = ... + ...
  mv t0, t5
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 11: i$2 = ...
  mv t0, t3
  mv t5, t0
  # Line 12: label
L26:
  # Line 6: t0 = ... % ...
  mv t0, t5
  li t1, 2
  rem t2, t0, t1
  mv t3, t2
  # Line 6: if (...) goto L28
  mv t0, t3
  li t1, 0
  bne t0, t1, L28
  # Line 10: label
L27:
  # Line 7: t1 = ... + ...
  mv t0, t4
  mv t1, t5
  add t2, t0, t1
  mv t3, t2
  # Line 7: sum$1 = ...
  mv t0, t3
  mv t4, t0
  # Line 10: goto L29
  j L29
  # Line 10: label
L28:
  # Line 9: t2 = ... + ...
  mv t0, t5
  mv t1, t5
  add t2, t0, t1
  mv t3, t2
  # Line 9: t3 = ... + ...
  mv t0, t4
  mv t1, t3
  add t2, t0, t1
  mv t3, t2
  # Line 9: sum$1 = ...
  mv t0, t3
  mv t4, t0
  # Line 10: label
L29:
  # Line 11: t4 = ... + ...
  mv t0, t5
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 11: i$2 = ...
  mv t0, t3
  mv t5, t0
  # Line 12: label
L2:
  # Line 14: return
  mv a0, t4
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

