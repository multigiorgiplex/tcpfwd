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

#define POLLING_TIMEOUT		1000	/*1 second*/
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
	ELlink * temp_link;
	struct _ELlink * link_list;	
	int watchlistCounter;
	SH_signalMask signal_mask;

	//modules initialization
	CLI_init ();
	PM_init (POLLING_TIMEOUT);
	EL_init ();
	SH_init ();

	//variables initialization
	links.link = 0;
	links.next = 0;
	temp_link = 0;

	//populate endpoint linker callback vector
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
	PM_setSignalMask (signal_mask);										//apply signal mask to pollerManager (so SIGINT is ignored)

	
	
	CLI_welcomeMessage ();
	arguments = CLI_validateArguments (argc, argv);
	if (!arguments.valid)
		return 1;

	server = TCP_connection_init ();
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

	PM_watchlist_add (server->fd);

	while (1)
	{
		watchlistCounter = PM_watchlist_run ();
		if (watchlistCounter == -1)
		{
			die ("PM_watchlist_run", watchlistCounter, errno);
		}
		
		while (watchlistCounter--)
		{
			if (server && PM_watchlist_check (server->fd))					//Event from SERVER
			{
				PM_watchlist_clear (server->fd);
				
				//new client is connecting				
				callReturn = TCP_connection_accept (server, &inbound_connection);
				if (callReturn)	die ("TCP_connection_accept", callReturn, errno);

				//check if we're busy
				if (links_num == MAX_CONNECTION)				//TODO add queue, separate internal tcp buffers
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
				printf ("Client %s:%u connected.\nConnecting to %s:%u ...\n", inbound_connection->address, inbound_connection->port, outbound_connection->address, outbound_connection->port);

				
				callReturn = TCP_connection_connect (outbound_connection);	//connection
				if (callReturn == 11 && errno == ECONNREFUSED)
				{
					// No answer, quit current connection
					TCP_connection_destroy (outbound_connection);				
					callReturn = TCP_connection_close (inbound_connection);
					if (callReturn) die ("TCP_connection_close", callReturn, errno);			

					//clear pointers for next round
					inbound_connection = 0;
					outbound_connection = 0;
				}
				else if (callReturn)
					return 1;
				else
				{
					; //now do something
					//PM_watchlist_add (connection[0]->fd);

					PM_watchlist_add (inbound_connection->fd);
					PM_watchlist_add (outbound_connection->fd);

					//link together endpoints
					printf ("Linking %s:%u  <==>  %s:%u\n", inbound_connection->address, inbound_connection->port, outbound_connection->address, outbound_connection->port);
					temp_link = EL_link_open (inbound_connection, outbound_connection, cbv);
					if (!temp_link) die ("EL_link_open", 0, errno);
					_ELlink_add (temp_link);
					
					//clear pointers for next round
					inbound_connection = 0;
					outbound_connection = 0;
				}
			}

			link_list = &links;
			while (link_list && link_list->link)
			{
				if (EL_link_check (link_list->link))
				{
					callReturn = EL_link_manage (link_list->link);
					if (callReturn == 20)	//link destroyed
					{
						link_list = _ELlink_remove (link_list->link);
					}
					else if (callReturn)	//some errors
						die ("EL_link_manage", callReturn, errno);
				}
				else
				{
					link_list = link_list->next;
				}			
			}
		}
	}
	
	return 0;
}

