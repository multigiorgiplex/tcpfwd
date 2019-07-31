#ifndef _ENDPOINTLINKER_H
#define _ENDPOINTLINKER_H

#include "tcpHandler.h"

#define ENDPOINT_IN		0
#define ENDPOINT_OUT	1

struct callback_vector {
	//pollerManager.c
	void			(*remove)(int);
	void			(*clear)(int);
	char			(*check)(int);
	//tcpHandler.c	
	int				(*send)(tcpConnection *);
	int				(*recv)(tcpConnection *);
	int				(*close)(tcpConnection *);
	void			(*destroy)(tcpConnection *);
};

struct data_exchange {
	unsigned long	upload;
	unsigned long	download;
};


typedef struct {
	unsigned int			id;
	tcpConnection *			endpoint[2];
	struct callback_vector	cbv[2];
	struct data_exchange	transferred[2];
} ELlink;


void		EL_init				(void);
ELlink *	EL_link_open		(tcpConnection *, tcpConnection *, struct callback_vector);
int			EL_link_manage		(ELlink *);
char		EL_link_check		(ELlink *);

#endif
