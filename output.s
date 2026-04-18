  .text
  .globl main

  .section .rodata
.LC12:
  .asciz "Invalid choice!\n"
.LC11:
  .asciz "%d"
.LC10:
  .asciz "Enter value to search: "
.LC9:
  .asciz "%d"
.LC8:
  .asciz "Enter value to insert: "
.LC7:
  .asciz "%d"
.LC6:
  .asciz "Enter choice: "
.LC5:
  .asciz "\n1. Insert\n2. Display (Inorder)\n3. Search\n4. Exit\n"
.LC4:
  .asciz "Element %d not found in BST\n"
.LC3:
  .asciz "Element %d found in BST\n"
.LC2:
  .asciz "\n"
.LC1:
  .asciz "Inorder Traversal: "
.LC0:
  .asciz "%d "
  .text

main:
  # --- Prologue (Frame Size: 224) ---
  addi sp, sp, -224
  sd ra, 216(sp)
  sd s0, 208(sp)
  sd s1, 200(sp)
  sd s2, 192(sp)
  sd s3, 184(sp)
  sd s4, 176(sp)
  sd s5, 168(sp)
  sd s6, 160(sp)
  sd s7, 152(sp)
  addi s0, sp, 224

  # Line 75: t0 = UnOp ...
  addi t1, s0, -80
  mv t3, t1
  # Line 75: Param
  mv t0, t3
  mv a0, t0
  # Line 75: Call BST__ctor
  call BST__ctor
  # Line 81: t1 = UnOp ...
  addi t1, s0, -84
  mv s7, t1
  # Line 86: t2 = UnOp ...
  addi t1, s0, -88
  mv s6, t1
  # Line 87: t3 = UnOp ...
  addi t1, s0, -80
  mv s5, t1
  # Line 91: t4 = UnOp ...
  addi t1, s0, -80
  mv s4, t1
  # Line 96: t5 = UnOp ...
  addi t1, s0, -88
  mv s3, t1
  # Line 97: t6 = UnOp ...
  addi t1, s0, -80
  mv s2, t1
  # Line 106: label
L19:
  # Line 79: Param
  la t0, .LC5
  mv a0, t0
  # Line 79: Call printf
  call printf
  # Line 80: Param
  la t0, .LC6
  mv a0, t0
  # Line 80: Call printf
  call printf
  # Line 81: Param
  la t0, .LC7
  mv a0, t0
  # Line 81: Param
  mv t0, s7
  mv a1, t0
  # Line 81: Call scanf
  call scanf
  # Line 105: if (...) goto L22
  lw t0, -84(s0)
  li t1, 1
  beq t0, t1, L22
  # Line 105: if (...) goto L23
  lw t0, -84(s0)
  li t1, 2
  beq t0, t1, L23
  # Line 105: if (...) goto L24
  lw t0, -84(s0)
  li t1, 3
  beq t0, t1, L24
  # Line 105: if (...) goto L25
  lw t0, -84(s0)
  li t1, 4
  beq t0, t1, L25
  # Line 105: goto L26
  j L26
  # Line 90: label
L22:
  # Line 85: Param
  la t0, .LC8
  mv a0, t0
  # Line 85: Call printf
  call printf
  # Line 86: Param
  la t0, .LC9
  mv a0, t0
  # Line 86: Param
  mv t0, s6
  mv a1, t0
  # Line 86: Call scanf
  call scanf
  # Line 87: Param
  mv t0, s5
  mv a0, t0
  # Line 87: Param
  lw t0, -88(s0)
  mv a1, t0
  # Line 87: Call BST_insert_int
  call BST_insert_int
  # Line 88: goto L19
  j L19
  # Line 94: label
L23:
  # Line 91: Param
  mv t0, s4
  mv a0, t0
  # Line 91: Call BST_display
  call BST_display
  # Line 92: goto L19
  j L19
  # Line 100: label
L24:
  # Line 95: Param
  la t0, .LC10
  mv a0, t0
  # Line 95: Call printf
  call printf
  # Line 96: Param
  la t0, .LC11
  mv a0, t0
  # Line 96: Param
  mv t0, s3
  mv a1, t0
  # Line 96: Call scanf
  call scanf
  # Line 97: Param
  mv t0, s2
  mv a0, t0
  # Line 97: Param
  lw t0, -88(s0)
  mv a1, t0
  # Line 97: Call BST_find_int
  call BST_find_int
  # Line 98: goto L19
  j L19
  # Line 103: label
L25:
  # Line 101: return
  li a0, 0
  j .L_exit_main
  # Line 105: label
L26:
  # Line 104: Param
  la t0, .LC12
  mv a0, t0
  # Line 104: Call printf
  call printf
  # Line 106: goto L19
  j L19

.L_exit_main:
  # --- Epilogue ---
  addi sp, s0, -224
  ld s1, 200(sp)
  ld s2, 192(sp)
  ld s3, 184(sp)
  ld s4, 176(sp)
  ld s5, 168(sp)
  ld s6, 160(sp)
  ld s7, 152(sp)
  ld ra, 216(sp)
  ld s0, 208(sp)
  addi sp, sp, 224
  jr ra

BST_find_int:
  # --- Prologue (Frame Size: 144) ---
  addi sp, sp, -144
  sd ra, 136(sp)
  sd s0, 128(sp)
  sd s1, 120(sp)
  addi s0, sp, 144

  # Move param this$31 from a0 to t4
  mv t4, a0
  # Move param key$32 from a1 to s1
  mv s1, a1
  # Line 66: Load Array/Pointer
  mv t0, t4
  li t1, 0
  slli t1, t1, 3
  add t2, t0, t1
  ld t2, 0(t2)
  mv t3, t2
  # Line 66: Param
  mv t0, t4
  mv a0, t0
  # Line 66: Param
  mv t0, t3
  mv a1, t0
  # Line 66: Param
  mv t0, s1
  mv a2, t0
  # Line 66: Call BST_search_struct_int
  call BST_search_struct_int
  mv t3, a0
  # Line 67: if (...) goto L17
  mv t0, t3
  li t1, 0
  beq t0, t1, L17
  # Line 70: label
L16:
  # Line 68: Param
  la t0, .LC3
  mv a0, t0
  # Line 68: Param
  mv t0, s1
  mv a1, t0
  # Line 68: Call printf
  call printf
  # Line 70: goto L18
  j L18
  # Line 70: label
L17:
  # Line 70: Param
  la t0, .LC4
  mv a0, t0
  # Line 70: Param
  mv t0, s1
  mv a1, t0
  # Line 70: Call printf
  call printf
  # Line 70: label
L18:
  # Line 71: return
  j .L_exit_BST_find_int

.L_exit_BST_find_int:
  # --- Epilogue ---
  addi sp, s0, -144
  ld s1, 120(sp)
  ld ra, 136(sp)
  ld s0, 128(sp)
  addi sp, sp, 144
  jr ra

BST_display:
  # --- Prologue (Frame Size: 112) ---
  addi sp, sp, -112
  sd ra, 104(sp)
  sd s0, 96(sp)
  sd s1, 88(sp)
  addi s0, sp, 112

  # Move param this$29 from a0 to s1
  mv s1, a0
  # Line 60: Param
  la t0, .LC1
  mv a0, t0
  # Line 60: Call printf
  call printf
  # Line 61: Load Array/Pointer
  mv t0, s1
  li t1, 0
  slli t1, t1, 3
  add t2, t0, t1
  ld t2, 0(t2)
  mv t3, t2
  # Line 61: Param
  mv t0, s1
  mv a0, t0
  # Line 61: Param
  mv t0, t3
  mv a1, t0
  # Line 61: Call BST_inorder_struct
  call BST_inorder_struct
  # Line 62: Param
  la t0, .LC2
  mv a0, t0
  # Line 62: Call printf
  call printf
  # Line 63: return
  j .L_exit_BST_display

.L_exit_BST_display:
  # --- Epilogue ---
  addi sp, s0, -112
  ld s1, 88(sp)
  ld ra, 104(sp)
  ld s0, 96(sp)
  addi sp, sp, 112
  jr ra

BST_insert_int:
  # --- Prologue (Frame Size: 112) ---
  addi sp, sp, -112
  sd ra, 104(sp)
  sd s0, 96(sp)
  sd s1, 88(sp)
  addi s0, sp, 112

  # Move param this$26 from a0 to s1
  mv s1, a0
  # Move param val$27 from a1 to t4
  mv t4, a1
  # Line 56: Load Array/Pointer
  mv t0, s1
  li t1, 0
  slli t1, t1, 3
  add t2, t0, t1
  ld t2, 0(t2)
  mv t3, t2
  # Line 56: Param
  mv t0, s1
  mv a0, t0
  # Line 56: Param
  mv t0, t3
  mv a1, t0
  # Line 56: Param
  mv t0, t4
  mv a2, t0
  # Line 56: Call BST_insert_struct_int
  call BST_insert_struct_int
  mv t3, a0
  # Line 56: Store Array/Pointer
  mv t0, s1
  li t1, 0
  slli t1, t1, 3
  mv t2, t3
  add t0, t0, t1
  sd t2, 0(t0)
  # Line 57: return
  j .L_exit_BST_insert_int

.L_exit_BST_insert_int:
  # --- Epilogue ---
  addi sp, s0, -112
  ld s1, 88(sp)
  ld ra, 104(sp)
  ld s0, 96(sp)
  addi sp, sp, 112
  jr ra

BST__ctor:
  # --- Prologue (Frame Size: 96) ---
  addi sp, sp, -96
  sd ra, 88(sp)
  sd s0, 80(sp)
  addi s0, sp, 96

  # Move param this$24 from a0 to t3
  mv t3, a0
  # Line 52: Store Array/Pointer
  mv t0, t3
  li t1, 0
  slli t1, t1, 3
  li t2, 0
  add t0, t0, t1
  sd t2, 0(t0)
  # Line 53: return
  j .L_exit_BST__ctor

.L_exit_BST__ctor:
  # --- Epilogue ---
  addi sp, s0, -96
  ld ra, 88(sp)
  ld s0, 80(sp)
  addi sp, sp, 96
  jr ra

BST_search_struct_int:
  # --- Prologue (Frame Size: 160) ---
  addi sp, sp, -160
  sd ra, 152(sp)
  sd s0, 144(sp)
  addi s0, sp, 160

  # Tail recursion entry point
BST_search_struct_int_tail_entry:

  # Move param this$20 from a0 to t6
  mv t6, a0
  # Move param node$21 from a1 to t5
  mv t5, a1
  # Move param key$22 from a2 to t4
  mv t4, a2
  # Line 41: if (...) goto L9
  mv t0, t5
  li t1, 0
  beq t0, t1, L9
  # Line 41: label
L12:
  # Line 41: Load Array/Pointer
  mv t0, t5
  li t1, 0
  slli t1, t1, 2
  add t2, t0, t1
  lw t2, 0(t2)
  mv t3, t2
  # Line 41: if (...) goto L11
  mv t0, t3
  mv t1, t4
  bne t0, t1, L11
  # Line 44: label
L9:
  # Line 42: return
  mv a0, t5
  j .L_exit_BST_search_struct_int
  # Line 44: label
L11:
  # Line 44: Load Array/Pointer
  mv t0, t5
  li t1, 0
  slli t1, t1, 2
  add t2, t0, t1
  lw t2, 0(t2)
  mv t3, t2
  # Line 44: if (...) goto L14
  mv t0, t4
  mv t1, t3
  bge t0, t1, L14
  # Line 47: label
L13:
  # Line 45: Load Array/Pointer
  mv t0, t5
  li t1, 1
  slli t1, t1, 3
  add t2, t0, t1
  ld t2, 0(t2)
  mv t3, t2
  # Line 45: Param
  mv t0, t6
  mv a0, t0
  # Line 45: Param
  mv t0, t3
  mv a1, t0
  # Line 45: Param
  mv t0, t4
  mv a2, t0
  # Line 45: Call BST_search_struct_int
  # Tail recursive self-call: reuse frame and jump to body
  j BST_search_struct_int_tail_entry
  # Line 45:   # Line 47: label
L14:
  # Line 47: Load Array/Pointer
  mv t0, t5
  li t1, 2
  slli t1, t1, 3
  add t2, t0, t1
  ld t2, 0(t2)
  mv t3, t2
  # Line 47: Param
  mv t0, t6
  mv a0, t0
  # Line 47: Param
  mv t0, t3
  mv a1, t0
  # Line 47: Param
  mv t0, t4
  mv a2, t0
  # Line 47: Call BST_search_struct_int
  # Tail recursive self-call: reuse frame and jump to body
  j BST_search_struct_int_tail_entry
  # Line 47: 
.L_exit_BST_search_struct_int:
  # --- Epilogue ---
  addi sp, s0, -160
  ld ra, 152(sp)
  ld s0, 144(sp)
  addi sp, sp, 160
  jr ra

BST_inorder_struct:
  # --- Prologue (Frame Size: 128) ---
  addi sp, sp, -128
  sd ra, 120(sp)
  sd s0, 112(sp)
  sd s1, 104(sp)
  sd s2, 96(sp)
  addi s0, sp, 128

  # Tail recursion entry point
BST_inorder_struct_tail_entry:

  # Move param this$17 from a0 to s2
  mv s2, a0
  # Move param node$18 from a1 to s1
  mv s1, a1
  # Line 32: if (...) goto L8
  mv t0, s1
  li t1, 0
  bne t0, t1, L8
  # Line 34: label
L6:
  # Line 32: return
  j .L_exit_BST_inorder_struct
  # Line 34: label
L8:
  # Line 34: Load Array/Pointer
  mv t0, s1
  li t1, 1
  slli t1, t1, 3
  add t2, t0, t1
  ld t2, 0(t2)
  mv t3, t2
  # Line 34: Param
  mv t0, s2
  mv a0, t0
  # Line 34: Param
  mv t0, t3
  mv a1, t0
  # Line 34: Call BST_inorder_struct
  call BST_inorder_struct
  # Line 35: Load Array/Pointer
  mv t0, s1
  li t1, 0
  slli t1, t1, 2
  add t2, t0, t1
  lw t2, 0(t2)
  mv t3, t2
  # Line 35: Param
  la t0, .LC0
  mv a0, t0
  # Line 35: Param
  mv t0, t3
  mv a1, t0
  # Line 35: Call printf
  call printf
  # Line 36: Load Array/Pointer
  mv t0, s1
  li t1, 2
  slli t1, t1, 3
  add t2, t0, t1
  ld t2, 0(t2)
  mv t3, t2
  # Line 36: Param
  mv t0, s2
  mv a0, t0
  # Line 36: Param
  mv t0, t3
  mv a1, t0
  # Line 36: Call BST_inorder_struct
  # Tail recursive self-call: reuse frame and jump to body
  j BST_inorder_struct_tail_entry
  # Line 37: 
.L_exit_BST_inorder_struct:
  # --- Epilogue ---
  addi sp, s0, -128
  ld s1, 104(sp)
  ld s2, 96(sp)
  ld ra, 120(sp)
  ld s0, 112(sp)
  addi sp, sp, 128
  jr ra

BST_insert_struct_int:
  # --- Prologue (Frame Size: 160) ---
  addi sp, sp, -160
  sd ra, 152(sp)
  sd s0, 144(sp)
  sd s1, 136(sp)
  sd s2, 128(sp)
  addi s0, sp, 160

  # Move param this$13 from a0 to t4
  mv t4, a0
  # Move param node$14 from a1 to s1
  mv s1, a1
  # Move param val$15 from a2 to s2
  mv s2, a2
  # Line 19: if (...) goto L2
  mv t0, s1
  li t1, 0
  bne t0, t1, L2
  # Line 22: label
L0:
  # Line 20: Param
  li t0, 24
  mv a0, t0
  # Line 20: Call malloc
  call malloc
  mv s1, a0
  # Line 20: Param
  mv t0, s1
  mv a0, t0
  # Line 20: Param
  mv t0, s2
  mv a1, t0
  # Line 20: Call Node__ctor
  call Node__ctor
  # Line 20: return
  mv a0, s1
  j .L_exit_BST_insert_struct_int
  # Line 22: label
L2:
  # Line 22: Load Array/Pointer
  mv t0, s1
  li t1, 0
  slli t1, t1, 2
  add t2, t0, t1
  lw t2, 0(t2)
  mv t3, t2
  # Line 22: if (...) goto L4
  mv t0, s2
  mv t1, t3
  bge t0, t1, L4
  # Line 25: label
L3:
  # Line 23: Load Array/Pointer
  mv t0, s1
  li t1, 1
  slli t1, t1, 3
  add t2, t0, t1
  ld t2, 0(t2)
  mv t3, t2
  # Line 23: Param
  mv t0, t4
  mv a0, t0
  # Line 23: Param
  mv t0, t3
  mv a1, t0
  # Line 23: Param
  mv t0, s2
  mv a2, t0
  # Line 23: Call BST_insert_struct_int
  call BST_insert_struct_int
  mv t3, a0
  # Line 23: Store Array/Pointer
  mv t0, s1
  li t1, 1
  slli t1, t1, 3
  mv t2, t3
  add t0, t0, t1
  sd t2, 0(t0)
  # Line 25: goto L5
  j L5
  # Line 25: label
L4:
  # Line 25: Load Array/Pointer
  mv t0, s1
  li t1, 2
  slli t1, t1, 3
  add t2, t0, t1
  ld t2, 0(t2)
  mv t3, t2
  # Line 25: Param
  mv t0, t4
  mv a0, t0
  # Line 25: Param
  mv t0, t3
  mv a1, t0
  # Line 25: Param
  mv t0, s2
  mv a2, t0
  # Line 25: Call BST_insert_struct_int
  call BST_insert_struct_int
  mv t3, a0
  # Line 25: Store Array/Pointer
  mv t0, s1
  li t1, 2
  slli t1, t1, 3
  mv t2, t3
  add t0, t0, t1
  sd t2, 0(t0)
  # Line 25: label
L5:
  # Line 27: return
  mv a0, s1
  j .L_exit_BST_insert_struct_int

.L_exit_BST_insert_struct_int:
  # --- Epilogue ---
  addi sp, s0, -160
  ld s1, 136(sp)
  ld s2, 128(sp)
  ld ra, 152(sp)
  ld s0, 144(sp)
  addi sp, sp, 160
  jr ra

Node__ctor:
  # --- Prologue (Frame Size: 96) ---
  addi sp, sp, -96
  sd ra, 88(sp)
  sd s0, 80(sp)
  addi s0, sp, 96

  # Move param this$8 from a0 to t4
  mv t4, a0
  # Move param val$9 from a1 to t3
  mv t3, a1
  # Line 8: Store Array/Pointer
  mv t0, t4
  li t1, 0
  slli t1, t1, 2
  mv t2, t3
  add t0, t0, t1
  sw t2, 0(t0)
  # Line 9: Store Array/Pointer
  mv t0, t4
  li t1, 2
  slli t1, t1, 3
  li t2, 0
  add t0, t0, t1
  sd t2, 0(t0)
  # Line 9: Store Array/Pointer
  mv t0, t4
  li t1, 1
  slli t1, t1, 3
  li t2, 0
  add t0, t0, t1
  sd t2, 0(t0)
  # Line 10: return
  j .L_exit_Node__ctor

.L_exit_Node__ctor:
  # --- Epilogue ---
  addi sp, s0, -96
  ld ra, 88(sp)
  ld s0, 80(sp)
  addi sp, sp, 96
  jr ra

