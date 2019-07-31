#ifndef _SIGNALHANDLER_H
#define _SIGNALHANDLER_H

#include <signal.h>


#define		SH_SIGNAL_START		1
#define		SH_SIGNAL_END		31


typedef struct {
	sigset_t signals;
} SH_signalMask;

void		SH_init						(void);
int			SH_fetchSignalMask			(SH_signalMask *);
void		SH_printSignalMask			(SH_signalMask);
int			SH_addSignalToMask			(SH_signalMask *, int);
int			SH_clearSignalMask			(SH_signalMask *);
int			SH_addSignalHandler			(void (*)(int), int);

#endif
