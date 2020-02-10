// k-type.h, 159
// OS phase 9
//
// Team Name: BJOS (Members: Brian Huang, Jupp Valdez, Veronica Gonzalez)

#ifndef __K_TYPE__
#define __K_TYPE__

#include "k-const.h"

typedef void (*func_p_t)(void); // void-return function pointer type
typedef void (*func_p_t2)(int);

typedef enum {UNUSED, READY, RUN, SLEEP, SUSPEND, ZOMBIE, WAIT, PAUSE} state_t;

typedef struct {
   //unsigned int reg[8];
   unsigned int edi, esi, ebp, esp, ebx, edx, ecx, eax, entry_id;
   unsigned int eip;
   unsigned int cs;
   unsigned int efl;

  
} trapframe_t;

typedef struct {
   state_t state;                       // read in 1.html
   int run_count;
   int total_count;
   trapframe_t *trapframe_p;
   int wake_centi_sec;
   int ppid;
   int sigint_handler;
   int main_table;
} pcb_t;                     

typedef struct {             // generic queue type
                             // for a simple queue
  int current_size;
  int head,tail; 
  int q[Q_SIZE];
} q_t;

typedef struct {
   int flag;
   int creator;
   q_t suspend_q;
} mux_t;

typedef struct {
   int tx_missed;
   int io_base;
   int out_mux;
   q_t out_q;
   q_t in_q;
   q_t echo_q;
   int in_mux;
} term_t;

#endif
