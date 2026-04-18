  .text
  .globl main

main:
  # --- Prologue (Frame Size: 96) ---
  addi sp, sp, -96
  sd ra, 88(sp)
  sd s0, 80(sp)
  addi s0, sp, 96

  # Line 12: return
  li a0, 31
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

