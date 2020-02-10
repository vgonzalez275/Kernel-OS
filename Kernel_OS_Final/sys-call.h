#ifndef __SYS_CALL__
#define __SYS_CALL__

int GetPidCall(void);
void ShowCharCall(int row, int col, char ch);
void SleepCall(int centi_sec);
int MuxCreateCall(int flag);
void MuxOpCall(int mux_id, int opcode);
void WriteCall(int device, char *str);
void ReadCall(int device, char *str);
int ForkCall(void);
int WaitCall(void);
void ExitCall(int exit_code);
void ExecCall(int code, int arg);
void SignalCall(int sig_num, int handler);
void PauseCall(void);
void KillCall(int pid, int sig_num);
unsigned RandCall(void);

#endif 
