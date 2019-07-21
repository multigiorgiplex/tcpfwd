#ifndef _POLLERMANAGER_H
#define _POLLERMANAGER_H

void		PM_init				(unsigned);
void		PM_watchlist_reset	(void);
void		PM_watchlist_add	(int);
int			PM_watchlist_run	(void);
void		PM_watchlist_clear	(int);
char		PM_watchlist_check	(int);

#endif
