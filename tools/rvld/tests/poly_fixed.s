  .text
  .globl _start
_start:
  # Call main
  call main
  # Exit with returned code
  li a7, 93
  .word 0x73

  .data
  .globl vtable_Base
vtable_Base:
  .dword Base_show

  .globl vtable_Derived
vtable_Derived:
  .dword Derived_show

  .text
Base_show:
  li a0, 1
  jr ra

Derived_show:
  li a0, 2
  jr ra

  .globl main
main:
  # Local variable: Derived d at sp+0 (size 16 for safety)
  addi sp, sp, -16
  sd ra, 8(sp)
  
  # d.vptr = vtable_Derived
  la t0, vtable_Derived
  sd t0, 0(sp)
  
  # Base* b = &d;
  mv s1, sp # s1 = &d
  
  # Call b->show()
  # 1. Load vptr
  ld t1, 0(s1)
  # 2. Load method at index 0 (first dword)
  ld t2, 0(t1)
  # 3. Call it (passing 'this' in a0)
  mv a0, s1
  jalr ra, t2, 0
  
  # result is in a0 (should be 2)
  ld ra, 8(sp)
  addi sp, sp, 16
  jr ra
