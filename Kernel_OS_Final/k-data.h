// k-data.h
// kernel data are all declared in main.c during bootstrap
// kernel .c code reference them as 'extern'

#ifndef __K_DATA__
#define __K_DATA__

#include "k-const.h"           // defines PROC_SIZE, PROC_STACK_SIZE
#include "k-type.h"            // defines q_t, pcb_t, ...

extern int run_pid;            // PID of running process
                            // prototype the rest...
extern q_t pid_q, ready_q;
extern pcb_t pcb[PROC_SIZE];
extern char proc_stack[PROC_SIZE][PROC_STACK_SIZE];
extern q_t sleep_q;
extern int sys_centi_sec;
extern int vid_mux;
extern q_t mux_q;
extern mux_t mux[MUX_SIZE];
extern term_t term[TERM_SIZE];
extern int page_user[PAGE_NUM];
extern unsigned rand;
extern int kernel_main_table;

#endif                         // endif of ifndef
