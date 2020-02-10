// k-sr.c, 159
// OS phase 9
//
// Team Name: BJOS (Members: Brian Huang, Jupp Valdez, Veronica Gonzalez)

#include "k-include.h"
#include "k-type.h"
#include "k-data.h"
#include "tools.h"
#include "k-sr.h"
#include "sys-call.h"
#include "proc.h"

// to create a process: alloc PID, PCB, and process stack
// build trapframe, initialize PCB, record PID to ready_q
void NewProcSR(func_p_t p) {  // arg: where process code starts
   int pid;

   if( pid_q.tail == 0) {     // may occur if too many been created
      cons_printf("Panic: no more process!\n");
      breakpoint();                     // cannot continue, alternative: breakpoint();
      return;
   }

   pid=DeQ(&pid_q);                                       // alloc PID (1st is 0)
   Bzero((char *)&pcb[pid],sizeof(pcb_t));                                       // clear PCB
   Bzero((char * )&proc_stack[pid][0],PROC_STACK_SIZE);                                       // clear stack
   //sizeof(proc_stack[pid][0])
   pcb[pid].state=READY;                                       // change process state

   if(pid > 0)
	EnQ(pid,&ready_q);                           // queue to ready_q if > 0

// point trapframe_p to stack & fill it out
   pcb[pid].trapframe_p = (trapframe_t *)&proc_stack[pid][PROC_STACK_SIZE];               // point to stack top
   pcb[pid].trapframe_p--;                   // lower by trapframe size
   pcb[pid].trapframe_p->efl = EF_DEFAULT_VALUE|EF_INTR; // enables intr
   pcb[pid].trapframe_p->cs = get_cs();                  // dupl from CPU
   pcb[pid].trapframe_p->eip =(int) p;                          // set to code
   pcb[pid].main_table = kernel_main_table;
}

void CheckWakeProc(void) {
   int numproc=sleep_q.tail;
   int i, testpid;
   
   //cons_printf("pid_q: %i, ready_q: %i, sleep_q: %i\n,",pid_q.tail,ready_q.tail,sleep_q.tail);
   for(i=0; i<numproc; i++) {
      testpid=DeQ(&sleep_q);   
      //cons_printf("testpid: %d\n", testpid);
      //cons_printf("numproc: %i, pid: %i, w_c_s: %i, s_c_s: %i\n",numproc,testpid,pcb[testpid].wake_centi_sec,sys_centi_sec);
      if(testpid > 19) {
	return;
      }
      else {
      	if(pcb[testpid].wake_centi_sec==sys_centi_sec) {
            EnQ(testpid,&ready_q);
	    pcb[testpid].state=READY;
         }
         else { 
            EnQ(testpid,&sleep_q);
         } 
      }       
   }

}

// count run_count and switch if hitting time slice
void TimerSR(void) {
   outportb(PIC_CONTROL, TIMER_DONE);                              // notify PIC timer done
   sys_centi_sec++;
   CheckWakeProc();
   if(run_pid==0) {
      return;
   }
  
   pcb[run_pid].run_count++;                                       // count up run_count
   pcb[run_pid].total_count++;                                     // count up total_count
 
   
   
   if(pcb[run_pid].run_count==TIME_SLICE) {       // if runs long enough
      pcb[run_pid].state=READY;                                    // move it to ready_q
      EnQ(run_pid, &ready_q);                                    // change its state
      run_pid=NONE;                                    // running proc = NONE
   }
   
}

int GetPidSR(void) {
   return run_pid;
}

void SleepSR(int centi_sec) {
   pcb[run_pid].wake_centi_sec = sys_centi_sec + centi_sec;
   pcb[run_pid].state=SLEEP;
   EnQ(run_pid,&sleep_q);
   run_pid=NONE;
}

void ShowCharSR(int row, int col, char ch) {
   unsigned short *p = VID_HOME;   // upper-left corner of display
   p += row * 80 + col;
   *p = ch + VID_MASK;
}

int MuxCreateSR(int flag){
   int ID;
   if(QisEmpty(&mux_q)){
       cons_printf("Mux limit reached!\n");
       return -1;
   }
   ID = DeQ(&mux_q);
   mux[ID].flag=flag;
   mux[ID].creator=run_pid;
   Bzero((char *)&mux[ID].suspend_q,sizeof(q_t));
   return ID;
   //allocate mutex from mux_q, empty the mutex and set the 
   //flag and creater then return mux_id via a register traframe_p->eax?
}

void MuxOpSR(int mux_id, int opcode){

   if(opcode==LOCK){
       if(mux[mux_id].flag>0){
           mux[mux_id].flag--;
       }else{
           EnQ(run_pid,&mux[mux_id].suspend_q);
           pcb[run_pid].state=SUSPEND;
           run_pid=NONE;
       }
   }else if(opcode==UNLOCK){
       if(QisEmpty(&mux[mux_id].suspend_q)){
           mux[mux_id].flag++;
       }else{
           int released_PID = DeQ(&mux[mux_id].suspend_q);
           pcb[released_PID].state = READY;
           EnQ(released_PID,&ready_q);
           //need to change run_pid here?
       }
   }else{
       cons_printf("Invalid opcode: %i\n", opcode);
   }
   //if opcode is LOCK && flag>0, 
   	//decrement the flag in the mutex by 1.
   //else
   	//suspend the process by 
   	//1. queue the PID of the calling process to suspend_q in the mutex
   	//2. alter state to SUSPEND and,
   	//3. reset running pid run_pid=NONE;
   //if opcode to MuxOpCall() is UNLOCK, then 
   	//if no suspended process is in the suspend_q queue of the mutex
   	//	then increment flag by 1.
   	//else release the 1st PID in the suspend_q
   	//	move it to the ready_q
   	//	update its state to READY.
}

void TermSR(int term_no) {
   int event;
   event = inportb(term[term_no].io_base+IIR);
   if(event==TXRDY) {
	TermTxSR(term_no);
   }
   else if(event==RXRDY) {
	        
	TermRxSR(term_no);
   }
   if(term[term_no].tx_missed==TRUE) {
	TermTxSR(term_no);
   }
}
void TermTxSR(int term_no) {
   char first;
   if(QisEmpty(&term[term_no].echo_q) && QisEmpty(&term[term_no].out_q)) { 
	term[term_no].tx_missed=TRUE;
	return;
   }
   if (!(QisEmpty(&term[term_no].echo_q))){
	first=DeQ(&term[term_no].echo_q);
        outportb(term[term_no].io_base+DATA, first);
	term[term_no].tx_missed=FALSE;
	//MuxOpSR(term[term_no].out_mux, UNLOCK);		
   }else{
	first=DeQ(&term[term_no].out_q);
        outportb(term[term_no].io_base+DATA, first);
	term[term_no].tx_missed=FALSE;
	MuxOpSR(term[term_no].out_mux, UNLOCK);
   }
}
/*	OLD - BEFORE PHASE 5
void TermTxSR(int term_no) {
   char first;
   if(QisEmpty(&term[term_no].out_q)) { 
	term[term_no].tx_missed=TRUE;
	return;
   }
   else {
	first=DeQ(&term[term_no].out_q);
        outportb(term[term_no].io_base+DATA, first);
	term[term_no].tx_missed=FALSE;
	MuxOpSR(term[term_no].out_mux, UNLOCK);
   }
}
*/

void TermRxSR(int term_no) {
   char i;
   int suspend_pid, device;
   i=inportb(term[term_no].io_base+DATA);
   EnQ(i,&term[term_no].echo_q);

   	
   if(i==SIGINT && !QisEmpty(&mux[term[term_no].in_mux].suspend_q)  &&  pcb[mux[term[term_no].in_mux].suspend_q.q[0]].sigint_handler>0) {
	suspend_pid=mux[term[term_no].in_mux].suspend_q.q[0];
	if(term_no==0) {
		device=TERM0_INTR;
   	}
   	else if(term_no==1) {
		device=TERM1_INTR;
	}
	WrapperSR(suspend_pid, pcb[suspend_pid].sigint_handler, device);
   }

   if(i=='\r') {
	EnQ('\n', &term[term_no].echo_q);
   }
   if(i=='\r') {
	EnQ('\0', &term[term_no].in_q);
   }
   else {
	EnQ(i, &term[term_no].in_q);
   }  
   MuxOpSR(term[term_no].in_mux, UNLOCK);
}

int ForkSR(void) {
   int child_pid;
   int *p;
   int diff;

   //pcb_t child[PROC_SIZE];
   if(pid_q.tail==0) {
	cons_printf("Panic! - Out of PIDs\n");
	return NONE;
   }
   child_pid=DeQ(&pid_q);
   Bzero((char *)&pcb[child_pid],sizeof(pcb_t));
   Bzero((char *)&proc_stack[child_pid],PROC_STACK_SIZE);
   pcb[child_pid].state=READY;
   pcb[child_pid].ppid=run_pid;
   EnQ(child_pid,&ready_q);


   diff=(int)&proc_stack[child_pid][0]-(int)&proc_stack[run_pid][0];


   pcb[child_pid].trapframe_p=(trapframe_t *)((int)pcb[run_pid].trapframe_p + diff);
   MemCpy((char *)&proc_stack[child_pid][0] , (char *)&proc_stack[run_pid][0] , PROC_STACK_SIZE);
   pcb[child_pid].trapframe_p->eax=0;
   pcb[child_pid].trapframe_p->esp=pcb[run_pid].trapframe_p->esp + diff;
   pcb[child_pid].trapframe_p->ebp=pcb[run_pid].trapframe_p->ebp + diff;
   pcb[child_pid].trapframe_p->esi=pcb[run_pid].trapframe_p->esi + diff;
   pcb[child_pid].trapframe_p->edi=pcb[run_pid].trapframe_p->edi + diff;


   p=(int *)pcb[child_pid].trapframe_p->ebp;
   while(*p!=0) {
	*p= *p + diff;
        p=(int *)*p;
   }
   pcb[child_pid].main_table = kernel_main_table;
   return child_pid;
   /*g. get the difference between the locations of the child's stack and the parent's

   h. copy the parent's trapframe_p to the child's trapframe_p
      (with the location adjustment applied to the new trapframe_p)

   i. copy the parent's proc stack to the child (use your own MemCpy())
   j. set the eax in the new trapframe to 0 (child proc gets 0 from ForkCall)
   k. apply the location adjustment to esp, ebp, esi, edi in the new trapframe
   (nested base ptrs adjustment:)
   l. set an integer pointer p to ebp in the new trapframe
   m. while (what p points to is not 0) {
         1. apply the location adjustment to the value pointed
         2. set p to the adjusted value (need a typecast)
      }
   n. return child PID*/
}

int WaitSR(void) {                            // parent waits
   int i,j,found,pid,exitc;
   found=0;
   for(i=0;i<PROC_SIZE;i++) {
	if(pcb[i].ppid==run_pid && pcb[i].state==ZOMBIE) {
		pid=i;
		found++;
		break;
	}
   }
   if(found==0) {
	pcb[run_pid].state=WAIT;
        run_pid=NONE;
        return NONE;
   }
   set_cr3(pcb[pid].main_table);
   exitc=pcb[pid].trapframe_p->eax;
   set_cr3(pcb[run_pid].main_table);
   pcb[pid].state=UNUSED;
   EnQ(pid, &pid_q);
   for(j = 0; j < PAGE_NUM; j++){
      if(page_user[j] == pid){
	 page_user[j] = NONE;
      }
   }
   return exitc;

   /*loop thru the PCB array (looking for a ZOMBIE child):
      the proc ppid is run_pid and the state is ZOMBIE -> break the loop

   if the whole PCB array was looped thru (didn't find any ZOMBIE child):
      1. alter the state of run_pid to ...
      2. set run_pid to ...
      3. return NONE

   get its exit code (from the eax of the child's trapframe)

   reclaim child's resources: 
      1. alter its state to ...
      2. enqueue its PID to ...

   return the exit code*/
}

void ExitSR(int exit_code) {                  // child exits
   int i;
   if(pcb[pcb[run_pid].ppid].state!=WAIT) {
	pcb[run_pid].state=ZOMBIE;
	run_pid=NONE;
	return;
   }
   pcb[pcb[run_pid].ppid].state=READY;
   EnQ(pcb[run_pid].ppid, &ready_q);
   pcb[pcb[run_pid].ppid].trapframe_p->eax=exit_code;
   pcb[run_pid].state=UNUSED;
   EnQ(run_pid, &pid_q);
   for(i = 0; i < PAGE_NUM; i++){
      if(page_user[i] == run_pid){
	 page_user[i] = NONE;
      }
   }
   run_pid=NONE;
   set_cr3(kernel_main_table);


   /*if the process state of my parent (ppid) is not WAIT:
      1. alter my state to ...
      2. reset run_pid to ...
      3. return

   alter the state of ppid to ...
   enqueue ppid to ...
   don't forget to pass it the exit code (via eax of parent's trapframe)

   reclaim child's resources:
      1. alter its state to ...
      2. enqueue its PID to ...
      3. reset run_pid to ...*/
}

void ExecSR(int code, int arg) {

   int i, j, pages[5], *p, entry;
   trapframe_t *q;
   enum {MAIN_TABLE, CODE_TABLE, STACK_TABLE, CODE_PAGE, STACK_PAGE};
   j=0;
   //1. allocate 5 RAM pages
   for(i=0; i<PAGE_NUM; i++) {
      if(page_user[i] == NONE) {
         pages[j]=i;
         j++;
	 page_user[i] = run_pid;
      }
      if(j==5) {
	break;
      }
   }
   if(j!=5) {
      cons_printf("Panic - Out of pages! Only got %i out of 5 Indices\n",j);
      breakpoint();
      return;
   }
   pages[MAIN_TABLE] = (pages[0] * PAGE_SIZE) + RAM;
   pages[CODE_TABLE] = (pages[1] * PAGE_SIZE) + RAM;
   pages[STACK_TABLE] = (pages[2] * PAGE_SIZE) + RAM;
   pages[CODE_PAGE] = (pages[3] * PAGE_SIZE) + RAM;
   pages[STACK_PAGE] = (pages[4] * PAGE_SIZE) + RAM;

   //2. build code page
   MemCpy((char*)pages[CODE_PAGE], (char *)code, PAGE_SIZE);

   //3. build stack page
   Bzero((char *)pages[STACK_PAGE], PAGE_SIZE); 
   p = (int*)(pages[STACK_PAGE] + PAGE_SIZE);
   p--;
   *p = arg;
   p--;

   q = (trapframe_t *)p;
   q--;
   q->efl = EF_DEFAULT_VALUE|EF_INTR; 	 // enables intr
   q->cs = get_cs();                 	 // dupl from CPU
   q->eip = (unsigned int)M256;

   //4. build addr-trans main table
   Bzero((char *)pages[MAIN_TABLE], PAGE_SIZE);
   MemCpy((char *)pages[MAIN_TABLE], (char *)kernel_main_table, sizeof(int[4]));
   entry = M256 >> 22;		//32-22=10 bits
   *((int*)pages[MAIN_TABLE] + entry) = pages[CODE_TABLE] | PRESENT | RW;
   entry = G1_1 >> 22;
   *((int*)pages[MAIN_TABLE] + entry) = pages[STACK_TABLE] | PRESENT | RW;


   //5. build code table
   Bzero((char *)pages[CODE_TABLE], PAGE_SIZE);
   entry = (M256 & MASK10) >> 12;
   *((int*)pages[CODE_TABLE] + entry) = pages[CODE_PAGE] | PRESENT | RW;

   //6. build stack table
   Bzero((char *)pages[STACK_TABLE], PAGE_SIZE);
   entry = (G1_1 & MASK10) >> 12;  
   *((int*)pages[STACK_TABLE] + entry) = pages[STACK_PAGE] | PRESENT | RW;

   //7.
   pcb[run_pid].main_table=pages[MAIN_TABLE];
   pcb[run_pid].trapframe_p=(trapframe_t *)V_TF;

   /*
   int code_page, stack_page;
   int * codeaddr;
   int * stackaddr;
   code_page=NONE;
   stack_page=NONE;
   for(i = 0; i < PAGE_NUM; i++){
      if(page_user[i] == NONE) {
         if(code_page == NONE) {
            code_page = i;
            continue;
         }
         else if(stack_page == NONE) {
            stack_page = i;
         }
         
         if(code_page != NONE && stack_page != NONE) {
            page_user[code_page] = run_pid;
            page_user[stack_page] = run_pid;
            break;
         }
      }
   }
   if(code_page == NONE || stack_page == NONE) {
	cons_printf("Panic - Out of pages!\n");
	return;
   }
   codeaddr=(int*)((code_page * PAGE_SIZE) + RAM);
   stackaddr=(int*)((stack_page * PAGE_SIZE) + RAM); 
   MemCpy((char *) codeaddr, (char *)code, PAGE_SIZE);
   Bzero((char *) stackaddr, PAGE_SIZE);
   //Skip a whole 4 Bytes
   p = (int *)((int)stackaddr + PAGE_SIZE);
   p--;
   *p = arg;
   p--;
   pcb[run_pid].trapframe_p = (trapframe_t *)p;
   pcb[run_pid].trapframe_p--;
   pcb[run_pid].trapframe_p->efl = EF_DEFAULT_VALUE|EF_INTR; // enables intr
   pcb[run_pid].trapframe_p->cs = get_cs();                  // dupl from CPU
   pcb[run_pid].trapframe_p->eip = (unsigned int)codeaddr;  */
}

void SignalSR(int sig_num, int handler) {
   pcb[run_pid].sigint_handler=handler;
}

void WrapperSR(int pid, int handler, int arg) {
   int *p;
   p=(int *)((int)pcb[pid].trapframe_p + sizeof(trapframe_t));
   MemCpy((char *)((int)pcb[pid].trapframe_p - sizeof(int[3])), (char *)((int)pcb[pid].trapframe_p), sizeof(trapframe_t));
   pcb[pid].trapframe_p = (trapframe_t *)((int)pcb[pid].trapframe_p - sizeof(int[3]));
   *p = pcb[pid].trapframe_p->eip;
   p--;
   *p = arg;
   p--;
   *p = handler;
   pcb[pid].trapframe_p->eip = (int)Wrapper;
}


void PauseSR(void) {
      pcb[run_pid].state=PAUSE;
      run_pid=NONE;

      /*a. alter the state of the running process
      b. the running process is now NONE*/
}

void KillSR(int pid, int sig_num) {
   int i;
   if(pid==0) {
      for(i=0; i<PROC_SIZE; i++ ) {
         if(run_pid==pcb[i].ppid && pcb[i].state==PAUSE) {
	    pcb[i].state=READY;
	    EnQ(i, &ready_q);
	 }
      }
   }
      /*if pid is 0 {
         loop thru PCB array {
            look for ppid is the running process and state is PAUSE
            if found: a. update its state to ? b. enqueue it to ?
	*/
}
unsigned RandSR(void) {
      rand = run_pid * rand + A_PRIME;
      rand %= G2;
      return rand;
}
