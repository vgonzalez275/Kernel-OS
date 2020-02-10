// main.c

#include "k-include.h"  // SPEDE includes
#include "k-entry.h"    // entries to kernel (TimerEntry, etc.)
#include "k-type.h"     // kernel data types
#include "tools.h"      // small handy functions
#include "k-sr.h"       // kernel service routines
#include "proc.h"       // all user process code here
#include "sys-call.h"

// kernel data are all here:
int run_pid;                        // current running PID; if -1, none selected
q_t pid_q, ready_q;                 // avail PID and those created/ready to run
pcb_t pcb[PROC_SIZE];               // Process Control Blocks
char proc_stack[PROC_SIZE][PROC_STACK_SIZE];   // process runtime stacks
struct i386_gate *intr_table;    // intr table's DRAM location
int sys_centi_sec;
q_t sleep_q;
mux_t mux[MUX_SIZE];
q_t mux_q;
int vid_mux;
term_t term[TERM_SIZE] = {
	{ TRUE, TERM0_IO_BASE },
	{ TRUE, TERM1_IO_BASE }
};
int page_user[PAGE_NUM];
unsigned rand = 0;
int kernel_main_table;

void InitKernelData(void) {         // init kernel data
   int i,j,k;
      
   kernel_main_table=get_cr3();

   intr_table = get_idt_base();            // get intr table location

   Bzero((char *)&pid_q,sizeof(q_t));                      // clear 2 queues
   Bzero((char *)&ready_q,sizeof(q_t));
   for(i=0;i<PROC_SIZE;i++) {
	EnQ(i,&pid_q);                        // put all PID's to pid queue
   }
   run_pid=NONE;			//set run_pid to NONE
   sys_centi_sec=0;
   Bzero((char *)&sleep_q,sizeof(q_t));

   Bzero((char *)&mux_q,sizeof(q_t));
   for(j=0;j<MUX_SIZE;j++) {
	EnQ(j,&mux_q);
   }
      for(k=0; k<PAGE_NUM; k++) {
	page_user[k]=NONE;
   }

}
void InitKernelControl(void) {      // init kernel control
   fill_gate(&intr_table[TIMER_INTR], (int)TimerEntry, get_cs(), ACC_INTR_GATE, 0);                  // fill out intr table for timer
   outportb(PIC_MASK, MASK);                   // mask out PIC for timer
   fill_gate(&intr_table[GETPID_CALL], (int)GetPidEntry, get_cs(), ACC_INTR_GATE, 0);
   fill_gate(&intr_table[SHOWCHAR_CALL], (int)ShowCharEntry, get_cs(), ACC_INTR_GATE, 0);
   fill_gate(&intr_table[SLEEP_CALL], (int)SleepEntry, get_cs(), ACC_INTR_GATE, 0);
   fill_gate(&intr_table[MUX_CREATE_CALL], (int)MuxCreateEntry, get_cs(), ACC_INTR_GATE, 0);
   fill_gate(&intr_table[MUX_OP_CALL], (int)MuxOpEntry, get_cs(), ACC_INTR_GATE, 0);
   fill_gate(&intr_table[TERM0_INTR], (int)Term0Entry, get_cs(), ACC_INTR_GATE, 0);
   fill_gate(&intr_table[TERM1_INTR], (int)Term1Entry, get_cs(), ACC_INTR_GATE, 0);
   fill_gate(&intr_table[FORK_CALL], (int)ForkEntry, get_cs(), ACC_INTR_GATE, 0);
   fill_gate(&intr_table[WAIT_CALL], (int)WaitEntry, get_cs(), ACC_INTR_GATE, 0);
   fill_gate(&intr_table[EXIT_CALL], (int)ExitEntry, get_cs(), ACC_INTR_GATE, 0);
   fill_gate(&intr_table[EXEC_CALL], (int)ExecEntry, get_cs(), ACC_INTR_GATE, 0);
   fill_gate(&intr_table[SIGNAL_CALL], (int)SignalEntry, get_cs(), ACC_INTR_GATE, 0);
   fill_gate(&intr_table[PAUSE_CALL], (int)PauseEntry, get_cs(), ACC_INTR_GATE, 0);
   fill_gate(&intr_table[KILL_CALL], (int)KillEntry, get_cs(), ACC_INTR_GATE, 0);
   fill_gate(&intr_table[RAND_CALL], (int)RandEntry, get_cs(), ACC_INTR_GATE, 0);
}

void Scheduler(void) {      // choose run_pid
   if(run_pid>0)
      return;
   if(QisEmpty(&ready_q))
      run_pid=0;
   else{
      pcb[0].state= READY;
      run_pid=DeQ(&ready_q);
   }
   /*if ready_q is empty:
      pick 0 as run_pid     // pick InitProc
   else:
      change state of PID 0 to ready
      dequeue ready_q to set run_pid*/

   pcb[run_pid].run_count =0;                    // reset run_count of selected proc
   pcb[run_pid].state=RUN;                    // upgrade its state to run
}

int main(void) {                          // OS bootstraps
   InitKernelData();                            // call to initialize kernel data
   InitKernelControl();				//call to initialize kernel control

   NewProcSR(InitProc);				//   call NewProcSR(InitProc) to create it  // create InitProc
   Scheduler();					//call Scheduler()
   set_cr3(pcb[run_pid].main_table);
   Loader(pcb[run_pid].trapframe_p);		//call Loader(pcb[run_pid].trapframe_p); // load/run it

   return 0; // statement never reached, compiler asks it for syntax
}

void Kernel(trapframe_t *trapframe_p) {           // kernel runs
   char ch;
   int wait;
   
   pcb[run_pid].trapframe_p = trapframe_p; // save it

   //TimerSR();                     // handle timer intr
  
   switch(trapframe_p->entry_id) {
   case TIMER_INTR:
      TimerSR();
      break;
   case SLEEP_CALL:
      SleepSR(trapframe_p->eax);
      break;
   case GETPID_CALL:
      trapframe_p->eax=GetPidSR();
      break;
   case SHOWCHAR_CALL:
      ShowCharSR(trapframe_p->eax, trapframe_p->ebx, trapframe_p->ecx);
      break;
   case MUX_CREATE_CALL:
      trapframe_p->eax = MuxCreateSR(trapframe_p->eax);
      break;
   case MUX_OP_CALL:
      MuxOpSR(trapframe_p->eax, trapframe_p->ebx);
      break;
   case TERM0_INTR:
      TermSR(0);
      outportb(PIC_CONTROL, TERM0_DONE);  
      //outportb(PIC_MASK, TERM0_DONE);  
      break;
   case TERM1_INTR:
      TermSR(1);
      outportb(PIC_CONTROL, TERM1_DONE); 
      //outportb(PIC_MASK, TERM1_DONE);  
      break;
   case FORK_CALL:
      trapframe_p->eax=ForkSR();
      break;
   case WAIT_CALL:
      wait=WaitSR();
      if(wait!=NONE) {
	trapframe_p->eax=wait;
      }
      break;
   case EXIT_CALL:
      ExitSR(trapframe_p->eax);
      break;
   case EXEC_CALL:
      ExecSR(trapframe_p->eax, trapframe_p->ebx);
      break;
   case SIGNAL_CALL:
      SignalSR(trapframe_p->eax, trapframe_p->ebx);
      break;
   case PAUSE_CALL:
      PauseSR();
      break;
   case KILL_CALL:
      KillSR(trapframe_p->eax,trapframe_p->ebx);
      break;
   case RAND_CALL:
      trapframe_p->eax=RandSR();
      break;
   default:
      cons_printf("Panic! EID= %i\n",trapframe_p->entry_id);
   }
   if(cons_kbhit()) {            // check if keyboard pressed
      ch = cons_getchar();
      if (ch == 'b') {                     // 'b' for breakpoint
         breakpoint();                           // let's go to GDB
         //break;
      }
      if (ch == 'n') {                     // 'n' for new process
	 if(rand==0) {	 
		rand=sys_centi_sec;
	 }
         NewProcSR(UserProc);     // create a UserProc
      }
   }
  
   Scheduler();    // may need to pick another proc
   set_cr3(pcb[run_pid].main_table);
   Loader(pcb[run_pid].trapframe_p);
}

