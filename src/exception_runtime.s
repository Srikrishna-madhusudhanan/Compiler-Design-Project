  .data
  .globl __paninic_try_stack_ptr
__paninic_try_stack_ptr:
  .dword 0

  .section .bss
__paninic_try_stack:
  .space 2048

  .text
  # __paninic_push_try_context
  # Saves current register state and returns 0.
  .globl __paninic_push_try_context
__paninic_push_try_context:
  la t0, __paninic_try_stack_ptr
  ld t1, 0(t0)
  
  la t2, __paninic_try_stack
  add t2, t2, t1
  
  sd s0, 0(t2)
  sd s1, 8(t2)
  sd s2, 16(t2)
  sd s3, 24(t2)
  sd s4, 32(t2)
  sd s5, 40(t2)
  sd s6, 48(t2)
  sd s7, 56(t2)
  sd s8, 64(t2)
  sd s9, 72(t2)
  sd s10, 80(t2)
  sd s11, 88(t2)
  sd ra, 96(t2)
  sd sp, 104(t2)
  
  addi t1, t1, 112
  sd t1, 0(t0)
  
  li a0, 0
  ret

  # __paninic_pop_try_context
  # Simply pops the last saved context without jumping.
  .globl __paninic_pop_try_context
__paninic_pop_try_context:
  la t0, __paninic_try_stack_ptr
  ld t1, 0(t0)
  addi t1, t1, -112
  sd t1, 0(t0)
  ret

  # __paninic_throw
  # Restores the last saved context and returns the exception value (a0).
  .globl __paninic_throw
__paninic_throw:
  # If stack is empty, we should probably abort, but let's assume valid catch for now.
  la t0, __paninic_try_stack_ptr
  ld t1, 0(t0)
  addi t1, t1, -112
  sd t1, 0(t0)
  
  la t2, __paninic_try_stack
  add t2, t2, t1
  
  ld s0, 0(t2)
  ld s1, 8(t2)
  ld s2, 16(t2)
  ld s3, 24(t2)
  ld s4, 32(t2)
  ld s5, 40(t2)
  ld s6, 48(t2)
  ld s7, 56(t2)
  ld s8, 64(t2)
  ld s9, 72(t2)
  ld s10, 80(t2)
  ld s11, 88(t2)
  ld ra, 96(t2)
  ld sp, 104(t2)
  
  # a0 already contains the exception value passed by the compiler.
  # Returning now effectively returns from __paninic_push_try_context with a0 != 0.
  ret
