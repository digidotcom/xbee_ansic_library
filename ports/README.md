Platform Support
================
These sub-directories contain headers and source code for using the
Digi XBee ANSI C Library on various target platforms.  Details on each
platform appear in the `ports/<platform>/README.md` file.

Adding New Platforms
--------------------
Supporting a new hardware platform should be relatively straightforward.
Start by looking at the `xbee/platform.h` file and the
`ports/xxx/platform_config.h` file for a similar platform.  You'll
need to create a `ports/yyy/platform_config.h` for your new
YYY platform.

Next you'll need to create two .C files with support functions:

- `xbee_platform_yyy.c`: Must have `xbee_seconds_timer` and
  `xbee_millisecond_timer` functions to report elapsed time.  May also
  have an `xbee_readline` function if porting the interactive sample
  programs.

- `xbee_serial_yyy.c`: Needs to implement all of the functions defined in
  `xbee/serial.h`.  During development, you can probably stub out the
  following functions (which are only used by `xbee_firmware.c` and
  `xbee_atmode.c` at the moment) and implement them in a second phase of
  development:
   - `xbee_ser_tx_free` and `xbee_ser_rx_free` -- always return `MAX_INT`
   - `xbee_ser_tx_used` and `xbee_ser_rx_used` -- always return 0
   - `xbee_ser_tx_flush`, `xbee_ser_rx_flush`, `xbee_ser_break`,
     `xbee_ser_flowcontrol` and `xbee_ser_set_rts` -- do nothing
   - `xbee_ser_get_cts` -- always return 1

Then set up your build system with the correct include paths, and .C files
to link into your application.  If you define the macro 
`XBEE_PLATFORM_HEADER` to be the name of your platform header (e.g.,
`"ports/yyy/platform_config.h"`), the `xbee/platform.h` file will include
it automatically.

