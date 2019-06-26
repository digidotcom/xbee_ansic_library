Platform Support: Freescale FRDM-KL25Z on mbed.org (ALPHA)
==========================================================

Overview
--------
The `ports/mbed` directory contains preliminary support for the Freescale 
FRDM-KL25Z platform with the mbed.org compiler.  The serial driver is 
hard-coded to work with a single XBee module on PTD3/PTD2.  It does not 
support CTS and RTS signals, which are necessary for reliable 
communications and to avoid overflowing the serial buffer on the XBee 
module.

From the `ports/mbed` directory, run `build.sh` to create 
`lib/xbee-mbed-kl25.zip`, which you can upload to any mbed project.


Building samples
----------------
Use the files in `samples/mbed-kl25` as the starting point for your 
project.  They demonstrate how to initialize the `xbee_dev_t` structure, 
send AT request and process AT responses.
