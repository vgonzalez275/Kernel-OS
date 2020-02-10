// proc.c
// all user processes are coded here
// processes do not R/W kernel data or code, must use sys-calls

#include "k-const.h"   // LOOP
#include "sys-call.h"  // all service calls used below
#include "k-data.h"
#include "k-sr.h"
#include "tools.h"      // <------------------------------ NEW!!!
#include "k-include.h"  // <------------------------------ NEW!!!

void InitTerm(int term_no) {  // <------------------------------ NEW!!!
   int i, j;

   Bzero((char *)&term[term_no].out_q, sizeof(q_t));
   Bzero((char *)&term[term_no].in_q, sizeof(q_t));      // <------------- new
   Bzero((char *)&term[term_no].echo_q, sizeof(q_t));    // <------------- new
   term[term_no].out_mux = MuxCreateCall(Q_SIZE);
   term[term_no].in_mux = MuxCreateCall(0);              // <------------- new

   outportb(term[term_no].io_base+CFCR, CFCR_DLAB);             // CFCR_DLAB is 0x80
   outportb(term[term_no].io_base+BAUDLO, LOBYTE(115200/9600)); // period of each of 9600 bauds
   outportb(term[term_no].io_base+BAUDHI, HIBYTE(115200/9600));
   outportb(term[term_no].io_base+CFCR, CFCR_PEVEN|CFCR_PENAB|CFCR_7BITS);

   outportb(term[term_no].io_base+IER, 0);
   outportb(term[term_no].io_base+MCR, MCR_DTR|MCR_RTS|MCR_IENABLE);
   for(i=0; i<LOOP/2; i++)asm("inb $0x80");
   outportb(term[term_no].io_base+IER, IER_ERXRDY|IER_ETXRDY); // enable TX & RX intr
   for(i=0; i<LOOP/2; i++)asm("inb $0x80");

   for(j=0; j<25; j++) {                           // clear screen, sort of
      outportb(term[term_no].io_base+DATA, '\n');
      for(i=0; i<LOOP/30; i++)asm("inb $0x80");
      outportb(term[term_no].io_base+DATA, '\r');
      for(i=0; i<LOOP/30; i++)asm("inb $0x80");
   }
/* // uncomment this part for VM (Procomm logo needs a key pressed, here reads it off)
   inportb(term_term_no].io_base); // clear key cleared PROCOMM screen
   for(i=0; i<LOOP/2; i++)asm("inb $0x80");
*/
}

void InitProc(void) {
   int i;
   vid_mux=MuxCreateCall(1);

   InitTerm(0);  // <------------------------------ NEW!!!
   InitTerm(1);  // <------------------------------ NEW!!!

   while(1) {
      ShowCharCall(0, 0, '.');
      for(i=0; i<LOOP/2; i++) asm("inb $0x80");   // this is also a kernel service

      ShowCharCall(0, 0, ' ');
      for(i=0; i<LOOP/2; i++) asm("inb $0x80");   // this is also a kernel service
   }
}

void Aout(int device) {
   int pid,i;
   int sleep;
   char str[] = "xx ( ) Hello, World!\n\r";
   pid=GetPidCall();

   str[0]='0'+ pid/10;
   str[1]='0'+ pid%10;
   str[4]='A' + pid - 1;
   WriteCall(device, str);
   PauseCall();
   
   for(i=0;i<=69;i++) {	
	sleep=RandCall() % 20 + 5;
	ShowCharCall(pid, i, str[4]);
	SleepCall(sleep);
	ShowCharCall(pid, i, ' ');
   }
   ExitCall(pid * 100);

   /*in above str, replace xx with my PID, and a alphabet inside
   the bracket (alphabet B if my PID is 2, C if 3, D if 4, etc.)

   prompt to terminal the str    // use same device as parent

   slide my alphabet across the Target PC display:
   cycle thru columns 0 to 69 {
      use ShowCharCall( ...
      use SleepCall(10);
      use ShowCharCall( ...
   }

   call ExitCall with an exit code that is my_pid * 100*/
}

void Ouch(int device) {
   WriteCall(device, "Can't touch that!\n\r");
}

void Wrapper(int handler, int arg) {   // args implanted in stack
   func_p_t2 func = (func_p_t2)handler;

   asm("pushal");                       // save regs
   func(arg);                           // call signal handler with arg
   asm("popal");                        // restore regs
   asm("movl %%ebp, %%esp; popl %%ebp; ret $8"::); // skip handler & arg
}

void UserProc(void) {
   int device;
   int my_pid = GetPidCall();  // get my PID
   int forkresp;
   char str1[STR_SIZE] = "PID    > ";         // <-------------------- new
   char str2[STR_SIZE];                       // <-------------------- new
   char forkstr[STR_SIZE];
   char exitcstr[STR_SIZE];
   char childalphabet[] = " X arrives!\n\r";
   int exitc;
   int i,j;
   str1[4]='0'+my_pid/10;
   str1[5]='0'+my_pid%10;
   device = my_pid % 2 == 1? TERM0_INTR : TERM1_INTR; // <---- new
   SignalCall(SIGINT,(int)Ouch);
   while(1) {
      WriteCall(device, str1);  // prompt for terminal input     <-------------- new
      ReadCall(device, str2);   // read terminal input           <-------------- new
      if(StrCmp(str2,"race")==FALSE) {
         continue;
      }
      for(i=0; i<5; i++) {
	forkresp=ForkCall();
      	if(forkresp==NONE) {
         	WriteCall(device, "Couldn't fork!\n\r");
	 	continue;
      	}
      	if(forkresp==0) {
	 	ExecCall((int)Aout, device);
      	} 
     	else {
		Itoa(forkstr,forkresp);
		WriteCall(device, "Child PID: ");
 		WriteCall(device,forkstr);
		WriteCall(device,"\n\r");
      	}
      }
      SleepCall(300);
      KillCall(0, SIGGO);
      SleepCall(1500);
      for(j=0; j<5; j++) {
        //cons_printf("wait %i\n",j);
	exitc=WaitCall();
	Itoa(exitcstr,exitc);
	childalphabet[1]='A' + exitc/100 - 1;
	WriteCall(device, "Child exit code: ");
	WriteCall(device, exitcstr);
	WriteCall(device, childalphabet);
      }
      /*prompt to terminal: the child PID
      Itoa(*pidstr, childpid);
      prompt to terminal "\n\r" can be needed;

      WaitCall();

      prompt to terminal: the child exit code (see demo)
      use Itoa() to convert # to a str and prompt it
      prompt to terminal "\n\r" can be needed*/
   }
}


