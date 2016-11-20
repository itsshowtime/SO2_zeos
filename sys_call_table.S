#include <asm.h>
#include <segment.h>

ENTRY (sys_call_table)
	.long sys_ni_syscall	//0
	.long sys_exit		//1 exit
	.long sys_fork		//2 fork
	.long sys_ni_syscall	//3
	.long sys_write		//4 write
	.long sys_ni_syscall	//5
	.long sys_ni_syscall	//6
	.long sys_ni_syscall	//7
	.long sys_ni_syscall	//8
	.long sys_ni_syscall	//9
	.long sys_gettime	//10 gettime
	.long sys_ni_syscall	//11
	.long sys_ni_syscall	//12
	.long sys_yield		//13 yield
	.long sys_ni_syscall	//14
	.long sys_ni_syscall	//15
	.long sys_ni_syscall	//16
	.long sys_ni_syscall	//17
	.long sys_ni_syscall	//18
	.long sys_clone		//19 clone
	.long sys_getpid	//20 getpid
	.long sys_sem_init	//21 sem_init
	.long sys_sem_wait	//22 sem_wait
	.long sys_sem_signal	//23 sem_signal
	.long sys_sem_destroy	//24 sem_destroy
	.long sys_ni_syscall	//25
	.long sys_ni_syscall	//26
	.long sys_ni_syscall	//27
	.long sys_ni_syscall	//28
	.long sys_ni_syscall	//29
	.long sys_ni_syscall	//30
	.long sys_ni_syscall	//31
	.long sys_ni_syscall	//32
	.long sys_ni_syscall	//33
	.long sys_ni_syscall	//34
	.long sys_get_stats	//35 get_stats
.globl MAX_SYSCALL
MAX_SYSCALL = (. - sys_call_table)/4
