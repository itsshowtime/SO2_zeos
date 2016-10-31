#include <libc.h>

char buff[24];

int pid;

long inner(long n)
{

  int i;
  long suma;
  suma = 0;
  for (i=0; i<n; i++) suma = suma + i;
  return suma;

}

long outer(long n)
{

  int i;
  long acum;
  acum = 0;
  for (i=0; i<n; i++) acum = acum + inner(i);
  return acum;

}

int add(int par1,int par2);

int __attribute__ ((__section__(".text.main")))
  main(void)
{
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
    /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */
//  long count, acum;
//  count = 75;
//  acum = 0;
//  acum = outer(count);
//runjp_rank(int first, int last);
  runjp_rank(0,2);
  runjp_rank(7,7);
  runjp_rank(18,19);
  runjp_rank(23,24);
  runjp_rank(28,30);
// 11 de 32 tests OK 
  while(1);
  return 0;
}
