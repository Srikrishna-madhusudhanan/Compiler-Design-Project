  .text
  .globl main

  .section .rodata
.LC2:
  .asciz "Tail-Recursive Factorial is %d\n"
.LC1:
  .asciz "%d"
.LC0:
  .asciz "Enter a positive integer: "
  .text

main:
  # --- Prologue (Frame Size: 96) ---
  addi sp, sp, -96
  sd ra, 88(sp)
  sd s0, 80(sp)
  sd s1, 72(sp)
  addi s0, sp, 96

  # Line 13: Param
  la t0, .LC0
  mv a0, t0
  # Line 13: Call printf
  call printf
  # Line 15: t0 = UnOp ...
  addi t1, s0, -28
  mv t3, t1
  # Line 15: Param
  la t0, .LC1
  mv a0, t0
  # Line 15: Param
  mv t0, t3
  mv a1, t0
  # Line 15: Call scanf
  call scanf
  # Line 16: Param
  lw t0, -28(s0)
  mv a0, t0
  # Line 16: Param
  li t0, 1
  mv a1, t0
  # Line 16: Call fact_tail
  call fact_tail
  mv t3, a0
  # Line 17: Param
  la t0, .LC2
  mv a0, t0
  # Line 17: Param
  mv t0, t3
  mv a1, t0
  # Line 17: Call printf
  call printf
  # Line 18: return
  li a0, 0
  addi sp, s0, -96
  ld s1, 72(sp)
  ld ra, 88(sp)
  ld s0, 80(sp)
  addi sp, sp, 96
  jr ra

  # --- Default Epilogue ---
  addi sp, s0, -96
  ld s1, 72(sp)
  ld ra, 88(sp)
  ld s0, 80(sp)
  addi sp, sp, 96
  jr ra

fact_tail:
  # --- Prologue (Frame Size: 80) ---
  addi sp, sp, -80
  sd ra, 72(sp)
  sd s0, 64(sp)
  addi s0, sp, 80

  # Tail recursion entry point
fact_tail_tail_entry:

  # Move param n$1 from a0 to t5
  mv t5, a0
  # Move param accumulator$2 from a1 to t4
  mv t4, a1
  # Line 2: if (...) goto L2
  mv t0, t5
  li t1, 1
  bgt t0, t1, L2
  # Line 9: label
L0:
  # Line 3: return
  mv a0, t4
  addi sp, s0, -80
  ld ra, 72(sp)
  ld s0, 64(sp)
  addi sp, sp, 80
  jr ra
  # Line 9: label
L2:
  # Line 9: t0 = ... - ...
  mv t0, t5
  li t1, 1
  sub t2, t0, t1
  mv t3, t2
  # Line 9: t1 = ... * ...
  mv t0, t5
  mv t1, t4
  mul t2, t0, t1
  mv t4, t2
  # Line 9: Param
  mv t0, t3
  mv a0, t0
  # Line 9: Param
  mv t0, t4
  mv a1, t0
  # Line 9: Call fact_tail
  # Tail recursive self-call: reuse frame and jump to body
  j fact_tail_tail_entry
  # Line 9: 
  # --- Default Epilogue ---
  addi sp, s0, -80
  ld ra, 72(sp)
  ld s0, 64(sp)
  addi sp, sp, 80
  jr ra

