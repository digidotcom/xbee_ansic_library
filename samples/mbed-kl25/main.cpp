#include "mbed.h"
#include "xbee_platform.h"

#include <stdio.h>
#include "_atinter.h"

xbee_dev_t my_xbee;

void wait_startup( void)
{
    uint16_t t;
    
    printf( "waiting for XBee to start up...\n");
    t = XBEE_SET_TIMEOUT_MS( 3000);     // 3 seconds
    while (! XBEE_CHECK_TIMEOUT_MS( t));
}

/*
    main

    Initiate communication with the XBee module, then accept AT commands from
    STDIO, pass them to the XBee module and print the result.
*/
int main()
{
    char cmdstr[80];
    int status;
    xbee_serial_t XBEE_SERPORT;

    XBEE_SERPORT.baudrate = 115200;

    // initialize the serial and device layer for this XBee device
    if (xbee_dev_init( &my_xbee, &XBEE_SERPORT, NULL, NULL))
    {
        printf( "Failed to initialize device.\n");
        return 0;
    }

    wait_startup();
    
    // Initialize the AT Command layer for this XBee device and have the
    // driver query it for basic information (hardware version, firmware version,
    // serial number, IEEE address, etc.)
    xbee_cmd_init_device( &my_xbee);
    printf( "Waiting for driver to query the XBee device...\n");
    do {
        xbee_dev_tick( &my_xbee);
        status = xbee_cmd_query_status( &my_xbee);
    } while (status == -EBUSY);
    if (status)
    {
        printf( "Error %d waiting for query to complete.\n", status);
    }

    // report on the settings
    xbee_dev_dump_settings( &my_xbee, XBEE_DEV_DUMP_FLAG_DEFAULT);

    printATCmds( &my_xbee);

    while (1)
    {
        while (xbee_readline( cmdstr, sizeof cmdstr) == -EAGAIN)
        {
            xbee_dev_tick( &my_xbee);
        }

        if (! strncmpi( cmdstr, "menu", 4))
        {
            printATCmds( &my_xbee);
        }
        else
        {
            process_command( &my_xbee, cmdstr);
        }
    }
}

#include "xbee_atcmd.h"         // for XBEE_FRAME_HANDLE_LOCAL_AT
#include "xbee_device.h"
const xbee_dispatch_table_entry_t xbee_frame_handlers[] =
{
    XBEE_FRAME_HANDLE_LOCAL_AT,
    XBEE_FRAME_MODEM_STATUS_DEBUG,
    XBEE_FRAME_TABLE_END
};
