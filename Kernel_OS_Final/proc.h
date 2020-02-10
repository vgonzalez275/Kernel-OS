// proc.h

#ifndef __K_PROC__
#define __K_PROC__

//#include "proc.c";
// prototype those in proc.c here
//void Delay(void);
//void ShowChar(int row, int col, char ch);
void InitTerm(int term_no);
void InitProc(void);
void Aout(int device);
void Ouch(int device);
void Wrapper(int handler, int arg);
void UserProc(void);

#endif
