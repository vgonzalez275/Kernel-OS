// k-entry.h
// prototypes those in k-entry.S
// OS phase 9
//
// Team Name: BJOS (Members: Brian Huang, Jupp Valdez, Veronica Gonzalez)

#ifndef __K_ENTRY__
#define __K_ENTRY__

#ifndef ASSEMBLER  // skip below if ASSEMBLER defined (from an assembly code)
                   // since below is not in assembler syntax
__BEGIN_DECLS

#include "k-type.h"               // trapframe_t

void TimerEntry(void);            // coded in k-entry.S, assembler won't like this syntax
void Loader(trapframe_t *);       // coded in k-entry.S
void GetPidEntry(void);
void ShowCharEntry(void);
void SleepEntry(void);
void MuxCreateEntry(void);
void MuxOpEntry(void);
void Term0Entry(void);
void Term1Entry(void);
void ForkEntry(void);
void WaitEntry(void);
void ExitEntry(void);
void ExecEntry(void);
void SignalEntry(void);
void PauseEntry(void);
void KillEntry(void);
void RandEntry(void);


__END_DECLS

#endif // ifndef ASSEMBLER

#endif // ifndef __K_ENTRY__

