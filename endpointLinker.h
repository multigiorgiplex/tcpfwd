#ifndef _ENDPOINTLINKER_H
#define _ENDPOINTLINKER_H

#include "tcpHandler.h"
#include "pollerManager.h"

#define EL_ENDPOINT_IN		0
#define EL_ENDPOINT_OUT		1

struct callback_vector
{
	//pollerManager.c
	void					(*add)(int, char);
	void					(*remove)(int, char);
	void					(*clear)(int, char);
	PM_watchlist_checked_t	(*check)(int);
	//tcpHandler.c	
	int						(*send)(tcpConnection *);
	int						(*recv)(tcpConnection *);
	int						(*close)(tcpConnection *);
	void					(*destroy)(tcpConnection *);
};

typedef struct {
	unsigned int 			id;
	tcpConnection *			endpoint[2];
	struct callback_vector	cbv[2];
	char					flag_sending[2];
} ELlink;


void		EL_init				(void);
ELlink *	EL_link_open		(tcpConnection *, tcpConnection *, struct callback_vector);
int			EL_link_manage		(ELlink *);
int			EL_link_destroy		(ELlink *);
char		EL_link_check		(ELlink *);

#endif
