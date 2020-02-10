// tools.h

#ifndef __K_LIB__
#define __K_LIB__

//#include "k-lib.c"

// prototype those in k-lib.c here
void Bzero(char *p, int bytes);
int QisEmpty(q_t *p);
int QisFull(q_t *p);
int DeQ(q_t *p);
void EnQ(int to_add, q_t *p);
void MemCpy(char *dst, char *src, int size);
int StrCmp(char *str1, char *str2);
void Itoa(char *str, int x);

#endif
