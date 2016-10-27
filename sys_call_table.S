#include <asm.h>
#include <segment.h>

ENTRY(sys_call_table)
      .long sys_ni_syscall // 0 nada
      .long sys_ni_syscall // 1 nada
      .long sys_fork	   // 2 fork
      .long sys_ni_syscall // 3 nada
      .long sys_write      // 4 write
      .long sys_ni_syscall // 5 nada
      .long sys_ni_syscall // 6 nada
      .long sys_ni_syscall // 7 nada
      .long sys_ni_syscall // 8 nada
      .long sys_ni_syscall // 9 nada
      .long sys_gettime    // 10 gettime
      .long sys_ni_syscall // 11 nada
      .long sys_ni_syscall // 12 nada
      .long sys_ni_syscall // 13 nada
      .long sys_ni_syscall // 14 nada
      .long sys_ni_syscall // 15 nada
      .long sys_ni_syscall // 16 nada
      .long sys_ni_syscall // 17 nada
      .long sys_ni_syscall // 18 nada
      .long sys_ni_syscall // 19 nada
      .long sys_getpid	   // 20 getpid


 .globl MAX_SYSCALL
 MAX_SYSCALL = (. - sys_call_table)/4
