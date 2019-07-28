#ifndef _POLLERMANAGER_H
#define _POLLERMANAGER_H

void		PM_init				(unsigned);
void		PM_watchlist_add	(int);
void		PM_watchlist_remove	(int);
void		PM_watchlist_clear	(int);
char		PM_watchlist_check	(int);
int			PM_watchlist_run	(void);

#endif
