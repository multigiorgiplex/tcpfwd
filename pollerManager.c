/*Manages polling of events*/

#include "pollerManager.h"
#include <sys/select.h>
#include <stdlib.h>
#include "signalHandler.h"

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
	/*	fd_set is just a struct containing an array of (__FD_SETSIZE / __NFDBITS) elements of __fd_mask type
	 * 	__fd_mask is a integer type of length __NFDBITS bits, therefore the maximum fd allowed is __FD_SETSIZE.
	 */
	int ret = __FD_SETSIZE -1;
	__fd_mask mask;
	signed char i;
	unsigned short j;

	/* for every element of fd_set.__fd_mask */
	for (i = (__FD_SETSIZE / __NFDBITS) -1; i >= 0; i--)
	{
		/* generate a bit mask starting a the leftmost bit */
		mask = 1UL << (__NFDBITS -1);

		/* for every bit in __fd_mask */
		for (j = 0; j < __NFDBITS; j++)
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


void PM_watchlist_add (int fd, char list_type)
{
	FD_SET (fd, &_PM_watchlist_compiled[list_type]);
	if (fd > PM_watchlist_max_fd[list_type]) PM_watchlist_max_fd[list_type] = fd;
}


void PM_watchlist_remove (int fd, char list_type)
{
	FD_CLR (fd, &_PM_watchlist_compiled[list_type]);
	PM_watchlist_max_fd[list_type] = _PM_get_max_fd (&_PM_watchlist_compiled[list_type]);
}


int PM_watchlist_run ()
{
	struct timespec timeout = PM_timeout;
	int max_fd;

	//reset
	PM_watchlist[PM_READ]		= _PM_watchlist_compiled[PM_READ];
	PM_watchlist[PM_WRITE]		= _PM_watchlist_compiled[PM_WRITE];
	PM_watchlist[PM_EXCEPT]		= _PM_watchlist_compiled[PM_EXCEPT];
	
													max_fd = PM_watchlist_max_fd[PM_READ];
	if (PM_watchlist_max_fd[PM_WRITE] > max_fd)		max_fd = PM_watchlist_max_fd[PM_WRITE];
	if (PM_watchlist_max_fd[PM_EXCEPT] > max_fd)	max_fd = PM_watchlist_max_fd[PM_EXCEPT];

	return pselect (max_fd+1, &PM_watchlist[PM_READ], &PM_watchlist[PM_WRITE], &PM_watchlist[PM_EXCEPT], &timeout, &(PM_signalMask.signals));
}


void PM_watchlist_clear (int fd, char list_type)
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
