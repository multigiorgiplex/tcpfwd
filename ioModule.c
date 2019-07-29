/*IO functions and utils*/

#include "ioModule.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

int IO_die (char * file, int line, char * function, int result, int _errno)
{
	if (_errno)
		fprintf (stderr, "[%s:%d] @ %s() = %d - %s (errno: %d)\n", file, line, function, result, strerror (_errno), _errno);
	else
		fprintf (stderr, "[%s:%d] @ %s() = %d\n", file, line, function, result);	
	
	return result;
}
