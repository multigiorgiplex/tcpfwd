/*Manages polling of events*/

#include "pollerManager.h"
#include <sys/select.h>
#include <stdlib.h>
#include "signalHandler.h"

#ifdef _DEBUG
#include <stdio.h>
#endif

struct _fd {	//internal use only
	int fd;
	struct _fd * next;
};


fd_set PM_watchlist[3];
fd_set _PM_watchlist_compiled[3];
int PM_watchlist_max_fd[3];
struct timespec PM_timeout;
struct _fd * fds[3];
SH_signalMask PM_signalMask;		//Signals to avoid interrupting to when calling PM_watchlist_run()


static void _PM_compile_watchlist (char list_type)	//internal
{
	struct _fd * list = fds[list_type];

	FD_ZERO (&_PM_watchlist_compiled[list_type]);
	PM_watchlist_max_fd[list_type] = 0;

#ifdef _DEBUG
	printf ("\t%s: ", (list_type == PM_READ) ? "PM_READ" : "PM_WRITE");
#endif
	while (list)
	{
		FD_SET (list->fd, &_PM_watchlist_compiled[list_type]);
		if (list->fd > PM_watchlist_max_fd[list_type]) PM_watchlist_max_fd[list_type] = list->fd;
#ifdef _DEBUG
		printf ("%d,", list->fd);
#endif
		list = list->next;
	}
#ifdef _DEBUG
	printf ("\b\n");
#endif
}


void PM_init (unsigned watch_timeout)
{
	PM_timeout.tv_sec	= watch_timeout / 1000;
	PM_timeout.tv_nsec	= (watch_timeout % 1000) * 1000000;
	fds[PM_READ] = 0;
	fds[PM_WRITE] = 0;
	fds[PM_EXCEPT] = 0;
	PM_watchlist_max_fd[PM_READ] = 0;
	PM_watchlist_max_fd[PM_WRITE] = 0;
	PM_watchlist_max_fd[PM_EXCEPT] = 0;
	SH_clearSignalMask (&PM_signalMask);
}


void PM_watchlist_add (int fd, char list_type)
{
	struct _fd * curr;
	struct _fd * prev;

	if (fds[list_type] == 0)
	{
		fds[list_type] = malloc (sizeof (struct _fd));
		if (!fds[list_type]) return;

		fds[list_type]->fd		= fd;
		fds[list_type]->next	= 0;

		PM_watchlist_max_fd[list_type] = 0;
		_PM_compile_watchlist (list_type);
		return;
	}

	prev = fds[list_type];
	curr = prev->next;	

	while (curr != 0)
	{
		prev = curr;
		curr = curr->next;
	}
	
	curr = malloc (sizeof (struct _fd));
	if (!curr)	return;

	prev->next	= curr;
	curr->fd	= fd;
	curr->next	= 0;

	PM_watchlist_max_fd[list_type] = 0;
	_PM_compile_watchlist (list_type);
}


void PM_watchlist_remove (int fd, char list_type)
{
	struct _fd * curr;
	struct _fd * prev = 0;

	if (!fds[list_type])
		return;

	if (fds[list_type]->fd == fd)
	{
		curr = fds[list_type];	//temp
		fds[list_type] = fds[list_type]->next;
		free (curr);
	}
	else
	{
		prev = fds[list_type];
		curr = fds[list_type]->next;

		while (curr && curr->fd != fd)
		{
			prev = curr;
			curr = curr->next;
		}

		if (curr)
		{
			prev->next = curr->next;
			free (curr);
		}
	}
	
	PM_watchlist_max_fd[list_type] = 0;
	_PM_compile_watchlist (list_type);
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
