#ifndef _POLLERMANAGER_H
#define _POLLERMANAGER_H

#include "signalHandler.h"
#include <sys/select.h>		//fd_set
#include <stdint.h>

#define PM_READ		0
#define PM_WRITE	1
#define PM_EXCEPT	2

typedef struct {
	union {
		uint8_t value;
		struct {
			uint8_t read	:1;
			uint8_t write	:1;
			uint8_t except	:1;
			uint8_t		:5;
		};
	};
} PM_watchlist_checked_t;

void					PM_init				(unsigned);
void					PM_watchlist_add	(int, uint8_t);
void					PM_watchlist_remove	(int, uint8_t);
void					PM_watchlist_clear	(int, uint8_t);
PM_watchlist_checked_t	PM_watchlist_check	(int);
int						PM_watchlist_run	(unsigned int *);
void					PM_setSignalMask	(SH_signalMask);
SH_signalMask			PM_getSignalMask	(void);

#ifdef _DEBUG
void					PM_watchlist_print	(fd_set *);
#endif

#endif
