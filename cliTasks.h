#ifndef _CLITASKS_H
#define _CLITASKS_H

#include <errno.h>

typedef enum {
	CLI_ARGUMENT_TRIGGER = 0,
	CLI_ARGUMENT_REMOTE_ADDRESS,
	CLI_ARGUMENT_REMOTE_PORT,
	CLI_ARGUMENT_LOCAL_ADDRESS,
	CLI_ARGUMENT_LOCAL_PORT,
} CLI_ARGUMENT;

typedef struct {
	char *		remoteAddress;
	unsigned	remotePort;
	
	char *		localAddress;
	unsigned	localPort;
	
	char		valid;
} CLI_Arguments;

int callReturn;
#define die(functionName, functionResult, functionErrno)		return	CLI_die (__FILE__, __LINE__, functionName, functionResult, functionErrno);
#define die_soft(functionName, functionResult, functionErrno)			CLI_die (__FILE__, __LINE__, functionName, functionResult, functionErrno);		//Do not return

void			CLI_init				(void);
void			CLI_welcomeMessage 		(void);
CLI_Arguments	CLI_validateArguments	(int, char **);
void			CLI_printArguments		(CLI_Arguments);
int				CLI_die					(char *, int, char *, int, int);

#endif
