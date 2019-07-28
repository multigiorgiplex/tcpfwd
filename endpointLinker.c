/*Handles connection endpoints*/

#include "endpointLinker.h"
#include "tcpHandler.h"
#include <stdlib.h>



//TODO error handling module
#include <errno.h>
#include <stdio.h>
int callReturn;
#define callResult(functionName)		fprintf ((callReturn) ? stderr : stdout, functionName"() [%d]: %s\n", callReturn, strerror (callReturn ? errno : 0));


void EL_init ()
{
	;
}

ELlink * EL_link_open (tcpConnection * a, tcpConnection * b, struct callback_vector cbv)
{
	ELlink * p;
	
	p = malloc (sizeof (ELlink));
	if (p == 0)
		return 0;
	
	p->endpoint[0]	= a;
	p->endpoint[1]	= b;
	p->cbv[0]		= cbv;
	p->cbv[1]		= cbv;
	
	return p;
}

int EL_link_manage (ELlink * l)
{
	signed char source_ep = -1;

	//check where is data available
	if (l->cbv[0].check (l->endpoint[0]->fd))
		source_ep = 0;
	if (l->cbv[1].check (l->endpoint[1]->fd))
		if (source_ep == -1)
			source_ep = 1;
		else
			return 10;	//can't have multiple endpoints sending data, only one allowed

	// receiving from endpoint
	callReturn = l->cbv[source_ep].recv (l->endpoint[source_ep]);
	if (callReturn > 0)
		return 1;
	if (callReturn == -10)
	{
		printf ("%s:%u disconnected. Closing other endpoint. Destroy link.\n", l->endpoint[source_ep]->address, l->endpoint[source_ep]->port);
		//watchlist remove
		l->cbv[source_ep].clear (l->endpoint[source_ep]->fd);
		l->cbv[source_ep].remove (l->endpoint[source_ep]->fd);
		l->cbv[!source_ep].remove (l->endpoint[!source_ep]->fd);

		//connection destroy
		l->cbv[source_ep].destroy (l->endpoint[source_ep]);
		l->cbv[!source_ep].close (l->endpoint[!source_ep]);

		//link destroy
		free (l);

		return 20;
	}

	//watchlist clear
	l->cbv[source_ep].clear (l->endpoint[source_ep]->fd);

	//data arrangment
	l->endpoint[!source_ep]->buffer		= l->endpoint[source_ep]->buffer;
	l->endpoint[!source_ep]->buffer_len	= l->endpoint[source_ep]->buffer_len;
	
	//data send
	if (l->cbv[!source_ep].send (l->endpoint[!source_ep]))
	{
		perror ("EL_link_manage.send");
		return 30;
	}
	
	return 0;
}

char EL_link_check (ELlink * l)
{
	if (l == 0)
		return 0;
		
	return (
		l->cbv[0].check (l->endpoint[0]->fd)	||
		l->cbv[1].check (l->endpoint[1]->fd)
	);
}

