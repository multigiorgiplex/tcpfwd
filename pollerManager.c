/*Manages polling of events*/

#include "pollerManager.h"
#include <sys/select.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <stdint.h>
#include "signalHandler.h"
#include "cliTasks.h"

#ifdef _DEBUG
#include <stdio.h>
#endif


fd_set PM_watchlist[3];
fd_set _PM_watchlist_compiled[3];
int PM_watchlist_max_fd[3];
struct timespec PM_timeout;
SH_signalMask PM_signalMask;		//Signals to avoid interrupting to when calling PM_watchlist_run()


/* returns the max fd in a specified set */
static int _PM_get_max_fd (fd_set * set)
{
	/*	fd_set is just a struct containing an array of (FD_SETSIZE / NFDBITS) elements of fd_mask type
	 *  fd_mask is a integer type of length NFDBITS bits, therefore the maximum fd allowed is FD_SETSIZE.
	 */
	int ret = FD_SETSIZE -1;
	fd_mask mask;
	signed char i;
	unsigned short j;

	/* for every element of fd_set.fd_mask */
	for (i = (FD_SETSIZE / NFDBITS) -1; i >= 0; i--)
	{
		/* generate a bit mask starting a the leftmost bit */
		mask = 1UL << (NFDBITS -1);

		/* for every bit in __fd_mask */
		for (j = 0; j < NFDBITS; j++)
		{
			/* compare if it matches with the mask */
			if (set->__fds_bits[i] & mask)
				/* if so, return the resulting fd */
				return ret;
			else
			{
				/* if not, right-shift the test mask by one and decrement the resulting fd */
				mask >>= 1;
				ret--;
			}
		}
	}

	/* if no fd found, return minus one */
	return -1;
}


void PM_init (unsigned watch_timeout)
{
	PM_timeout.tv_sec	= watch_timeout / 1000;
	PM_timeout.tv_nsec	= (watch_timeout % 1000) * 1000000;
	PM_watchlist_max_fd[PM_READ] = -1;
	PM_watchlist_max_fd[PM_WRITE] = -1;
	PM_watchlist_max_fd[PM_EXCEPT] = -1;
	SH_clearSignalMask (&PM_signalMask);
}

#ifdef _DEBUG
void PM_watchlist_print (fd_set * set)
{
	fd_mask mask;
	signed char i;
	unsigned short j;
	int setted_fd, max_fd;

	max_fd = _PM_get_max_fd (set);
	setted_fd = 0;

	for (i=0; i<FD_SETSIZE / NFDBITS; i++)
	{
		mask = 1UL;
		for (j = 0; j < NFDBITS; j++)
		{
			if (set->__fds_bits[i] & mask)
				printf ("%u, ", setted_fd);

			mask <<= 1;
			setted_fd++;

			if (setted_fd > max_fd)
			{
				// Exit
				j = NFDBITS;
				i = FD_SETSIZE / NFDBITS;
			}
		}
	}
}
#endif

void PM_watchlist_add (int fd, uint8_t list_type)
{
	FD_SET (fd, &_PM_watchlist_compiled[list_type]);
	if (fd > PM_watchlist_max_fd[list_type]) PM_watchlist_max_fd[list_type] = fd;	// Faster than _PM_get_max_fd

#ifdef _DEBUG
	printf ("[" __FILE__ ":%u] @ PM_watchlist_add(%d, %u) - FDs: ", __LINE__, fd, list_type);
	PM_watchlist_print (&_PM_watchlist_compiled[list_type]);
	printf ("\n");
#endif
}


void PM_watchlist_remove (int fd, uint8_t list_type)
{
	FD_CLR (fd, &_PM_watchlist_compiled[list_type]);
	PM_watchlist_max_fd[list_type] = _PM_get_max_fd (&_PM_watchlist_compiled[list_type]);

#ifdef _DEBUG
	printf ("[" __FILE__ ":%u] @ PM_watchlist_remove(%d, %u) - FDs: ", __LINE__, fd, list_type);
	PM_watchlist_print (&_PM_watchlist_compiled[list_type]);
	printf ("\n");
#endif
}


int PM_watchlist_run (unsigned int * elapsed_time)
{
	int max_fd, ret;
	unsigned int wait_time;
	struct timespec t_start, t_stop;

	//reset
	PM_watchlist[PM_READ]		= _PM_watchlist_compiled[PM_READ];
	PM_watchlist[PM_WRITE]		= _PM_watchlist_compiled[PM_WRITE];
	PM_watchlist[PM_EXCEPT]		= _PM_watchlist_compiled[PM_EXCEPT];
													max_fd = PM_watchlist_max_fd[PM_READ];
	if (PM_watchlist_max_fd[PM_WRITE] > max_fd)		max_fd = PM_watchlist_max_fd[PM_WRITE];
	if (PM_watchlist_max_fd[PM_EXCEPT] > max_fd)	max_fd = PM_watchlist_max_fd[PM_EXCEPT];


	callReturn = clock_gettime (CLOCK_MONOTONIC_RAW, &t_start);		/* Monotonic system-wide clock, not adjusted for frequency scaling.  */
	if (callReturn) die ("clock_gettime", callReturn, errno);

	/* select() may update the timeout argument to indicate how much time was left.  pselect() does not change this argument. */
	ret = pselect (max_fd+1, &PM_watchlist[PM_READ], &PM_watchlist[PM_WRITE], &PM_watchlist[PM_EXCEPT], &PM_timeout, &(PM_signalMask.signals));
	if (ret == -1) die ("pselect", ret, errno);

	callReturn = clock_gettime (CLOCK_MONOTONIC_RAW, &t_stop);
	if (callReturn) die ("clock_gettime", callReturn, errno);

	wait_time = (t_stop.tv_sec - t_start.tv_sec) *1000;
	wait_time += (t_stop.tv_nsec - t_start.tv_nsec) /1000000;

#if defined _DEBUG && 0
	printf ("[" __FILE__ ":%u] @ PM_watchlist_run() wait_time: %u\n", __LINE__, wait_time);
#endif

	if (elapsed_time)
		*elapsed_time = wait_time;

	return ret;
}


void PM_watchlist_clear (int fd, uint8_t list_type)
{
	FD_CLR (fd, &PM_watchlist[list_type]);
}


PM_watchlist_checked_t PM_watchlist_check (int fd)
{
	PM_watchlist_checked_t ret;

	ret.value	= 0;
	ret.read	= (FD_ISSET (fd, &PM_watchlist[PM_READ]) == 1);
	ret.write	= (FD_ISSET (fd, &PM_watchlist[PM_WRITE]) == 1);
	ret.except	= (FD_ISSET (fd, &PM_watchlist[PM_EXCEPT]) == 1);

	return ret;
}


void PM_setSignalMask (SH_signalMask m)
{
	PM_signalMask = m;
}


SH_signalMask PM_getSignalMask ()
{
	return PM_signalMask;
}
