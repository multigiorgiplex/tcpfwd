/*Handles connection endpoints*/

#include "endpointLinker.h"
#include <stdlib.h>
#include "tcpHandler.h"
#include "cliTasks.h"

	#include <stdio.h>	//TODO: remove, use ioModule.c

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
	{
		if (source_ep == -1)
			source_ep = 1;
		else
			source_ep = (l->endpoint[0]->buffer_len >= l->endpoint[0]->buffer_len) ? 0 : 1;		//if either endpoints are sending data, priority is given to the endpoint with bigger payload
			//return 10;	//can't have multiple endpoints sending data, only one allowed
	}
	
	//watchlist clear
	l->cbv[source_ep].clear (l->endpoint[source_ep]->fd);
	
	// receiving from endpoint
	callReturn = l->cbv[source_ep].recv (l->endpoint[source_ep]);
	if (callReturn > 0)
	{
		die_soft("struct callback_vector.recv", callReturn, errno);
		return 0;
	}
	if (callReturn == -10)
	{
		printf ("%s:%u disconnected. Closing other endpoint. Destroy link.\n", l->endpoint[source_ep]->address, l->endpoint[source_ep]->port);

		if (EL_link_destroy (l)) return 21;

		return 20;
	}

	//data arrangment
	l->endpoint[!source_ep]->buffer		= l->endpoint[source_ep]->buffer;
	l->endpoint[!source_ep]->buffer_len	= l->endpoint[source_ep]->buffer_len;
	
	//data send
	callReturn = l->cbv[!source_ep].send (l->endpoint[!source_ep]);
	if (callReturn) die ("struct callback_vector.send", callReturn, errno);
	
	return 0;
}

int EL_link_destroy (ELlink * l)
{
	if (!l)
		return 10;
		
	//watchlist remove
	l->cbv[EL_ENDPOINT_IN].remove (l->endpoint[EL_ENDPOINT_IN]->fd);
	l->cbv[EL_ENDPOINT_OUT].remove (l->endpoint[EL_ENDPOINT_OUT]->fd);

	//connection destroy
	callReturn = l->cbv[EL_ENDPOINT_IN].close (l->endpoint[EL_ENDPOINT_IN]);
	if (callReturn) die_return ("struct callback_vector.close", callReturn, errno, 1);
	callReturn = l->cbv[EL_ENDPOINT_OUT].close (l->endpoint[EL_ENDPOINT_OUT]);
	if (callReturn) die_return ("struct callback_vector.close", callReturn, errno, 2);
	
	//link destroy
	free (l);

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

