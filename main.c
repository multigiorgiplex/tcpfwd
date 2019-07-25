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

#define callResult(functionName)		fprintf ((callReturn) ? stderr : stdout, functionName"() [%d]: %s\n", callReturn, strerror (callReturn ? errno : 0));

#define POLLING_TIMEOUT		1000	/*1 second*/
#define MAX_CONNECTION		5

int main(int argc, char **argv)
{
	CLI_Arguments arguments;
	tcpConnection * server;
	tcpConnection * client;
	tcpConnection * connection[MAX_CONNECTION];
	
	int watchlistCounter;	
	int callReturn;

	CLI_init ();
	server = TCP_connection_init ();
	connection[0] = TCP_connection_init ();
	PM_init (POLLING_TIMEOUT);
	
	CLI_welcomeMessage ();
	arguments = CLI_validateArguments (argc, argv);
	if (!arguments.valid)
		return 1;

	//~ CLI_printArguments (arguments);
	if (TCP_connection_parse_input(server, arguments.localAddress, arguments.localPort) == 10)
	{
		printf ("Invalid local listening IP address (-la) %s.\n", arguments.localAddress);
		return 1;
	}
	if (TCP_connection_parse_input(connection[0], arguments.remoteAddress, arguments.remotePort) == 10)
	{
		printf ("Invalid remote connection IP address (-ra) %s.\n", arguments.remoteAddress);
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

		#ifdef _DEBUG
		case 0:
			printf ("Listening on %s:%u ...\n", server->address, server->port);
			break;
		#endif
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
			if (server && PM_watchlist_check (server->fd))
			{
				//new client is connecting				
				callReturn = TCP_connection_accept (server, &client);
				#ifdef _DEBUG
					callResult ("TCP_connection_accept");
				#endif				
				if (callReturn)	return 1;

				PM_watchlist_add (client->fd);
				printf ("Client %s:%u connected.\nConnecting to %s:%u ...\n", client->address, client->port, connection[0]->address, connection[0]->port);
				
				//attempt connection to the other side
				
				callReturn = TCP_connection_connect (connection[0]);
				#ifdef _DEBUG
					callResult ("TCP_connection_connect");
				#endif
				if (callReturn == 11 && errno == ECONNREFUSED)
				{
					// No answer, quit current connection					
					callReturn = TCP_connection_close (client);
					#ifdef _DEBUG
						callResult ("TCP_connection_close");
					#endif					
				}
				else if (callReturn)
					return 1;
				else
				{
					; //now do something
					PM_watchlist_add (connection[0]->fd);
				}				
				
				PM_watchlist_clear (server->fd);
			}

			if (client && PM_watchlist_check (client->fd))
			{
				printf ("Data in from client!\n");

				callReturn = TCP_connection_receive (client);
				#ifdef _DEBUG
					callResult ("TCP_connection_receive");
				#endif
				if (callReturn > 0)
					return 1;
				if (callReturn == -10)
				{
					printf ("Client disconnected!\n");
					TCP_connection_close (client);
					TCP_connection_close (server);	//?
					return 0;
				}
				printf ("Received from %s:%u - %d bytes: %s\n", client->address, client->port, client->buffer_len, client->buffer);


				connection[0]->buffer		= client->buffer;
				connection[0]->buffer_len	= client->buffer_len;
				callReturn = TCP_connection_send (connection[0]);
				#ifdef _DEBUG
					callResult ("TCP_connection_send");
				#endif
				if (callReturn)
					return 1;

				
				PM_watchlist_clear (client->fd);
			}			
			/*
			if (connection[0] && PM_watchlist_check (connection[0]->fd))
			{
				printf ("Dati in from connection[0]!\n");
				PM_watchlist_clear (connection[0]->fd);
			}
			*/		
		}
	}
	
	return 0;
}

