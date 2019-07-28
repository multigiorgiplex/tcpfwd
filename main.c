#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "common.h"
#include "tcpHandler.h"
#include "cliTasks.h"
#include "pollerManager.h"
#include "endpointLinker.h"

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

#define callResult(functionName)		fprintf ((callReturn) ? stderr : stdout, functionName"() [%d]: %s\n", callReturn, strerror (callReturn ? errno : 0));

#define POLLING_TIMEOUT		1000	/*1 second*/
#define MAX_CONNECTION		5

int callReturn;
struct callback_vector cbv;	//use for everything


int main(int argc, char **argv)
{
	CLI_Arguments arguments;
	tcpConnection * server;
	tcpConnection * inbound_connection;
	tcpConnection * outbound_connection;
	ELlink * link[1];
	
	int watchlistCounter;	

	CLI_init ();
	PM_init (POLLING_TIMEOUT);
	EL_init ();
	link[0] = 0;

	//populate callback vector
	cbv.check	= &PM_watchlist_check;
	cbv.clear	= &PM_watchlist_clear;
	cbv.remove	= &PM_watchlist_remove;
	cbv.close	= &TCP_connection_close;
	cbv.recv	= &TCP_connection_receive;
	cbv.send	= &TCP_connection_send;
	cbv.destroy	= &TCP_connection_destroy;
	
	CLI_welcomeMessage ();
	arguments = CLI_validateArguments (argc, argv);
	if (!arguments.valid)
		return 1;

	server = TCP_connection_init ();
	if (TCP_connection_parse_input(server, arguments.localAddress, arguments.localPort) == 10)
	{
		printf ("Invalid local listening IP address (-la) %s.\n", arguments.localAddress);
		return 1;
	}
	

	callReturn = TCP_connection_listen (server);
	switch (callReturn)
	{
		case 10:
		case 11:
		case 12:
		default:
			callResult ("TCP_server_listen");
			return 1;

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
			perror ("PM_watchlist_run()");
			return 1;
		}
		
		while (watchlistCounter--)
		{
			if (server && PM_watchlist_check (server->fd))					//Event from SERVER
			{
				PM_watchlist_clear (server->fd);
				
				//new client is connecting				
				callReturn = TCP_connection_accept (server, &inbound_connection);
				#ifdef _DEBUG
					callResult ("TCP_connection_accept");
				#endif				
				if (callReturn)	return 1;

				//check if we're busy
				if (link[0])				//TODO add queue, separate internal tcp buffers
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
					return 1;
				}
				printf ("Client %s:%u connected.\nConnecting to %s:%u ...\n", inbound_connection->address, inbound_connection->port, outbound_connection->address, outbound_connection->port);

				
				callReturn = TCP_connection_connect (outbound_connection);	//connection
				#ifdef _DEBUG
					callResult ("TCP_connection_connect");
				#endif
				if (callReturn == 11 && errno == ECONNREFUSED)
				{
					// No answer, quit current connection
					TCP_connection_destroy (outbound_connection);				
					callReturn = TCP_connection_close (inbound_connection);
					#ifdef _DEBUG
						callResult ("TCP_connection_close");
					#endif					

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
					link[0] = EL_link_open (inbound_connection, outbound_connection, cbv);
					if (link[0] == 0)
						return 1;
					
					//clear pointers for next round
					inbound_connection = 0;
					outbound_connection = 0;
				}
			}

			if (EL_link_check (link[0]))
			{
				callReturn = EL_link_manage (link[0]);
				if (callReturn == 20)	//link destroyed
					link[0] = 0;
				else if (callReturn)		//some errors
					return 1;
			}
		}
	}
	
	return 0;
}

