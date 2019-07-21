#include <stdio.h>
#include <errno.h>
#include "common.h"
#include "tcpHandler.h"
#include "cliTasks.h"
#include "pollerManager.h"

#define POLLING_TIMEOUT		1000	/*1 second*/

int main(int argc, char **argv)
{
	CLI_Arguments arguments;
	tcpServer server;
	int watchlistCounter;

	TCP_server_init (&server);
	CLI_init ();
	PM_init (POLLING_TIMEOUT);
	
	CLI_welcomeMessage ();
	arguments = CLI_validateArguments (argc, argv);
	if (!arguments.valid)
		return 1;

	//~ CLI_printArguments (arguments);
	if (TCP_server_parse_input(&server, arguments.localAddress, arguments.localPort) == 10)
	{
		printf ("Error listening on %s:%u - Invalid IP address.\n", arguments.localAddress, arguments.localPort);
		return 1;
	}

	switch (TCP_server_listen (&server))
	{
		case 10:
		case 11:
		case 12:	
			perror ("TCP_server_listen()");
			return 1;

		case 0:
			printf ("Listening on %s:%u ...\n", server.listen_address, server.listen_port);
			break;
			
		default:
			printf ("TCP_server_listen(): unhandled error.\n");
			return 1;
	}

	while (1)
	{
		PM_watchlist_reset ();
		PM_watchlist_add (server.fd);

		watchlistCounter = PM_watchlist_run ();
		if (watchlistCounter == -1)
		{
			perror ("PM_watchlist_run()");
			return 1;
		}
		
		while (watchlistCounter--)
		{
			if 		(PM_watchlist_check (server.fd))
			{
				//new client is connecting
				PM_watchlist_clear (server.fd);
			}
		}
	}
	
	return 0;
}

