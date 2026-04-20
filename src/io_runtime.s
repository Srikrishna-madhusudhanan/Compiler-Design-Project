  .section .text
  .globl _start
_start:
  # a0 = argc, a1 = argv, a2 = envp  (loaded by QEMU)
  # Pass argc/argv to main
  call main
  # a0 = return value from main
  li   a7, 93         # __NR_exit
  ecall

  # Syscall wrappers for minilib.c
  .globl __sys_read
__sys_read:
  li a7, 63           # __NR_read
  ecall
  ret

  .globl __sys_write
__sys_write:
  li a7, 64           # __NR_write
  ecall
  ret

  .globl __sys_brk
__sys_brk:
  li a7, 214          # __NR_brk
  ecall
  ret

  .globl __sys_exit
__sys_exit:
  li a7, 93           # __NR_exit
  ecall
  ret
