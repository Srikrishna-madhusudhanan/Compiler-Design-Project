.text
.globl main
main:
  addi sp, sp, -16
  sd s0, 8(sp)
  addi s0, sp, 16
  li t0, 42
  sw t0, -28(s0)
  lw a0, -28(s0)
  ld ra, 8(sp)
  addi sp, sp, 16
  jr ra
