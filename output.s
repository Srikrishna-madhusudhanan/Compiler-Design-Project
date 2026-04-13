  .text
  .globl main

  .section .rodata
.LC0:
  .asciz "total=%d"
  .text

main:
  # --- Prologue (Frame Size: 96) ---
  addi sp, sp, -96
  sd ra, 88(sp)
  sd s0, 80(sp)
  addi s0, sp, 96

  # Line 2: total$1 = ...
  li t0, 0
  mv t3, t0
  # Line 3: i$2 = ...
  li t0, 0
  mv t4, t0
  # Line 8: label
L0:
  # Line 8: label
L3:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L4:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L5:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L6:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L7:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L8:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L9:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L10:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L11:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L12:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L13:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L14:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L15:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L16:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L17:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L18:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L19:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L20:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L21:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L22:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L23:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L24:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L25:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L26:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L27:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L28:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L29:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L30:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L31:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L32:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L33:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L34:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L35:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L36:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L37:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L38:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L39:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L40:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L41:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L42:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L43:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L44:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L45:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L46:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L47:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L48:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L49:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L50:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L51:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L52:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L53:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L54:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L55:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L56:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L57:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L58:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L59:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L60:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L61:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L62:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L63:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L64:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L65:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L66:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L67:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L68:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L69:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L70:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L71:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L72:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L73:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L74:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L75:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L76:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L77:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L78:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L79:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L80:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L81:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L82:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L83:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L84:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L85:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L86:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L87:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L88:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L89:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L90:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L91:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L92:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L93:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L94:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L95:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L96:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L97:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L98:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L99:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L100:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L101:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L102:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L103:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L104:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L105:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L106:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L107:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L108:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L109:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L110:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L111:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L112:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L113:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L114:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L115:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L116:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L117:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L118:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L119:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L120:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L121:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L122:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L123:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L124:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L125:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L126:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L127:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L128:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L129:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L130:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L131:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L132:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L133:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L134:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L135:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L136:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L137:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L138:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L139:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L140:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L141:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L142:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L143:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L144:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L145:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L146:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L147:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L148:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L149:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L150:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L151:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L152:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L153:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L154:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L155:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L156:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L157:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L158:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L159:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L160:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L161:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L162:
  # Line 6: t0 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 7: t1 = ... + ...
  mv t0, t4
  li t1, 1
  add t2, t0, t1
  mv t4, t2
  # Line 6: total$1 = ...
  mv t0, t3
  mv t3, t0
  # Line 7: i$2 = ...
  mv t0, t4
  mv t4, t0
  # Line 8: label
L2:
  # Line 10: Param
  la t0, .LC0
  mv a0, t0
  # Line 10: Param
  mv t0, t3
  mv a1, t0
  # Line 10: Call printf
  call printf
  # Line 11: return
  li a0, 0
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

