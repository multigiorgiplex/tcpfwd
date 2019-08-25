#include <stdio.h>
//~ #include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "common.h"
#include "tcpHandler.h"
#include "cliTasks.h"
#include "pollerManager.h"
#include "endpointLinker.h"
#include "signalHandler.h"

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

#define POLLING_TIMEOUT		200		/*.2 second*/
#define MAX_CONNECTION		20

struct _ELlink {		//main.c use only
	ELlink *			link;
	struct _ELlink *	next;
};

struct callback_vector cbv;	//use for everything
struct _ELlink	links;
unsigned int 	links_num;
tcpConnection * server;		//main() server


void _ELlink_add (ELlink * l)
{
	struct _ELlink * p;
	
	if (links.link == 0)	//first
	{
		links.link = l;
		links_num = 1;
		return;
	}

	p = &links; //not = links.next; to avoid segmentation faults
	while (p->next)
		p = p->next;

	p->next = malloc (sizeof (struct _ELlink));
	p = p->next;

	p->link = l;
	p->next = 0;
	links_num++;
}

/*Returns removed element "->next" pointer*/
struct _ELlink * _ELlink_remove (ELlink * l)
{
	struct _ELlink * p;
	struct _ELlink * p_old;
	
	if (links.link == l)	//first
	{
		p = links.next;
		if (p)			//there is another element in the list, setting that element as the list new head
		{
			links.link = p->link;
			links.next = p->next;
			links_num--;
			free (p);
			return &links;
		}
		else 			//only this element in the list, clear list
		{
			links.link = 0;
			if (links_num != 1) return 0; //die()	TODO assert
			links_num = 0;
			return 0;
		}
	}

	p = &links;	//not = links.next; to avoid segmentation faults
	while (p->link != l)
	{
		if (p->next)
		{
			p_old = p;
			p = p->next;
		}
		else
			return 0;	//not found
	}
	
	if (p->next)
		p_old->next = p->next;
	else 	//last one
		p_old->next = 0;
		
	free (p);
	links_num--;

	return p_old->next;
}

int _ELlink_join (tcpConnection ** in, tcpConnection ** out)
{
	tcpConnection * inbound_connection = *in;
	tcpConnection * outbound_connection = *out;
	ELlink * temp_link = 0;

	PM_watchlist_clear (outbound_connection->fd, PM_WRITE);

	//It is now possible to connect
	PM_watchlist_add (inbound_connection->fd, PM_READ);
	PM_watchlist_add (outbound_connection->fd, PM_READ);

	//link together endpoints
	temp_link = EL_link_open (inbound_connection, outbound_connection, cbv);
	if (!temp_link)
		die_return ("EL_link_open", 0, errno, 1);

	printf ("Link ID %u: created between IN (fd: %d) %s:%u <==> %s:%u (fd: %d) OUT\n", temp_link->id,
			inbound_connection->fd, inbound_connection->address, inbound_connection->port,
			outbound_connection->address, outbound_connection->port, outbound_connection->fd);

	_ELlink_add (temp_link);

	//clear pointers for next round
	*in = 0;
	*out = 0;

	return 0;
}

void Signal (int s)	//TODO accessed when PM timeout is expired!
{
	struct _ELlink * n = &links;
	
	printf ("Received signal: %d, closing...\n", s);
	
	while (n)	//ELlink list destroy
	{
		EL_link_destroy (n->link);		
		n = _ELlink_remove (n->link);
	}
	TCP_connection_close (server);
	exit (EXIT_SUCCESS);
}

int main(int argc, char **argv)
{
	CLI_Arguments arguments;	
	tcpConnection * inbound_connection;
	tcpConnection * outbound_connection;
	struct _ELlink * link_list;	
	int watchlistCounter;
	PM_watchlist_checked_t watchlistCheck;
	SH_signalMask signal_mask;


	//modules initialization
	CLI_init ();
	PM_init (POLLING_TIMEOUT);
	EL_init ();
	SH_init ();

	//variables initialization
	inbound_connection = 0;
	outbound_connection = 0;
	links.link = 0;
	links.next = 0;

	//populate endpoint linker callback vector
	cbv.add		= &PM_watchlist_add;
	cbv.check	= &PM_watchlist_check;
	cbv.clear	= &PM_watchlist_clear;
	cbv.remove	= &PM_watchlist_remove;
	cbv.close	= &TCP_connection_close;
	cbv.recv	= &TCP_connection_receive;
	cbv.send	= &TCP_connection_send;
	cbv.destroy	= &TCP_connection_destroy;

	//setting up signals
	if (SH_clearSignalMask (&signal_mask)) return 1;					//clear signal mask
	if (SH_addSignalHandler (&Signal, SIGINT)) return 1;				//add a signal function handler to SIGINT (keyboard interrupt)
	if (SH_addSignalToMask (&signal_mask, SIGINT)) return 1;			//populate signal mask with SIGINT
	if (SH_addSignalHandler (&Signal, SIGTERM)) return 1;				//add a signal function handler to SIGTERM
	if (SH_addSignalToMask (&signal_mask, SIGTERM)) return 1;			//populate signal mask with SIGTERM
	PM_setSignalMask (signal_mask);										//apply signal mask to pollerManager (so SIGINT is ignored)

	
	
	CLI_welcomeMessage ();
	arguments = CLI_validateArguments (argc, argv);
	if (!arguments.valid)
		return 1;

	server = TCP_connection_init();
	if (TCP_connection_parse_input(server, arguments.localAddress, arguments.localPort) == 10)
	{
		printf ("Invalid local listening IP address (-la) %s.\n", arguments.localAddress);
		die ("TCP_connection_parse_input", callReturn, 0);
	}
	

	callReturn = TCP_connection_listen (server);
	switch (callReturn)
	{
		case 10:
		case 11:
		case 12:
		default:
			die ("TCP_connection_listen", callReturn, errno);

		case 0:
			printf ("Listening on %s:%u ...\n", server->address, server->port);
			break;
	}

	PM_watchlist_add (server->fd, PM_READ);	//PM_READ needed only to accept connections

	while (1)
	{
		watchlistCounter = PM_watchlist_run (0);
		if (watchlistCounter == -1)
			return 1;
		
		while (watchlistCounter--)
		{
			if (server)
			{
				watchlistCheck = PM_watchlist_check(server->fd);
				if (watchlistCheck.read)							//Event from SERVER
				{
					PM_watchlist_clear (server->fd, PM_READ);

					if (inbound_connection)	//We have another connection pending...
					{
						continue;	//Next round
					}

					//new client is connecting
					callReturn = TCP_connection_accept (server, &inbound_connection);
					if (callReturn)	die ("TCP_connection_accept", callReturn, errno);

					//check if we're busy
					if (links_num == MAX_CONNECTION)				//TODO add queue
					{
						printf ("Declining connection, we're busy now...\n");
						TCP_connection_close (inbound_connection);
						inbound_connection = 0;
						continue;
					}

					//attempt connection to the other side
					outbound_connection = TCP_connection_init ();	//data parsing
					if (TCP_connection_parse_input(outbound_connection, arguments.remoteAddress, arguments.remotePort) == 10)
					{
						printf ("Invalid remote connection IP address (-ra) %s.\n", arguments.remoteAddress);
						die ("TCP_connection_parse_input", callReturn, 0);
					}
//					printf ("Client %s:%u connected.\nConnecting to %s:%u ...\n", inbound_connection->address, inbound_connection->port, outbound_connection->address, outbound_connection->port);


					callReturn = TCP_connection_connect (outbound_connection);
					if (callReturn == 11 && errno == ECONNREFUSED)	/* Connection refused */
					{
						// No answer, quit current connection
						TCP_connection_destroy (outbound_connection);
						callReturn = TCP_connection_close (inbound_connection);
						if (callReturn) die ("TCP_connection_close", callReturn, errno);

						//clear pointers for next round
						inbound_connection = 0;
						outbound_connection = 0;
					}
					else if (callReturn == 11 && errno == EINPROGRESS)	/* Operation now in progress */
					{
						//"Queue" operation by putting under select
						PM_watchlist_add (outbound_connection->fd, PM_WRITE);
					}
					else if (callReturn)
						die_return ("TCP_connection_connect", callReturn, errno, 1)
					else
					{
						if (_ELlink_join (&inbound_connection, &outbound_connection))
							return 1;
					}
				}
			}	//if (server)


			if (outbound_connection)
			{
				watchlistCheck = PM_watchlist_check(outbound_connection->fd);
				if (watchlistCheck.write)
				{
					PM_watchlist_remove (outbound_connection->fd, PM_WRITE);
					if (_ELlink_join (&inbound_connection, &outbound_connection))
						return 1;
				}
			}	//if (outbound_connection)


			link_list = &links;
			while (link_list && link_list->link)
			{
				if (EL_link_check (link_list->link))
				{
					callReturn = EL_link_manage (link_list->link);
					if (callReturn == 10 || callReturn == 11)	//link destroyed
					{
						printf ("Link ID %u: destroyed because endpoint %s has disconnected.\n",
								link_list->link->id, (callReturn-10 == EL_ENDPOINT_IN) ? "IN" : "OUT");

						callReturn = EL_link_destroy (link_list->link);
						if (callReturn)
							return 1;
						link_list = _ELlink_remove (link_list->link);
						continue;
					}
					else if (callReturn)	//some errors
						die ("EL_link_manage", callReturn, errno);
				}

				link_list = link_list->next;

			}	//while (link_list && link_list->link)
		}	//while (watchlistCounter--)
	}
	
	return 0;
}

