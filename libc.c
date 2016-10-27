/*
 * libc.c 
 */

#include <libc.h>

#include <types.h>

int errno;

void itoa(int a, char *b)
{
  int i, i1;
  char c;
  
  if (a==0) { b[0]='0'; b[1]=0; return ;}
  
  i=0;
  while (a>0)
  {
    b[i]=(a%10)+'0';
    a=a/10;
    i++;
  }
  
  for (i1=0; i1<i/2; i1++)
  {
    c=b[i1];
    b[i1]=b[i-i1-1];
    b[i-i1-1]=c;
  }
  b[i]=0;
}

int strlen(char *a)
{
  int i;
  
  i=0;
  
  while (a[i]!=0) i++;
  
  return i;
}

void perror()
{
  char errstring[256];
  itoa(errno, errstring);
  write(1,errstring,strlen(errstring));
}

int write(int fd, char *buffer, int size)
{
  int ret;

  asm
  (
    "int $0x80\n\t"
    : "=a" (ret)
    : "a" (0x04), "b" (fd), "c" (buffer), "d" (size)

  );

  if(ret < 0)
  {
  errno = -ret;
  return -1;
  }

  return ret;
}

int gettime()
{
  int ret;
  asm
  (
    "int $0x80\n\t"
    : "=a" (ret)
    : "a" (0x0A)
  );

  return ret;
}

int getpid(void)
{
  int ret;
  asm
  (
    "int $0x80\n\t"
    : "=a" (ret)
    : "a" (0x14)
  );  

  return ret;
}

int fork(void)
{
  int ret;
  asm
  (
    "int $0x80\n\t"
    : "=a" (ret)
    : "a" (0x02)
  );

  if(ret < 0)
  {
  errno = -ret;
  return -1;
  }

  return ret;
}
