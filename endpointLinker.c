/*Handles connection endpoints*/

#include "endpointLinker.h"
#include <stdlib.h>
#include "tcpHandler.h"
#include "cliTasks.h"
#include "pollerManager.h"

#ifdef _DEBUG
#include <stdio.h>
#endif

void EL_init ()
{
	;
}

/* Opens a link assigning given tcpConnectiona as endpoints and the same callback_vector to each of them*/
ELlink * EL_link_open (tcpConnection * ep_in, tcpConnection * ep_out, struct callback_vector cbv)
{
	static unsigned int link_id = 0;
	ELlink * p;
	
	p = malloc (sizeof (ELlink));
	if (p == 0)
		return 0;

	p->id								= ++link_id;
	p->endpoint[EL_ENDPOINT_IN]			= ep_in;
	p->endpoint[EL_ENDPOINT_OUT]		= ep_out;
	p->cbv[EL_ENDPOINT_IN]				= cbv;
	p->cbv[EL_ENDPOINT_OUT]				= cbv;
	p->flag_sending[EL_ENDPOINT_IN]		= 0;
	p->flag_sending[EL_ENDPOINT_OUT]	= 0;
	
	return p;
}


int EL_link_manage (ELlink * l)
{
	signed char source_ep = -1;
	PM_watchlist_checked_t dummy_checked;

	//Find where is data
	dummy_checked = l->cbv[EL_ENDPOINT_IN].check (l->endpoint[EL_ENDPOINT_IN]->fd);
	if (dummy_checked.value)
		source_ep = EL_ENDPOINT_IN;
	else
	{
		dummy_checked = l->cbv[EL_ENDPOINT_OUT].check (l->endpoint[EL_ENDPOINT_OUT]->fd);
		if (dummy_checked.value)
			source_ep = EL_ENDPOINT_OUT;
		else
			return 0;	//nothing
	}

	if (dummy_checked.read)	//source_ep needs to be read
	{
//#ifdef _DEBUG
//		printf ("Link ID %u: endpoint %u needs to be read.\n", l->id, source_ep);
//#endif

		//Check if other endpoint is clear to send
		if (l->flag_sending[!source_ep])
			return 0;	//Try next time	TODO add retry counter


		//Receiving data from endpoint
		callReturn = l->cbv[source_ep].recv (l->endpoint[source_ep]);
		if (callReturn == TCP_CONN_RECEIVE_ERECV)
		{
			die_soft("struct callback_vector.recv", callReturn, errno);
			return 0;
		}
		if (callReturn == TCP_CONN_RECEIVE_DISCONNECTED)
		{
			return 10 + source_ep;
		}
		if (callReturn == TCP_CONN_RECEIVE_BLOCK)
			return 0;	//The operation would block - try next time


		//Data received

		//Clear watchlist
		l->cbv[source_ep].clear (l->endpoint[source_ep]->fd, PM_READ);

		//Arrange data
		l->endpoint[!source_ep]->buffer		= l->endpoint[source_ep]->buffer;
		l->endpoint[!source_ep]->buffer_len	= l->endpoint[source_ep]->buffer_len;

		//Send data to the other endpoint

		callReturn = l->cbv[!source_ep].send (l->endpoint[!source_ep]);
		if (callReturn == TCP_CONN_SEND_BLOCK)	//The operation would block
		{
#ifdef _DEBUG
			callReturn = TCP_connection_get_socket_error (l->endpoint[!source_ep]);
			die_soft ("asd", callReturn, 0);
#endif
			//Block other endpoint to writing
			l->flag_sending[!source_ep] = 1;

			//Add other endpoint to write watchlist
			l->cbv[!source_ep].add (l->endpoint[!source_ep]->fd, PM_WRITE);

			//Remove this endpoint from read watchlist as it will be indefinitely triggered until the other endpoint will be freed
			l->cbv[source_ep].remove (l->endpoint[source_ep]->fd, PM_READ);

#ifdef _DEBUG
			printf ("Link ID: %u, endpoint %u disabled read() because endpoint %u has blocking write().\n", l->id, source_ep, !source_ep);
#endif
		}
		else if (callReturn == TCP_CONN_SEND_ESEND)
			die ("struct callback_vector.send", callReturn, errno);


		//Check if other endopoint has data left to send
		if (l->endpoint[!source_ep]->buffer_len)
		{
			//Block other endpoint to writing
			l->flag_sending[!source_ep] = 1;

			//Add other endpoint to write watchlist
			l->cbv[!source_ep].add (l->endpoint[!source_ep]->fd, PM_WRITE);

			//Remove this endpoint from read watchlist as it will be indefinitely triggered until the other endpoint will be freed
			l->cbv[source_ep].remove (l->endpoint[source_ep]->fd, PM_READ);

#ifdef _DEBUG
			printf ("Link ID: %u, endpoint %u has %u bytes left to send\n", l->id, !source_ep, l->endpoint[!source_ep]->buffer_len);
#endif
		}


		return 0;
	}
	else if (dummy_checked.write)	//source_ep needs to be wrote
	{
#ifdef _DEBUG
		printf ("Link ID %u: endpoint %u needs to be wrote.\n", l->id, source_ep);
#endif
		//Check if there is data to be sent
		if (l->flag_sending[source_ep])
		{
			//Retry sending data
			callReturn = l->cbv[source_ep].send (l->endpoint[source_ep]);
			if (callReturn == TCP_CONN_SEND_BLOCK)	//Connection would block
			{
				//TODO: Add retry
			}
			else if (callReturn == TCP_CONN_SEND_ESEND)
			{
				die ("struct callback_vector.send", callReturn, errno);
			}

			//Check if this endpoint has still data left to send
			if (l->endpoint[source_ep]->buffer_len)
			{
#ifdef _DEBUG
				printf ("Link ID: %u, endpoint %u has %u bytes left to send\n", l->id, source_ep, l->endpoint[source_ep]->buffer_len);
#endif
			}
			else
			{
				//Clear blocking flag
				l->flag_sending[source_ep] = 0;

				//Remove from write watchlist
				l->cbv[source_ep].remove (l->endpoint[source_ep]->fd, PM_WRITE);

				//Re-enable the other endpoint on reading, as the read operation would succeed because this endpoint is clear to be written
				l->cbv[!source_ep].add (l->endpoint[!source_ep]->fd, PM_READ);

#ifdef _DEBUG
				printf ("Link ID: %u, endpoint %u restored read() because endpoint %u has cleared write().\n", l->id, !source_ep, source_ep);
#endif
			}
		}
		else
		{
			//Remove from write watchlist
			l->cbv[source_ep].remove (l->endpoint[source_ep]->fd, PM_WRITE);

#ifdef _DEBUG
			printf ("Link ID %u: endpoint %u write cleared but no data available.\n", l->id, source_ep);
#endif
		}
	}
	else
		return 0;	//not managed

	return 0;
}


int EL_link_destroy (ELlink * l)
{
	if (!l)
		return 10;
		
	//watchlist remove
	l->cbv[EL_ENDPOINT_IN].remove (l->endpoint[EL_ENDPOINT_IN]->fd, PM_READ);
	l->cbv[EL_ENDPOINT_IN].remove (l->endpoint[EL_ENDPOINT_IN]->fd, PM_WRITE);
	l->cbv[EL_ENDPOINT_OUT].remove (l->endpoint[EL_ENDPOINT_OUT]->fd, PM_READ);
	l->cbv[EL_ENDPOINT_OUT].remove (l->endpoint[EL_ENDPOINT_OUT]->fd, PM_WRITE);

	//connection destroy
	callReturn = l->cbv[EL_ENDPOINT_IN].close (l->endpoint[EL_ENDPOINT_IN]);
	if (callReturn != TCP_CONN_CLOSE_OK) die_return ("struct callback_vector.close", callReturn, errno, 1);
	callReturn = l->cbv[EL_ENDPOINT_OUT].close (l->endpoint[EL_ENDPOINT_OUT]);
	if (callReturn != TCP_CONN_CLOSE_OK) die_return ("struct callback_vector.close", callReturn, errno, 2);

	//link destroy
	free (l);

	return 0;
}


/* Returns true if at least one of the two endpoints in link l is ready for IO operations. */
char EL_link_check (ELlink * l)
{
	PM_watchlist_checked_t dummy;

	if (l == 0)
		return 0;
		
	dummy = l->cbv[EL_ENDPOINT_IN].check (l->endpoint[EL_ENDPOINT_IN]->fd);
	if (dummy.value)
		return 1;
	dummy = l->cbv[EL_ENDPOINT_OUT].check (l->endpoint[EL_ENDPOINT_OUT]->fd);
	if (dummy.value)
		return 2;
	return 0;
}

