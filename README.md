Digi XBee ANSI C Library
========================

The project is a collection of portable ANSI C code for communicating with
Digi International's [XBee] wireless radio modules in API mode.  This source
has been contributed by [Digi International] under the Mozilla Public
License v2.0.

This is a BETA quality software release, and has gone through a limited
QA cycle.

It currently supports the following platforms:

- POSIX operating systems (Windows/[Cygwin], Mac OS X, Linux, BSD) with gcc
- Windows with [MinGW] and gcc
- DOS using the [OpenWatcom] compiler
- Rabbit-brand microprocessors (using [Dynamic C] 10.70 or later)
- Freescale HCS08 with CodeWarrior 10.x (part of [Programmable XBee Dev Kit])

It provides an API for a host platform that communicates with an XBee radio
serially.  Some of the features include:

- Frame dispatcher that passes complete frames from the XBee radio out to
  multiple functions based on frame type.
- Interface for sending API frames serially, with automatic header and
  checksum.
- Message delivery that includes addressing information (short/long remote
  address, source/destination endpoint, cluster ID and profile ID).
- Respond to messages using addressing information from the envelope of
  a received message.
- Interface for sending "AT Command" frames.
- Perform Node Discovery and manage a list of nodes.
- Install firmware images (.EBL or .OEM files) on the XBee radio.
- Parse and display digital/analog I/O samples.

Contact [Tom Collins] to report bugs, request features or contribute code
to this project.  Please read through this document before getting started
with the code.

[Digi International]: http://www.digi.com/
[XBee]: http://www.digi.com/xbee/
[Tom Collins]: mailto:tom.collins@digi.com
[Cygwin]: http://www.cygwin.org/
[MinGW]: http://www.mingw.org/
[OpenWatcom]: http://www.openwatcom.org/
[Dynamic C]: http://www.digi.com/support/productdetail?pid=4978
[Programmable XBee Dev Kit]: http://www.digi.com/programmablexbeekit


Requirements
------------

- For POSIX, navigate to the `samples/posix` directory and run `make all`.
  (You'll need to have a standard gcc toolchain, with `make`, installed.)

- For Windows, view `samples/win32/readme.txt` for instructions on setting up
  MinGW and MSYS.

- For DOS, open `src/dos/xbee_driver.wpj` in OpenWatcom and build it to create
  `lib/dos/xbee_driver.lib` and `lib/dos/xbee_driver.lb1` which you can link
  into your programs.  Be sure the compilation options (processor and memory
  model) for your programs match the ones for the library.  You can probably
  compile many of the POSIX samples in OpenWatcom with minimal changes.

- For Rabbit, download and install Dynamic C 10.70 and read its documentation.

- For Freescale HCS08, download and install Digi's Programmable XBee Dev Kit
  and read its documentation.


Documentation
-------------
If you don't already have HTML documentation in the `doc/html` directory, you
can generate it with Doxygen.  Go to www.doxygen.org to download the installer.

Launch Doxywizard and open `Doxyfile` at the root directory of the driver.
Go to the "Run" tab and click the "Run doxygen" button to build the HTML
documentation files.  The doxygen output will produce warnings for undocumented
portions of the code -- that's OK and expected.

To view the documentation, open `doc/html/index.html` in a web browser.


XBee Firmware
-------------
Configure your XBee radio with DigiMesh, ZigBee or Smart Energy firmware in
"API" mode (with an API mode firmware or by setting ATAP=1).  The samples use
a default baud rate of 115200.  Configure the radio with ATAO set to 1 to use
extended transmit and receive frames.


ZigBee Firmware
---------------
When using ZigBee and Smart Energy firmware, keep the radio set to ATAO=1.
In your application, you will need to set AO to 3 at startup in order to
receive and process ZDO/ZDP requests used to discover your device's endpoints
and clusters.  On exit, set AO back to 1 so the radio will process those
requests (and Smart Energy devices can complete Key Establishment to join a
network).

    xbee_cmd_simple( &my_xbee, "AO", 3);

On ZigBee networks without Key Establishment, it should be safe to keep AO set
to 3.


File Descriptions
-----------------
Headers:

-   `xbee/platform.h`: Function prototypes for hardware abstraction layer.

-   `xbee/platform_xxx.h`: Additional headers, automatically included by
        xbee/platform.h, based on the target device.

-   `xbee/serial.h`: Platform-specific functions for managing the serial
        interface, reading and writing bytes, checking the XBee's CTS pin and
        setting the host's RTS pin.

-   `xbee/device.h`: Read frames from XBee module and dispatch to frame
        handlers, send full frames to XBee module.

-   `xbee/atcmd.h`: Working with local and remote "AT Command" frames for
        getting and setting XBee attributes and executing XBee commands.

-   `xbee/atmode.h`: Interfacing with XBee module in "AT mode", currently used
        for firmware updates on non-ZigBee modules.

-   `xbee/byteorder.h`: Byte-order-related functions used by multiple layers of
        the driver.

-   `xbee/cbuf.h`: Circular buffer used by the serial driver on some platforms,
        and the "transparent serial" cluster used in OTA updates.  Available
        for general use to upper layers of the stack.

-   `xbee/commissioning.h`: Code for supporting the ZCL Commissioning Cluster
        on XBee ZB modules.  See `zigbee/zcl_commissioning.h` for the required
        networking code.

-   `xbee/firmware.h`: Code for updating radio firmware via .ebl or .oem files.

-   `xbee/io.h`: Code for working with the analog and digitial I/O pins on the
        XBee module.

-   `xbee/ota_client.h`: Client code for sending OTA (over-the-air) firmware
        updates to Programmable XBee modules.

-   `xbee/ota_server.h`: Server code for advertising OTA capabilities on a
        device (typically a Programmable XBee module).

-   `xbee/sxa.h`: A "Simplified XBee API" with support for node table
        management, configurable I/O, point-to-point data streams and a
        connectionless datagram protocol.  Used for XBee-only (not general
        ZigBee) networks.

-   `xbee/wpan.h`: Glue layer between XBee device driver and WPAN/ZigBee
        network.

-   `xbee/xmodem.h`: Xmodem protocol layer, on top of xbee/serial layer, for
        sending firmware updates.

-   `xbee/xmodem_crc16.h`: Function to calculate 16-bit Xmodem CRC.

-   `wpan/types.h`: Types used by Wireless Personal Area Networks, not specific
        to the ZigBee protocol (e.g., can be used by DigiMesh).

-   `wpan/aps.h`: APS-layer of WPAN networks (endpoints/clusters).

-   `zigbee/zcl.h`: ZigBee Cluster Library, including general commands.

-   `zigbee/zcl_basic.h`: Basic Cluster for ZCL.

-   `zigbee/zcl_basic_attributes.h`: Used in main program to create a data
        structure and attribute for use in the device's endpoint table.

-   `zigbee/zcl_client.h`: Helper functions for creating ZCL client clusters.

-   `zigbee/zcl_commissioning.h`: Non-certified implementation of
        Commissioning Cluster (server).

-   `zigbee/zcl_identify.h`: ZCL Identify Cluster (server).

-   `zigbee/zcl_onoff.h`: ZCL OnOff Cluster (incomplete).

-   `zigbee/zcl_time.h`: ZCL Time Cluster (client and server).

-   `zigbee/zcl_types.h`: ZCL datatypes

-   `zigbee/zdo.h`: ZigBee Device Objects/ZigBee Device Profile layer.


Source (.c) directories:

-   `wpan`: source files for `include/wpan/*.h`

-   `xbee`: source files for `include/xbee/*.h`

-   `util`: Helper functions that may not be available on a given target.  In
        some cases (like `swapbytes.c`), it can be used until an optimized,
        assembly-only version of the function can be written for the target.

-   `zigbee`: source files for include/zigbee/*.h

-   `dos`: Files for OpenWatcom/DOS targets.

-   `win32`: Files for gcc/Windows (MinGW, not Cygwin) targets.

-   `rabbit`: Files for Dynamic C/Rabbit targets.

-   `hcs08`: Files for Codewarrior/Freescale HCS08 (Programmable XBee) target.

-   `posix`: Files for POSIX operating systems (Mac OS X, BSD, Linux, Cygwin).


Sample Programs
---------------
[Provide details on the supplied sample programs here.]


Adding to the Library
=====================

We welcome your contributions to this library of code, but ask that you
please follow these general guidelines.

Coding Style
------------
-	In general, code additions should match the style of existing code.

-	When naming things in the global namespace (functions, macros, global
    variables) use a consistent prefix in the names.

-	Set tabs to 3 spaces and limit line length to 80 characters.

-	Use Doxygen-formatted comments for functions, types, globals and macros.

-	Always use curly braces ({ and }) after if and else statements.  Follow
    style of existing files:

        if (foo)
        {
            do_bar();
        }
        else
        {
            do_baz();
        }

-	Declare pointers as `char *foo`, not `char* foo` or `char * foo`.


Portability
-----------
This driver has been designed for portability to multiple platforms, from a
resource-constrained 8-bit Freescale, up to a Windows PC.  As such, it's
important to keep the following in mind while working with the driver:

-	Leave BeginHeader/EndHeader comments in place -- they're used by the
    linker in Dynamic C on the Rabbit platform.

-	Use ANSI C (C89/C90 preferred, C99 features allowed if widely supported).
    To ease portability, code should not make use of GNU or other compiler
    extensions.

    Exceptions: Some pointers in the driver are labelled `FAR` to allow for
    use of far memory on the Rabbit and HCS08 targets.  Most target platforms
    define `FAR` as an empty macro.

-	Use types from `stdint.h` instead of `int`, `short` and `long`.  Unless
    a variable is used within a function for a loop (where `int` is fine),
    or used in a platform-specific file (e.g., as part of the hardware
    abstraction layer), you should make use of the standard types from
    `stdint.h`: `int8_t`, `uint8_t`, `int16_t`, `uint16_t`, `int32_t`,
    `uint32_t`.

-	Variables and struct members that are stored in non-host-byte-order should
    have `_le` or `_be` appended to their names (for big endian and little
    endian).  Use the functions from `xbee/byteorder.h` (`htobe16`, `le32toh`,
    `memcpy_htobe`, `memcpy_betole`, etc.) to convert between host and
    big/little endian byte order.  If you pretend that you don't know your
    host platform's endian-ness, you'll be forced to write portable code that
    never assumes a given byte order.

    *DO NOT* use `hton`, `ntoh`, `htons`, `ntohs`, `intel` or `intel16`.

    XBee frames store multi-byte values in big endian byte order.
    ZigBee frames store multi-byte values in little endian byte order.
    Target platforms can be big endian or little endian.

    Use `#if BYTE_ORDER == LITTLE_ENDIAN` or `#if BYTE_ORDER == BIG_ENDIAN`
    when code requires different execution paths depending on the target's
    byte order.

-   When working with ZCL attributes, make use of the ZCL types for 40-bit
    `zcl40_t`, 48-bit `zcl48_t`, and 64-bit `zcl64_t` values.  24-bit
    attributes should use `uint32_t` or `int32_t` and 56-bit attributes
    should use `zcl64_t` (ZCL automatically sign extends into the high byte).

-	Place code at the correct layer of the driver.  We're trying to keep the
    ZigBee layers (and above) separate from the XBee layers to allow for possible
    gateway applications (ZigBee to Ethernet, for example), and to limit the
    public release of ZigBee and Smart Energy code.


Adding New Platforms
--------------------

Supporting a new hardware platform should be relatively straightforward.  Start
by looking at the `xbee/platform.h` file and the `xbee/platform_xxx.h` file for
a similar platform.  You'll need to create a `xbee/platform_yyy.h` for your new
YYY platform.

Next you'll need to create two .C files with support functions:

-   `xbee_platform_yyy.c`: Must have `xbee_seconds_timer` and
        `xbee_millisecond_timer` functions to report elapsed time.  May also
        have an `xbee_readline` function if porting the interactive sample
        programs.

-   `xbee_serial_yyy.c`: Needs to implement all of the functions defined in
        `xbee/serial.h`.  During development, you can probably stub out the
        following functions (which are only used by `xbee_firmware.c` and
        `xbee_atmode.c` at the moment) and implement them in a second phase of
        development:
   - `xbee_ser_tx_free` and `xbee_ser_rx_free` -- always return `MAX_INT`
   - `xbee_ser_tx_used` and `xbee_ser_rx_used` -- always return 0
   - `xbee_ser_tx_flush`, `xbee_ser_rx_flush`, `xbee_ser_break`,
     `xbee_ser_flowcontrol` and `xbee_ser_set_rts` -- do nothing
   - `xbee_ser_get_cts` -- always return 1

Then set up your build system with the correct include paths, and .C files to
link into your application.  If you define the macro `XBEE_PLATFORM_HEADER` to
be the name of your platform header (e.g., `"xbee/platform_yyy.h"`), the
`xbee/platform.h` file will include it automatically.


License
-------

This software is open-source software.  Copyright Digi International, 2012.

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this file,
You can obtain one at http://mozilla.org/MPL/2.0/.

This software includes MPL 1.1 licensed code from the Mozilla project.  See
`include/jslong.h` and `src/util/jslong.c` for details.

