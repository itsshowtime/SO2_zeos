#include <asm.h>
#include <segment.h>

ENTRY(sys_call_table)
      .long sys_ni_syscall // 0 nada
      .long sys_ni_syscall // 1 nada
      .long sys_ni_syscall // 2 nada
      .long sys_ni_syscall // 3 nada
      .long sys_write      // 4 write
      .long sys_ni_syscall // 5 nada
      .long sys_ni_syscall // 6 nada
      .long sys_ni_syscall // 7 nada
      .long sys_ni_syscall // 8 nada
      .long sys_ni_syscall // 9 nada
      .long sys_gettime    // 10 gettime

 .globl MAX_SYSCALL
 MAX_SYSCALL = (. - sys_call_table)/4
