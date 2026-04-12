.text
.globl main
main:
  li a0, 1
  li a1, 2
  blt a0, a1, L1
  li a0, 0
L1:
  li a0, 7
  jr ra
