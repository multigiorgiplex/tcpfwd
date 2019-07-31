/*Handles Command Line Interface tasks*/

#include "cliTasks.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "common.h"
#include "endpointLinker.h"

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;


void CLI_welcomeMessage ()
{
	printf ("Welcome to " APPLICATION_NAME " - " APPLICATION_VERSION "\n" APPLICATION_DESCRIPTION "\n\n");
}

CLI_Arguments CLI_validateArguments (int argc, char ** argv)
{
	unsigned i;
	long int conv;
	char * check;
	CLI_Arguments ret;
	CLI_ARGUMENT argument;

	memset (&ret, 0, sizeof (CLI_Arguments));

	if (argc < 3)
	{
		printf ("Expecting at least 3 arguments!\n");
		ret.valid = 0; return ret;
	}

	argument = CLI_ARGUMENT_TRIGGER;
	for (i=1; i<argc; i++)
	{
		if (argv[i][0] == '-')
		{
			if 		(argv[i][1] == 'r' && argv[i][2] == 'a')	//remote address
				argument = CLI_ARGUMENT_REMOTE_ADDRESS;
			else if (argv[i][1] == 'r' && argv[i][2] == 'p')	//remote port
				argument = CLI_ARGUMENT_REMOTE_PORT;
			else if (argv[i][1] == 'l' && argv[i][2] == 'a')	//local address
				argument = CLI_ARGUMENT_LOCAL_ADDRESS;
			else if (argv[i][1] == 'l' && argv[i][2] == 'p')	//local port
				argument = CLI_ARGUMENT_LOCAL_PORT;
			else 	//invalid
			{
				printf ("Invalid argument %s\n", argv[i]);
				return ret;
			}
		}
		else
		{
			switch (argument)
			{
				case CLI_ARGUMENT_TRIGGER:
					printf ("Argument: %s, expecting a trigger first!\n", argv[i]);
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
					errno = 0;
					conv = strtol (argv[i], &check, 0);
					if (errno)
					{
						perror ("");
						return ret;
					}
					if (conv < 0)	//not used, a negative value is starting with "-" so is interpreted as a trigger
					{
						printf ("Argument %s: value can't be a negative number.\n", (argument==CLI_ARGUMENT_LOCAL_PORT) ? "-lp" : "-rp");
						return ret;
					}
					else if (conv > 0xFFFF)
					{
						printf ("Argument %s: value is out of range.\n", (argument==CLI_ARGUMENT_LOCAL_PORT) ? "-lp" : "-rp");
						return ret;
					}
					else if (*check)
					{
						printf ("Argument %s: value is an invalid number.\n", (argument==CLI_ARGUMENT_LOCAL_PORT) ? "-lp" : "-rp");
						return ret;
					}

					if (argument == CLI_ARGUMENT_LOCAL_PORT)
						ret.localPort = (unsigned int) conv;
					else
						ret.remotePort = (unsigned int) conv;
					
					argument = CLI_ARGUMENT_TRIGGER;
					break;
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

void CLI_init ()
{
	;
}

int CLI_die (char * file, int line, char * function, int result, int _errno)
{
	if (_errno)
		fprintf (stderr, "[%s:%d] @ %s() = %d - %s (errno: %d)\n", file, line, function, result, strerror (_errno), _errno);
	else
		fprintf (stderr, "[%s:%d] @ %s() = %d\n", file, line, function, result);	
	
	return result;
}

void CLI_printTotalTransferred (struct data_exchange * data)			//TODO consider ncurses adoption!
{
	FILE * dest = stdout;
	unsigned int up, dw;
	char b[]	= "B  ";
	char kb[]	= "kiB";
	char mb[]	= "MiB";
	char * up_unit, * dw_unit;
	
	if (data->download >= 1048576)
	{
		dw = data->download /*>> 20*/;
		dw_unit = mb;
	}
	else if (data->download >= 1024)
	{
		dw = data->download >> 10;
		dw_unit = kb;
	}
	else
	{
		dw = data->download;
		dw_unit = b;
	}
	
	if (data->upload >= 1048576)
	{
		up = data->upload/* >> 20*/;
		up_unit = mb;
	}
	else if (data->upload >= 1024)
	{
		up = data->upload >> 10;
		up_unit = kb;
	}
	else
	{
		up = data->upload;
		up_unit = b;
	}

	fprintf (dest,	"\rTotal Upstream: % 6u %s, Downstream: % 6u %s", up, up_unit, dw, dw_unit);
	fflush (dest);
}



