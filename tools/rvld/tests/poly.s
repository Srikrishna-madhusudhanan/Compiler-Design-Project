  .text
  .globl main

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
  li t1, 25500
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
  mv t3, a0
  # Line 20: return
  mv a0, t3
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
  # --- Prologue (Frame Size: 80) ---
  addi sp, sp, -80
  sd ra, 72(sp)
  sd s0, 64(sp)
  addi s0, sp, 80

  # Store param this$8 from a0 to stack
  sd a0, 16(s0)
  # Line 11: return
  li a0, 2
  j .L_exit_Derived_show

.L_exit_Derived_show:
  # --- Epilogue ---
  addi sp, s0, -80
  ld ra, 72(sp)
  ld s0, 64(sp)
  addi sp, sp, 80
  jr ra

Base_show:
  # --- Prologue (Frame Size: 80) ---
  addi sp, sp, -80
  sd ra, 72(sp)
  sd s0, 64(sp)
  addi s0, sp, 80

  # Store param this$4 from a0 to stack
  sd a0, 16(s0)
  # Line 4: return
  li a0, 1
  j .L_exit_Base_show

.L_exit_Base_show:
  # --- Epilogue ---
  addi sp, s0, -80
  ld ra, 72(sp)
  ld s0, 64(sp)
  addi sp, sp, 80
  jr ra

