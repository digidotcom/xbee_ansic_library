#include "mbed.h"
#include "xbee_platform.h"
 
Serial Stdio( USBTX,USBRX);

Ticker ticker;
uint32_t MS_TIMER = 0;
uint32_t SEC_TIMER = 0;

void mstimer_isr( void)
{
    if (++MS_TIMER % 1000 == 0) ++SEC_TIMER;
}

void xbee_platform_init( void)
{
    static bool_t init = TRUE;
    if (init)
    {
        init = FALSE;
        ticker.attach_us( &mstimer_isr, 1000);
        Stdio.baud( 115200);
    }
}

uint32_t xbee_seconds_timer( void)
{
    return SEC_TIMER;
}

uint32_t xbee_millisecond_timer( void)
{
    return MS_TIMER;
}

uint16_t _xbee_get_unaligned16( const void FAR *p)
{
    uint16_t retval;
    _f_memcpy( &retval, p, 2);

    return retval;
}

uint32_t _xbee_get_unaligned32( const void FAR *p)
{
    uint32_t retval;
    _f_memcpy( &retval, p, 4);

    return retval;
}

void _xbee_set_unaligned16( void FAR *p, uint16_t value)
{
    _f_memcpy( p, &value, 2);
}

void _xbee_set_unaligned32( void FAR *p, uint32_t value)
{
    _f_memcpy( p, &value, 4);
}

#include <ctype.h>

#define XBEE_READLINE_STATE_INIT            0
#define XBEE_READLINE_STATE_START_LINE      1
#define XBEE_READLINE_STATE_CONTINUE_LINE   2

// See xbee/platform.h for function documentation.
int xbee_readline( char *buffer, int length)
{
    static int state = XBEE_READLINE_STATE_INIT;
    int c;
    static char *cursor;

    if (buffer == NULL || length < 1)
    {
        return -EINVAL;
    }

    switch (state)
    {
        default:
        case XBEE_READLINE_STATE_INIT:
            

        case XBEE_READLINE_STATE_START_LINE:            // start of new line
            cursor = buffer;
            *buffer = '\0'; // reset string
            state = XBEE_READLINE_STATE_CONTINUE_LINE;
            // fall through to continued input

        case XBEE_READLINE_STATE_CONTINUE_LINE:     // continued input
            if (! Stdio.readable())
            {
                return -EAGAIN;
            }
            c = Stdio.getc();
            switch (c)
            {
                case 0x7F:              // backspace (Win32)
                case '\b':              // supposedly backspace...
                    if (buffer != cursor)
                    {
                        fputs("\b \b", stdout);     // back up, erase last character
                        cursor--;
                    }
                    break;

                case '\n':
                case '\r':
                    putchar('\n');
                    state = XBEE_READLINE_STATE_START_LINE;
                    return cursor - buffer;

                default:
                    if (isprint( c) && (cursor - buffer < length - 1))
                    {
                        *cursor++= c;
                        putchar(c);
                    }
               else
               {
                  putchar( '\x07');     // error beep -- bad char
               }
                    break;
            }
            *cursor = 0;        // keep string null-terminated
            fflush(stdout);
    }

    return -EAGAIN;
}
