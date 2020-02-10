// sys-call.c
// calls to kernel services

#include "k-const.h"
#include "sys-call.h"
#include "k-type.h"
#include "k-data.h"
#include "k-sr.h"
#include "tools.h"

int GetPidCall(void) {
   int pid;

   asm("int %1;             
        movl %%eax, %0"     // after, copy eax to variable 'pid'
       : "=g" (pid)         // output
       : "g" (GETPID_CALL)  // input
       : "eax"              // used registers
   );

   return pid;
}

void ShowCharCall(int row, int col, char ch) {
   asm("movl %0,%%eax;
        movl %1,%%ebx;  
        movl %2,%%ecx;
        int %3"             // initiate call, %3 gets entry_id
       :                    // no output
       : "g" (row), "g" (col), "g" (ch), "g" (SHOWCHAR_CALL)
       : "eax", "ebx" ,"ecx"        // affected/used registers
   );
}

void SleepCall(int centi_sec) {  // # of 1/100 of a second to sleep
   asm("movl %0, %%eax;
        int %1"
       :
       :"g" (centi_sec),"g" (SLEEP_CALL)
       :"eax"
   );  
}


//structurally , coded as same way as phase 2
int MuxCreateCall(int flag){
 // has input and output, requires combining asm()
 // syntax from GetPidCall and SleepCall.
   int id;
   asm("movl %1, %%eax;
	int %2;
	movl %%eax, %0" 
   	:"=g"(id)
   	:"g"(flag),"g"(MUX_CREATE_CALL)
   	:"eax" 
   );
   return id;
}

//structurally , coded as same way as phase 2
void MuxOpCall(int mux_id, int opcode){
  asm("movl %0, %%eax;
       movl %1, %%ebx;
       int %2"
       :
       :"g"(mux_id),"g"(opcode),"g"(MUX_OP_CALL)
       :"eax","ebx"  
       );
} 

void WriteCall(int device, char *str) {
   int i,colu;
   i=GetPidCall();
   colu=0;
   if(device==STDOUT) {
	while(*str!='\0') {
	    ShowCharCall(i, colu, *str);
            str++;
            colu++;
        }
   } else {
	int term_no;
	if(device==TERM0_INTR) {
	   term_no=0;
	}
	else if(device==TERM1_INTR) {
	   term_no=1;
	}
	while(*str!='\0') {
	   MuxOpCall(term[term_no].out_mux, LOCK);
	   EnQ((int)*str, &term[term_no].out_q);
	   if(device==TERM0_INTR) {
		asm("int $35");
	   }
	   else {
		asm("int $36");
	   }
	   str++;
	}
   }        
}

void ReadCall(int device, char *str) {
   char i;
   int term_no;
   int char_count;
   if(device==TERM0_INTR) {
	term_no=0;
   }
   else if(device==TERM1_INTR) {
	term_no=1;
   }
   char_count=0;
   while(1) { 
	MuxOpCall(term[term_no].in_mux, LOCK);
	i=DeQ(&term[term_no].in_q);
	*str=i;
	if(i=='\0') { 
	   return;
	}
	str++;
	char_count++;
	if(char_count==STR_SIZE) {
	   *str='\0';
	   return;
	}
	if(device==TERM0_INTR) {
	   asm("int $35");
	}
	else {
	   asm("int $36");
	}
   }	
}

int ForkCall(void) {
   int pid;
   asm("int %1;             
        movl %%eax, %0"     // after, copy eax to variable 'pid'
       : "=g" (pid)         // output
       : "g" (FORK_CALL)  // input
       : "eax"              // used registers
   );
   return pid;
}

int WaitCall(void) {
   int exitc;
   asm("int %1;             
        movl %%eax, %0"     // after, copy eax to variable 'exitc'
       : "=g" (exitc)         // output
       : "g" (WAIT_CALL)  // input
       : "eax"              // used registers
   );
   return exitc;

}

void ExitCall(int exit_code) {
   asm("movl %0, %%eax;
        int %1"
       :
       :"g" (exit_code),"g" (EXIT_CALL)
       :"eax"
   );  
}

void ExecCall(int code, int arg) {
  asm("movl %0, %%eax;
       movl %1, %%ebx;
       int %2"
       :
       :"g"(code),"g"(arg),"g"(EXEC_CALL)
       :"eax","ebx"  
       );
}

void SignalCall(int sig_num, int handler) {
  asm("movl %0, %%eax;
       movl %1, %%ebx;
       int %2"
       :
       :"g"(sig_num),"g"(handler),"g"(SIGNAL_CALL)
       :"eax","ebx"  
       );
}

void PauseCall(void) {
   asm("int %0;"             
       : 
       : "g" (PAUSE_CALL)  // input
     
   );
}

void KillCall(int pid, int sig_num) {
  asm("movl %0, %%eax;
       movl %1, %%ebx;
       int %2"
       :
       :"g"(pid),"g"(sig_num),"g"(KILL_CALL)
       :"eax","ebx"  
  );
}

unsigned RandCall(void) {
   unsigned rand;
   asm("int %1;             
        movl %%eax, %0"     // after, copy eax to variable 'pid'
       : "=g" (rand)         // output
       : "g" (RAND_CALL)  // input
       : "eax"              // used registers
   );
   return rand;
}
