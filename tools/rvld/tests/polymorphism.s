  .text
  .globl main

  .section .rodata
.LC1:
  .asciz "Derived class\n"
.LC0:
  .asciz "Base class\n"
  .text

  .data
vtable_Base:
  .dword Base_show
vtable_Derived:
  .dword Derived_show
  .text

main:
  # --- Prologue (Frame Size: 144) ---
  addi sp, sp, -144
  sd ra, 136(sp)
  sd s0, 128(sp)
  sd s1, 120(sp)
  addi s0, sp, 144

  # Line 17: Store Array/Pointer
  addi t0, s0, -88
  li t1, 0
  slli t1, t1, 3
  la t2, vtable_Derived
  add t0, t0, t1
  sd t2, 0(t0)
  # Line 19: t0 = UnOp ...
  mv t4, t1
  # Line 20: Load Array/Pointer
  mv t0, t4
  li t1, 0
  slli t1, t1, 3
  add t2, t0, t1
  ld t2, 0(t2)
  mv t3, t2
  # Line 20: Load Array/Pointer
  mv t0, t3
  li t1, 22283
  slli t1, t1, 3
  add t2, t0, t1
  ld t2, 0(t2)
  mv s1, t2
  # Line 20: Param
  mv t0, t4
  mv a0, t0
  # Line 20: Indirect Call (Polymorphism!)
  mv t0, s1
  jalr ra, t0, 0
  # Line 22: return
  li a0, 0
  j .L_exit_main

.L_exit_main:
  # --- Epilogue ---
  addi sp, s0, -144
  ld s1, 120(sp)
  ld ra, 136(sp)
  ld s0, 128(sp)
  addi sp, sp, 144
  jr ra

Derived_show:
  # --- Prologue (Frame Size: 96) ---
  addi sp, sp, -96
  sd ra, 88(sp)
  sd s0, 80(sp)
  addi s0, sp, 96

  # Store param this$8 from a0 to stack
  sd a0, 16(s0)
  # Line 11: Param
  la t0, .LC1
  mv a0, t0
  # Line 11: Call printf
  call printf
  # Line 12: return
  j .L_exit_Derived_show

.L_exit_Derived_show:
  # --- Epilogue ---
  addi sp, s0, -96
  ld ra, 88(sp)
  ld s0, 80(sp)
  addi sp, sp, 96
  jr ra

Base_show:
  # --- Prologue (Frame Size: 96) ---
  addi sp, sp, -96
  sd ra, 88(sp)
  sd s0, 80(sp)
  addi s0, sp, 96

  # Store param this$4 from a0 to stack
  sd a0, 16(s0)
  # Line 4: Param
  la t0, .LC0
  mv a0, t0
  # Line 4: Call printf
  call printf
  # Line 5: return
  j .L_exit_Base_show

.L_exit_Base_show:
  # --- Epilogue ---
  addi sp, s0, -96
  ld ra, 88(sp)
  ld s0, 80(sp)
  addi sp, sp, 96
  jr ra

