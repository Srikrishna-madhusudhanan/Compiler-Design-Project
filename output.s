  .text
  .globl main

  .section .rodata
.LC6:
  .asciz "The top element is %d\n"
.LC5:
  .asciz "%d popped from the stack\n"
.LC4:
  .asciz "%d popped from the stack\n"
.LC3:
  .asciz "Stack is Empty\n"
.LC2:
  .asciz "Stack underflow!\n"
.LC1:
  .asciz "%d pushed into the stack\n"
.LC0:
  .asciz "Stack overflow!\n"
  .text

main:
  # --- Prologue (Frame Size: 624) ---
  addi sp, sp, -624
  sd ra, 616(sp)
  sd s0, 608(sp)
  sd s1, 600(sp)
  addi s0, sp, 624

  # Line 55: t0 = UnOp ...
  addi t1, s0, -428
  mv t3, t1
  # Line 55: Param
  mv t0, t3
  mv a0, t0
  # Line 55: Call Stack__ctor
  call Stack__ctor
  # Line 56: t1 = UnOp ...
  addi t1, s0, -428
  mv t3, t1
  # Line 56: Param
  mv t0, t3
  mv a0, t0
  # Line 56: Param
  li t0, 10
  mv a1, t0
  # Line 56: Call Stack_push_int
  call Stack_push_int
  mv t3, a0
  # Line 57: t3 = UnOp ...
  addi t1, s0, -428
  mv t3, t1
  # Line 57: Param
  mv t0, t3
  mv a0, t0
  # Line 57: Param
  li t0, 20
  mv a1, t0
  # Line 57: Call Stack_push_int
  call Stack_push_int
  mv t3, a0
  # Line 58: t5 = UnOp ...
  addi t1, s0, -428
  mv t3, t1
  # Line 58: Param
  mv t0, t3
  mv a0, t0
  # Line 58: Param
  li t0, 30
  mv a1, t0
  # Line 58: Call Stack_push_int
  call Stack_push_int
  mv t3, a0
  # Line 60: t7 = UnOp ...
  addi t1, s0, -428
  mv t3, t1
  # Line 60: Param
  mv t0, t3
  mv a0, t0
  # Line 60: Call Stack_pop
  call Stack_pop
  mv t3, a0
  # Line 61: Param
  la t0, .LC4
  mv a0, t0
  # Line 61: Param
  mv t0, t3
  mv a1, t0
  # Line 61: Call printf
  call printf
  # Line 62: t9 = UnOp ...
  addi t1, s0, -428
  mv t3, t1
  # Line 62: Param
  mv t0, t3
  mv a0, t0
  # Line 62: Call Stack_pop
  call Stack_pop
  mv t3, a0
  # Line 63: Param
  la t0, .LC5
  mv a0, t0
  # Line 63: Param
  mv t0, t3
  mv a1, t0
  # Line 63: Call printf
  call printf
  # Line 64: t11 = UnOp ...
  addi t1, s0, -428
  mv t3, t1
  # Line 64: Param
  mv t0, t3
  mv a0, t0
  # Line 64: Call Stack_peek
  call Stack_peek
  mv t3, a0
  # Line 64: Param
  la t0, .LC6
  mv a0, t0
  # Line 64: Param
  mv t0, t3
  mv a1, t0
  # Line 64: Call printf
  call printf
  # Line 66: return
  li a0, 0
  j .L_exit_main

.L_exit_main:
  # --- Epilogue ---
  addi sp, s0, -624
  ld s1, 600(sp)
  ld ra, 616(sp)
  ld s0, 608(sp)
  addi sp, sp, 624
  jr ra

Stack_isEmpty:
  # --- Prologue (Frame Size: 112) ---
  addi sp, sp, -112
  sd ra, 104(sp)
  sd s0, 96(sp)
  addi s0, sp, 112

  # Move param this$18 from a0 to t3
  mv t3, a0
  # Line 50: Load Array/Pointer
  mv t0, t3
  li t1, 100
  slli t1, t1, 2
  add t2, t0, t1
  lw t2, 0(t2)
  mv t3, t2
  # Line 50: t1 = ...
  li t0, 0
  mv t4, t0
  # Line 50: if (...) goto L10
  mv t0, t3
  li t1, 0
  bge t0, t1, L10
  # Line 50: label
L9:
  # Line 50: t1 = ...
  li t0, 1
  mv t4, t0
  # Line 50: label
L10:
  # Line 50: return
  mv a0, t4
  j .L_exit_Stack_isEmpty

.L_exit_Stack_isEmpty:
  # --- Epilogue ---
  addi sp, s0, -112
  ld ra, 104(sp)
  ld s0, 96(sp)
  addi sp, sp, 112
  jr ra

Stack_peek:
  # --- Prologue (Frame Size: 128) ---
  addi sp, sp, -128
  sd ra, 120(sp)
  sd s0, 112(sp)
  addi s0, sp, 128

  # Move param this$16 from a0 to t4
  mv t4, a0
  # Line 40: Load Array/Pointer
  mv t0, t4
  li t1, 100
  slli t1, t1, 2
  add t2, t0, t1
  lw t2, 0(t2)
  mv t3, t2
  # Line 40: if (...) goto L7
  mv t0, t3
  li t1, 0
  bge t0, t1, L7
  # Line 45: label
L6:
  # Line 41: Param
  la t0, .LC3
  mv a0, t0
  # Line 41: Call printf
  call printf
  # Line 42: return
  li a0, 0
  j .L_exit_Stack_peek
  # Line 45: label
L7:
  # Line 44: Load Array/Pointer
  mv t0, t4
  li t1, 100
  slli t1, t1, 2
  add t2, t0, t1
  lw t2, 0(t2)
  mv t3, t2
  # Line 44: Load Array/Pointer
  mv t0, t4
  mv t1, t3
  slli t1, t1, 2
  add t2, t0, t1
  lw t2, 0(t2)
  mv t3, t2
  # Line 44: return
  mv a0, t3
  j .L_exit_Stack_peek

.L_exit_Stack_peek:
  # --- Epilogue ---
  addi sp, s0, -128
  ld ra, 120(sp)
  ld s0, 112(sp)
  addi sp, sp, 128
  jr ra

Stack_pop:
  # --- Prologue (Frame Size: 144) ---
  addi sp, sp, -144
  sd ra, 136(sp)
  sd s0, 128(sp)
  addi s0, sp, 144

  # Move param this$13 from a0 to t5
  mv t5, a0
  # Line 29: Load Array/Pointer
  mv t0, t5
  li t1, 100
  slli t1, t1, 2
  add t2, t0, t1
  lw t2, 0(t2)
  mv t3, t2
  # Line 29: if (...) goto L4
  mv t0, t3
  li t1, 0
  bge t0, t1, L4
  # Line 35: label
L3:
  # Line 30: Param
  la t0, .LC2
  mv a0, t0
  # Line 30: Call printf
  call printf
  # Line 31: return
  li a0, 0
  j .L_exit_Stack_pop
  # Line 35: label
L4:
  # Line 33: Load Array/Pointer
  mv t0, t5
  li t1, 100
  slli t1, t1, 2
  add t2, t0, t1
  lw t2, 0(t2)
  mv t4, t2
  # Line 33: t2 = ... - ...
  mv t0, t4
  li t1, 1
  sub t2, t0, t1
  mv t3, t2
  # Line 33: Store Array/Pointer
  mv t0, t5
  li t1, 100
  slli t1, t1, 2
  mv t2, t3
  add t0, t0, t1
  sw t2, 0(t0)
  # Line 33: Load Array/Pointer
  mv t0, t5
  mv t1, t4
  slli t1, t1, 2
  add t2, t0, t1
  lw t2, 0(t2)
  mv t3, t2
  # Line 34: return
  mv a0, t3
  j .L_exit_Stack_pop

.L_exit_Stack_pop:
  # --- Epilogue ---
  addi sp, s0, -144
  ld ra, 136(sp)
  ld s0, 128(sp)
  addi sp, sp, 144
  jr ra

Stack_push_int:
  # --- Prologue (Frame Size: 144) ---
  addi sp, sp, -144
  sd ra, 136(sp)
  sd s0, 128(sp)
  addi s0, sp, 144

  # Move param this$10 from a0 to t5
  mv t5, a0
  # Move param x$11 from a1 to t4
  mv t4, a1
  # Line 17: Load Array/Pointer
  mv t0, t5
  li t1, 100
  slli t1, t1, 2
  add t2, t0, t1
  lw t2, 0(t2)
  mv t3, t2
  # Line 17: if (...) goto L1
  mv t0, t3
  li t1, 99
  blt t0, t1, L1
  # Line 24: label
L0:
  # Line 18: Param
  la t0, .LC0
  mv a0, t0
  # Line 18: Call printf
  call printf
  # Line 19: return
  li a0, 0
  j .L_exit_Stack_push_int
  # Line 24: label
L1:
  # Line 21: Load Array/Pointer
  mv t0, t5
  li t1, 100
  slli t1, t1, 2
  add t2, t0, t1
  lw t2, 0(t2)
  mv t3, t2
  # Line 21: t3 = ... + ...
  mv t0, t3
  li t1, 1
  add t2, t0, t1
  mv t3, t2
  # Line 21: Store Array/Pointer
  mv t0, t5
  li t1, 100
  slli t1, t1, 2
  mv t2, t3
  add t0, t0, t1
  sw t2, 0(t0)
  # Line 22: Param
  la t0, .LC1
  mv a0, t0
  # Line 21: Store Array/Pointer
  mv t0, t5
  mv t1, t3
  slli t1, t1, 2
  mv t2, t4
  add t0, t0, t1
  sw t2, 0(t0)
  # Line 22: Param
  mv t0, t4
  mv a1, t0
  # Line 22: Call printf
  call printf
  # Line 23: return
  li a0, 1
  j .L_exit_Stack_push_int

.L_exit_Stack_push_int:
  # --- Epilogue ---
  addi sp, s0, -144
  ld ra, 136(sp)
  ld s0, 128(sp)
  addi sp, sp, 144
  jr ra

Stack__ctor:
  # --- Prologue (Frame Size: 96) ---
  addi sp, sp, -96
  sd ra, 88(sp)
  sd s0, 80(sp)
  addi s0, sp, 96

  # Move param this$8 from a0 to t3
  mv t3, a0
  # Line 12: Store Array/Pointer
  mv t0, t3
  li t1, 100
  slli t1, t1, 2
  li t2, -1
  add t0, t0, t1
  sw t2, 0(t0)
  # Line 13: return
  j .L_exit_Stack__ctor

.L_exit_Stack__ctor:
  # --- Epilogue ---
  addi sp, s0, -96
  ld ra, 88(sp)
  ld s0, 80(sp)
  addi sp, sp, 96
  jr ra

