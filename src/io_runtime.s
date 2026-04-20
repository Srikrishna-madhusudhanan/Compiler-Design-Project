  .section .rodata
__io_stub_msg: .asciz "rvld: I/O STUB called\n"

  .text
  .globl _start
_start:
  # Initialize stack pointer (optional but good practice)
  # QEMU usually sets it up, but we could do it here if we had a __stack_top.
  # For now, we assume sp is already valid.
  call main
  # a0 already has main's return value
  call exit
  # Should not return

  .globl printf
printf:
  # Simple stub: just print a message that it was called.
  # Real printf is too complex for this minimal runtime.
  # In a real scenario, this would be provided by a real libc.
  li a7, 64          # sys_write
  li a0, 1           # stdout
  la a1, __io_stub_msg
  li a2, 22          # length
  ecall
  li a0, 0
  ret

  .globl scanf
scanf:
  li a0, 0
  ret

  .globl malloc
malloc:
  li a0, 0           # Return NULL
  ret

  .globl free
free:
  ret

  .globl exit
exit:
  li a7, 93          # sys_exit
  ecall
  ret
