  .text
  .globl main

main:
  # --- Prologue (Frame Size: 176) ---
  addi sp, sp, -176
  sd ra, 168(sp)
  sd s0, 160(sp)
  sd s1, 152(sp)
  sd s2, 144(sp)
  sd s3, 136(sp)
  sd s4, 128(sp)
  sd s5, 120(sp)
  addi s0, sp, 176

  # Line 4: Store Array/Pointer
  addi t0, s0, -136
  li t1, 0
  slli t1, t1, 2
  li t2, 64
  add t0, t0, t1
  sw t2, 0(t0)
  # Line 4: Store Array/Pointer
  addi t0, s0, -136
  li t1, 1
  slli t1, t1, 2
  li t2, 34
  add t0, t0, t1
  sw t2, 0(t0)
  # Line 4: Store Array/Pointer
  addi t0, s0, -136
  li t1, 2
  slli t1, t1, 2
  li t2, 25
  add t0, t0, t1
  sw t2, 0(t0)
  # Line 4: Store Array/Pointer
  addi t0, s0, -136
  li t1, 3
  slli t1, t1, 2
  li t2, 12
  add t0, t0, t1
  sw t2, 0(t0)
  # Line 4: Store Array/Pointer
  addi t0, s0, -136
  li t1, 4
  slli t1, t1, 2
  li t2, 22
  add t0, t0, t1
  sw t2, 0(t0)
  # Line 5: Store Array/Pointer
  addi t0, s0, -136
  li t1, 5
  slli t1, t1, 2
  li t2, 11
  add t0, t0, t1
  sw t2, 0(t0)
  # Line 5: Store Array/Pointer
  addi t0, s0, -136
  li t1, 6
  slli t1, t1, 2
  li t2, 90
  add t0, t0, t1
  sw t2, 0(t0)
  # Line 5: Store Array/Pointer
  addi t0, s0, -136
  li t1, 7
  slli t1, t1, 2
  li t2, 88
  add t0, t0, t1
  sw t2, 0(t0)
  # Line 5: Store Array/Pointer
  addi t0, s0, -136
  li t1, 8
  slli t1, t1, 2
  li t2, 76
  add t0, t0, t1
  sw t2, 0(t0)
  # Line 5: Store Array/Pointer
  addi t0, s0, -136
  li t1, 9
  slli t1, t1, 2
  li t2, 54
  add t0, t0, t1
  sw t2, 0(t0)
  # Line 6: Store Array/Pointer
  addi t0, s0, -136
  li t1, 10
  slli t1, t1, 2
  li t2, 32
  add t0, t0, t1
  sw t2, 0(t0)
  # Line 6: Store Array/Pointer
  addi t0, s0, -136
  li t1, 11
  slli t1, t1, 2
  li t2, 16
  add t0, t0, t1
  sw t2, 0(t0)
  # Line 6: Store Array/Pointer
  addi t0, s0, -136
  li t1, 12
  slli t1, t1, 2
  li t2, 8
  add t0, t0, t1
  sw t2, 0(t0)
  # Line 6: Store Array/Pointer
  addi t0, s0, -136
  li t1, 13
  slli t1, t1, 2
  li t2, 4
  add t0, t0, t1
  sw t2, 0(t0)
  # Line 6: Store Array/Pointer
  addi t0, s0, -136
  li t1, 14
  slli t1, t1, 2
  li t2, 2
  add t0, t0, t1
  sw t2, 0(t0)
  # Line 7: Store Array/Pointer
  addi t0, s0, -136
  li t1, 15
  slli t1, t1, 2
  li t2, 1
  add t0, t0, t1
  sw t2, 0(t0)
  # Line 7: Store Array/Pointer
  addi t0, s0, -136
  li t1, 16
  slli t1, t1, 2
  li t2, 0
  add t0, t0, t1
  sw t2, 0(t0)
  # Line 7: Store Array/Pointer
  addi t0, s0, -136
  li t1, 17
  slli t1, t1, 2
  li t2, 5
  add t0, t0, t1
  sw t2, 0(t0)
  # Line 3: n$4 = ...
  li t0, 20
  mv s2, t0
  # Line 7: Store Array/Pointer
  addi t0, s0, -136
  li t1, 18
  slli t1, t1, 2
  li t2, 23
  add t0, t0, t1
  sw t2, 0(t0)
  # Line 7: Store Array/Pointer
  addi t0, s0, -136
  li t1, 19
  slli t1, t1, 2
  li t2, 32
  add t0, t0, t1
  sw t2, 0(t0)
  # Line 13: i$5 = ...
  li t0, 0
  mv s5, t0
  # Line 13: t0 = ... - ...
  mv t0, s2
  li t1, 1
  sub t2, t0, t1
  mv s4, t2
  # Line 21: label
L0:
  # Line 13: if (...) goto L3
  mv t0, s5
  mv t1, s4
  bge t0, t1, L3
  # Line 21: label
L1:
  # Line 14: t1 = ... - ...
  mv t0, s2
  mv t1, s5
  sub t2, t0, t1
  mv t3, t2
  # Line 14: j$6 = ...
  li t0, 0
  mv s1, t0
  # Line 14: t2 = ... - ...
  mv t0, t3
  li t1, 1
  sub t2, t0, t1
  mv t6, t2
  # Line 20: label
L4:
  # Line 14: if (...) goto L7
  mv t0, s1
  mv t1, t6
  bge t0, t1, L7
  # Line 20: label
L5:
  # Line 15: t4 = ... + ...
  mv t0, s1
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 15: Load Array/Pointer
  addi t0, s0, -136
  mv t1, s1
  slli t1, t1, 2
  add t2, t0, t1
  lw t2, 0(t2)
  mv t4, t2
  # Line 15: Load Array/Pointer
  addi t0, s0, -136
  mv t1, t3
  slli t1, t1, 2
  add t2, t0, t1
  lw t2, 0(t2)
  mv t3, t2
  # Line 15: if (...) goto L10
  mv t0, t4
  mv t1, t3
  ble t0, t1, L10
  # Line 20: label
L8:
  # Line 17: t7 = ... + ...
  mv t0, s1
  li t1, 1
  add t2, t0, t1
  mv t5, t2
  # Line 16: Load Array/Pointer
  addi t0, s0, -136
  mv t1, s1
  slli t1, t1, 2
  add t2, t0, t1
  lw t2, 0(t2)
  mv t4, t2
  # Line 17: Load Array/Pointer
  addi t0, s0, -136
  mv t1, t5
  slli t1, t1, 2
  add t2, t0, t1
  lw t2, 0(t2)
  mv t3, t2
  # Line 17: Store Array/Pointer
  addi t0, s0, -136
  mv t1, s1
  slli t1, t1, 2
  mv t2, t3
  add t0, t0, t1
  sw t2, 0(t0)
  # Line 18: Store Array/Pointer
  addi t0, s0, -136
  mv t1, t5
  slli t1, t1, 2
  mv t2, t4
  add t0, t0, t1
  sw t2, 0(t0)
  # Line 20: label
L10:
  # Line 14: t10 = ... + ...
  mv t0, s1
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 14: j$6 = ...
  mv t0, t3
  mv s1, t0
  # Line 20: goto L4
  j L4
  # Line 20: label
L7:
  # Line 13: t11 = ... + ...
  mv t0, s5
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 13: i$5 = ...
  mv t0, t3
  mv s5, t0
  # Line 21: goto L0
  j L0
  # Line 21: label
L3:
  # Line 23: Load Array/Pointer
  addi t0, s0, -136
  li t1, 0
  slli t1, t1, 2
  add t2, t0, t1
  lw t2, 0(t2)
  mv t3, t2
  # Line 23: return
  mv a0, t3
  addi sp, s0, -176
  ld s1, 152(sp)
  ld s2, 144(sp)
  ld s3, 136(sp)
  ld s4, 128(sp)
  ld s5, 120(sp)
  ld ra, 168(sp)
  ld s0, 160(sp)
  addi sp, sp, 176
  jr ra

  # --- Default Epilogue ---
  addi sp, s0, -176
  ld s1, 152(sp)
  ld s2, 144(sp)
  ld s3, 136(sp)
  ld s4, 128(sp)
  ld s5, 120(sp)
  ld ra, 168(sp)
  ld s0, 160(sp)
  addi sp, sp, 176
  jr ra

