/*Manages polling of events*/

#include "pollerManager.h"
#include <sys/select.h>

fd_set PM_watchlist;
int PM_watchlist_max_fd;
struct timeval PM_timeout;

void PM_init (unsigned watch_timeout)
{
	PM_timeout.tv_sec	= watch_timeout / 1000;
	PM_timeout.tv_usec	= (watch_timeout % 1000) * 1000;
}

void PM_watchlist_reset ()
{
	FD_ZERO (&PM_watchlist);
	PM_watchlist_max_fd = 0;
}

void PM_watchlist_add (int fd)
{
	FD_SET (fd, &PM_watchlist);
	if (fd > PM_watchlist_max_fd) PM_watchlist_max_fd = fd;
}

int PM_watchlist_run ()	//TODO: manage writefs and exceptfs
{
	struct timeval timeout = PM_timeout;
	
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
