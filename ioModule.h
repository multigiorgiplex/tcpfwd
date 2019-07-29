#ifndef _IOMODULE_H
#define _IOMODULE_H

#include <errno.h>
int callReturn;
#define die(functionName, functionResult, functionErrno)		return	IO_die (__FILE__, __LINE__, functionName, functionResult, functionErrno);
#define die_soft(functionName, functionResult, functionErrno)	(void)	IO_die (__FILE__, __LINE__, functionName, functionResult, functionErrno);		//Do not return


int		IO_die		(char *, int, char *, int, int);

#endif
