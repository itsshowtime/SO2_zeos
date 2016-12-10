/*
 * io.c - 
 */

#include <io.h>

#include <types.h>

/**************/
/** Screen  ***/
/**************/

#define NUM_COLUMNS 80
#define NUM_ROWS    25

Byte x, y=19;

/* Read a byte from 'port' */
Byte inb (unsigned short port)
{
  Byte v;

  __asm__ __volatile__ ("inb %w1,%0":"=a" (v):"Nd" (port));
  return v;
}

void printc(char c)
{
     __asm__ __volatile__ ( "movb %0, %%al; outb $0xe9" ::"a"(c));
  if (c=='\n')
  {
    x = 0;
    y=(y+1)%NUM_ROWS;
  }
  else
  {
    Word ch = (Word) (c & 0x00FF) | 0x0200;
    DWord screen = 0xb8000 + (y * NUM_COLUMNS + x) * 2;
    if (++x >= NUM_COLUMNS)
    {
      x = 0;
      y=(y+1)%NUM_ROWS;
    }
    asm("movw %0, (%1)" : : "g"(ch), "g"(screen));
  }
}

void printc_xy(Byte mx, Byte my, char c)
{
  Byte cx, cy;
  cx=x;
  cy=y;
  x=mx;
  y=my;
  printc(c);
  x=cx;
  y=cy;
}

void printk(char *string)
{
  int i;
  for (i = 0; string[i]; i++)
    printc(string[i]);
}

void kbuff_init(){
  keyboardbuffer.size = 0;
  keyboardbuffer.current = 0;
}

int kbuff_isEmpty(){
  return (keyboardbuffer.size == 0);
}

int kbuff_isFull(){
  return (keyboardbuffer.size == TAM_BUFF);
}

int kbuff_getsize(){
  return keyboardbuffer.size;
}

void kbuff_pushchar(char c){
  if(!kbuff_isFull()){
  keyboardbuffer.kbuff[(keyboardbuffer.current+keyboardbuffer.size)%TAM_BUFF] = c;
  ++keyboardbuffer.size;
  }
}

char kbuff_popchar(){
  char c = keyboardbuffer.kbuff[keyboardbuffer.current];
  if (keyboardbuffer.current < TAM_BUFF-1) ++keyboardbuffer.current;
  else keyboardbuffer.current = 0;

  if(keyboardbuffer.size > 0) --keyboardbuffer.size;
  return c;
}
