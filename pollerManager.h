#ifndef _POLLERMANAGER_H
#define _POLLERMANAGER_H

#include "signalHandler.h"
#include <sys/select.h>		//fd_set

#define PM_READ		0
#define PM_WRITE	1
#define PM_EXCEPT	2

typedef struct {
	union {
		char value;
		struct {
			char read	:1;
			char write	:1;
			char except	:1;
			char		:5;
		};
	};
} PM_watchlist_checked_t;

void					PM_init				(unsigned);
void					PM_watchlist_add	(int, char);
void					PM_watchlist_remove	(int, char);
void					PM_watchlist_clear	(int, char);
PM_watchlist_checked_t	PM_watchlist_check	(int);
int						PM_watchlist_run	(unsigned int *);
void					PM_watchlist_print	(fd_set *);
void					PM_setSignalMask	(SH_signalMask);
SH_signalMask			PM_getSignalMask	(void);

#endif
