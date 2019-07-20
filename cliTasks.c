/*Handles Command Line Interface tasks*/

#include "cliTasks.h"
#include "common.h"
#include <stdio.h>
#include <string.h>



void CLI_welcomeMessage ()
{
	printf ("Welcome to " APPLICATION_NAME " - " APPLICATION_VERSION "\n" APPLICATION_DESCRIPTION "\n\n");
}

CLI_Arguments CLI_validateArguments (int argc, char ** argv)
{
	unsigned i;
	CLI_Arguments ret;
	CLI_ARGUMENT argument;

	memset (&ret, 0, sizeof (CLI_Arguments));

	if (argc < 3)
	{
		printf ("Expecting at least 3 arguments!\n");
		ret.valid = 0;
		return ret;
	}

	argument = CLI_ARGUMENT_TRIGGER;
	for (i=1; i<argc; i++)
	{
		if (argv[i][0] == '-')
		{
			if 		(argv[i][1] == 'r' && argv[i][2] == 'a')	//remote address
			{
				argument = CLI_ARGUMENT_REMOTE_ADDRESS;
			}
			else if (argv[i][1] == 'r' && argv[i][2] == 'p')	//remote port
			{
				argument = CLI_ARGUMENT_REMOTE_PORT;
			}
			else if (argv[i][1] == 'l' && argv[i][2] == 'a')	//local address
			{
				argument = CLI_ARGUMENT_LOCAL_ADDRESS;
			}
			else if (argv[i][1] == 'l' && argv[i][2] == 'p')	//local port
			{
				argument = CLI_ARGUMENT_LOCAL_PORT;
			}
			else 	//invalid
			{
				printf ("Invalid argument %s\n", argv[i]);
				ret.valid = 0;
				return ret;
			}
		}
		else
		{
			switch (argument)
			{
				case CLI_ARGUMENT_TRIGGER:
					printf ("Argument: %s, expecting a trigger first!\n", argv[i]);
					ret.valid = 0;
					return ret;

				case CLI_ARGUMENT_LOCAL_ADDRESS:
					ret.localAddress = argv[i];
					argument = CLI_ARGUMENT_TRIGGER;
					break;

				case CLI_ARGUMENT_REMOTE_ADDRESS:
					ret.remoteAddress = argv[i];
					argument = CLI_ARGUMENT_TRIGGER;
					break;

				case CLI_ARGUMENT_LOCAL_PORT:
				case CLI_ARGUMENT_REMOTE_PORT:
					argument = CLI_ARGUMENT_TRIGGER;
			}
		}
	}

	ret.valid = 1;
	return ret;
}

void CLI_printArguments (CLI_Arguments a)
{
	printf ("Valid: %u\n", a.valid);
	printf ("remoteAddress: %s\n", a.remoteAddress);
	printf ("remotePort: %u\n", a.remotePort);
	printf ("localAddress: %s\n", a.localAddress);
	printf ("localPort: %u\n", a.localPort);
}
