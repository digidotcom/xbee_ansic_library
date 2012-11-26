/*
	Include this file in a project to have STDIO (printf, puts, gets, etc.)
	use SCI1 serial port (host UART).
*/

#include "derivative.h"

//#define _DISABLE

// Support function for printf() to send output to host UART.
void TERMIO_PutChar( char c)
{
	#ifdef _DISABLE
		(void) c;
	#else
		while (! SCI1S1_TDRE);
		SCI1D = (c == '\n') ? '\r' : c;
	#endif
}
char TERMIO_GetChar()
{
	#ifdef _DISABLE
		return 0;
	#else
		while (! SCI1S1_RDRF) __RESET_WATCHDOG();
		return SCI1D;
	#endif
}

