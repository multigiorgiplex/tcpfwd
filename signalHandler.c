/*Signals handler*/

#include "signalHandler.h"
#include <signal.h>
#include <stdio.h>
#include "cliTasks.h"


void SH_init ()
{
	;
}

int SH_fetchSignalMask (SH_signalMask * out)
{
	sigset_t temp;

	callReturn = sigprocmask (0, 0, &temp);
	if (callReturn) die_return ("sigprocmask", callReturn, errno, 1);	//return 1;

	if (out)
		out->signals = temp;

	return 0;
}

void SH_printSignalMask (SH_signalMask m)
{
	int i;

	for (i = SH_SIGNAL_START; i <= SH_SIGNAL_END; i++)
	{
		callReturn = sigismember (&(m.signals), i);
		if (callReturn == -1)	die_return ("sigismember", callReturn, errno, );	//return;
		if (callReturn)
			printf ("%d, ", i);
	}
	printf ("\n");
}

int SH_addSignalToMask (SH_signalMask * m, int sig)
{
	callReturn = sigaddset (&(m->signals), sig);
	if (callReturn == -1) die ("sigaddset", callReturn, errno);

	return 0;
}

int SH_clearSignalMask (SH_signalMask * m)
{
	callReturn = sigemptyset (&(m->signals));
	if (callReturn == -1) die ("sigemptyset", callReturn, errno);
	return 0;
}

int SH_addSignalHandler (void (*handler)(int), int sig)
{
	struct sigaction action;
	sigset_t mask_temp;

	//generate empty mask with _sig_ setted, to avoid catching _sig_ in _handler_
	callReturn = sigemptyset (&mask_temp);
	if (callReturn == -1) die ("sigemptyset", callReturn, errno);
	callReturn = sigaddset (&mask_temp, sig);
	if (callReturn == -1) die ("sigaddset", callReturn, errno);

	action.sa_handler = handler;
	//~ action.sa_sigaction = 0;	//unneeded
	action.sa_mask = mask_temp;		//apply mask
	action.sa_flags		= 0;		//no flags
	action.sa_restorer	= 0;		//do not use
	
	callReturn = sigaction (sig, &action, 0);
	if (callReturn) die ("sigaction", callReturn, errno);
	return 0;
}
