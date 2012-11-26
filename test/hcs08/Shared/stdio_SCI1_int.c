/*
	Include this file in a project to have STDIO (printf, puts, gets, etc.)
	use SCI1 serial port (host UART) and you're using vSci1Rx as the interrupt
	handler for that port.
*/

#include "derivative.h"
#include "xbee/serial.h"

// Support function for printf() to send output to host UART.
void TERMIO_PutChar( char c)
{
	while (! SCI1S1_TDRE);
	SCI1D = (c == '\n') ? '\r' : c;
}
char TERMIO_GetChar()
{
	int ch;

	do {
		__RESET_WATCHDOG();
		ch = xbee_ser_getchar( &HOST_SERIAL_PORT);
	} while (ch < 0);

  return (char) ch;
}

