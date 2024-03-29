#ifndef _ENDPOINTLINKER_H
#define _ENDPOINTLINKER_H

#include "tcpHandler.h"
#include "pollerManager.h"
#include <stdint.h>

#define EL_ENDPOINT_IN		0
#define EL_ENDPOINT_OUT		1

struct callback_vector
{
	//pollerManager.c
	void					(*add)(int, uint8_t);
	void					(*remove)(int, uint8_t);
	void					(*clear)(int, uint8_t);
	PM_watchlist_checked_t	(*check)(int);
	//tcpHandler.c	
	TCP_CONN_SEND_t			(*send)(tcpConnection *);
	TCP_CONN_RECEIVE_t		(*recv)(tcpConnection *);
	TCP_CONN_CLOSE_t		(*close)(tcpConnection *);
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
