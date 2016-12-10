/*
 * io.h - Definició de l'entrada/sortida per pantalla en mode sistema
 */

#ifndef __IO_H__
#define __IO_H__

#define TAM_BUFF	256

#include <list.h>
#include <types.h>

/** Screen functions **/
/**********************/

Byte inb (unsigned short port);
void printc(char c);
void printc_xy(Byte x, Byte y, char c);
void printk(char *string);

struct list_head keyboardqueue;

struct circularbuffer{
  char kbuff[TAM_BUFF];
  int size;
  int current;
};

struct circularbuffer keyboardbuffer;

void kbuff_init();
int kbuff_isEmpty();
int kbuff_isFull();
int kbuff_getsize();
void kbuff_pushchar(char c);
char kbuff_popchar();

#endif  /* __IO_H__ */
