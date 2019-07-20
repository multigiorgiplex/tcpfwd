#ifndef _CLITASKS_H
#define _CLITASKS_H

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



void			CLI_welcomeMessage 		(void);
CLI_Arguments	CLI_validateArguments	(int, char **);
void			CLI_printArguments		(CLI_Arguments);

#endif
