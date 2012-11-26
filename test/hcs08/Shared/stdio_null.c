/*
	Include this file in a project to disable STDIO.
*/

#include "derivative.h"

void TERMIO_PutChar( char c)
{
	(void) c;
}
char TERMIO_GetChar()
{
	return 0;
}

