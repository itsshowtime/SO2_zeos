/*
 * sys.c - Syscalls implementation
 */
#include <devices.h>

#include <utils.h>

#include <io.h>

#include <mm.h>

#include <mm_address.h>

#include <sched.h>

#include <errno.h>

#define LECTURA 0
#define ESCRIPTURA 1

int check_fd(int fd, int permissions)
{
  if (fd!=1) return -9; /*EBADF*/
  if (permissions!=ESCRIPTURA) return -13; /*EACCES*/
  return 0;
}

int sys_ni_syscall()
{
	return -ENOSYS;
}

int sys_getpid()
{
	return current()->PID;
}

int sys_fork()
{
  int PID=-1;

  // creates the child process
  
  return PID;
}

int sys_write(int fd, char *buffer, int size)
{
  //check fd
  int ret = check_fd(fd,ESCRIPTURA);  
  if(ret) return ret;
  //check buffer
  if(buffer == NULL) return -EFAULT;
  
  //check size
  if(size < 0)
    {
      return -EINVAL;
    }
  
  //write
  ret = sys_write_console(buffer, size);
  return ret;
}

int sys_gettime()
{
  return zeos_ticks;
}

void sys_exit()
{  
}

