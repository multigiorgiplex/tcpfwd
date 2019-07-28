/*Manages polling of events*/

#include "pollerManager.h"
#include <sys/select.h>
#include <stdlib.h>


struct _fd {	//internal use only
	int fd;
	struct _fd * next;
};


fd_set PM_watchlist;
fd_set _PM_watchlist_compiled;
int PM_watchlist_max_fd;
struct timeval PM_timeout;
struct _fd * fds;


static void _PM_compile_watchlist (void)	//internal
{
	struct _fd * list = fds;
	FD_ZERO (&_PM_watchlist_compiled);
	PM_watchlist_max_fd = 0;

	while (list)
	{
		FD_SET (list->fd, &_PM_watchlist_compiled);
		if (list->fd > PM_watchlist_max_fd) PM_watchlist_max_fd = list->fd;

		list = list->next;
	}
}

void PM_init (unsigned watch_timeout)
{
	PM_timeout.tv_sec	= watch_timeout / 1000;
	PM_timeout.tv_usec	= (watch_timeout % 1000) * 1000;
	fds = 0;
}

void PM_watchlist_add (int fd)
{
	struct _fd * curr;
	struct _fd * prev;

	if (fds == 0)
	{
		fds = malloc (sizeof (struct _fd));
		if (!fds) return;

		fds->fd		= fd;
		fds->next	= 0;

		_PM_compile_watchlist ();
		return;
	}

	prev = fds;
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

	_PM_compile_watchlist ();
}

void PM_watchlist_remove (int fd)
{
	struct _fd * curr;
	struct _fd * prev = 0;

	if (!fds)
		return;

	if (fds->fd == fd)
	{
		curr = fds;	//temp
		fds = fds->next;
		free (curr);
		return;
	}

	prev = fds;
	curr = fds->next;
	
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

	_PM_compile_watchlist ();
}

int PM_watchlist_run ()	//TODO: manage writefs and exceptfs
{
	struct timeval timeout = PM_timeout;

	//reset
	PM_watchlist = _PM_watchlist_compiled;
	
	return select (PM_watchlist_max_fd+1, &PM_watchlist, 0, 0, &timeout);
}

void PM_watchlist_clear (int fd)
{
	FD_CLR (fd, &PM_watchlist);
}

char PM_watchlist_check (int fd)
{
	return (FD_ISSET (fd, &PM_watchlist) == 1);
}
