#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "common.h"
#include "tcpHandler.h"
#include "cliTasks.h"
#include "pollerManager.h"

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

#define POLLING_TIMEOUT		1000	/*1 second*/
#define MAX_CONNECTION		5

#define _DEBUG

int main(int argc, char **argv)
{
	CLI_Arguments arguments;
	tcpServer * server;
	tcpConnection * client;
	tcpConnection * connection[MAX_CONNECTION];
	
	int watchlistCounter;
	
	int callReturn;

	server = TCP_server_init (); 
	connection[0] = TCP_connection_init ();
	CLI_init ();
	PM_init (POLLING_TIMEOUT);
	
	CLI_welcomeMessage ();
	arguments = CLI_validateArguments (argc, argv);
	if (!arguments.valid)
		return 1;

	//~ CLI_printArguments (arguments);
	if (TCP_server_parse_input(server, arguments.localAddress, arguments.localPort) == 10)
	{
		printf ("Invalid local listening IP address (-la) %s.\n", arguments.localAddress);
		return 1;
	}
	if (TCP_connection_parse_input(connection[0], arguments.remoteAddress, arguments.remotePort) == 10)
	{
		printf ("Invalid remote connection IP address (-ra) %s.\n", arguments.remoteAddress);
		return 1;
	}

	callReturn = TCP_server_listen (server);
	switch (callReturn)
	{
		case 10:
		case 11:
		case 12:
		default:
			fprintf ((callReturn) ? stderr : stdout, "TCP_server_listen() [%d]: %s\n", callReturn, strerror (errno));
			return 1;

		#ifdef _DEBUG
		case 0:
			printf ("Listening on %s:%u ...\n", server->listen_address, server->listen_port);
			break;
		#endif
	}

	while (1)
	{
		PM_watchlist_reset ();
		PM_watchlist_add (server->fd);

		watchlistCounter = PM_watchlist_run ();
		if (watchlistCounter == -1)
		{
			perror ("PM_watchlist_run()");
			return 1;
		}
		
		while (watchlistCounter--)
		{
			if 		(PM_watchlist_check (server->fd))
			{
				//new client is connecting				
				callReturn = TCP_connection_accept (server, client);
				#ifdef _DEBUG 
					fprintf (callReturn ? stderr : stdout, "TCP_connection_accept() [%d]: %s\n", callReturn, strerror (errno)); 
				#endif
				if (callReturn)	return 1;
				
				printf ("Client %s:%u connected.\nConnecting to %s:%u ...\n", client->address, client->port, connection[0]->address, connection[0]->port);
				
				//attempt connection to the other side
				callReturn = TCP_connection_connect (connection[0]);
				#ifdef _DEBUG 
					fprintf (callReturn ? stderr : stdout, "TCP_connection_connect() [%d]: %s\n", callReturn, strerror (errno)); 
				#endif
				if (callReturn)	return 1;
				
				PM_watchlist_clear (server->fd);
			}
		}
	}
	
	return 0;
}

