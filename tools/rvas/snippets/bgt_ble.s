.text
.globl main
main:
  li t0, 5
  li t1, 3
  bgt t0, t1, L1
  li a0, 0
  j L2
L1:
  li a0, 1
L2:
  ble t1, t0, L3
  li a0, 2
L3:
  jr ra
