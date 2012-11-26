XBee Samples for Rabbit/Dynamic C
=================================

Dynamic C 10.70 Release notes:


1) All samples assume the use of an XBee module with ZigBee "API mode"
   firmware, with ATAP set to 1 (default) and ATBD set to 7 (115200 bps).

2) If you're using hardware other than the RCM4510W or a BL4S100-series board,
	you may need to review the instructions at the top of
      Samples\XBee\xbee_config.h
   and make changes to the macros therein.

3) All samples will display the XBee radio's settings at startup; hardware
	version (HV), firmware version (VR), MAC address (SH/SL) and network
	address (MY).


Sample Programs (stand-alone):

*	serial_bypass.c: This is a stand-alone program that you can use to bridge
	the programming port of the Rabbit (Serial A) with the serial port of the
	XBee module.  This allows for use of X-CTU to configure and update the
	frimware on the XBee module via the programming cable's DIAG connector.

*	xbee_update_ebl.c: This is a stand-alone program that you can use to update
	the firmware on your XBee module.  It uses .EBL files that you can find in
	the update/ebl_files directory of your X-CTU installation.

Sample Programs (project files):

*	AT Interactive.dcp: This is a good sample to start with -- it ensures that
	the XBee module is set up properly and the Rabbit is able to communicate
	with it.  You can type "AT" commands to read/write the registers of the
	XBee module, which may be necessary to properly configure it or get it
	to join a network.

*  AT_remote.dcp: As above, except also allows selection of remote XBee
   modules.

*	Transparent Serial.dcp: This ZigBee sample uses Digi's Transparent Serial
	cluster to send and receive streams of data to/from remote XBee nodes.
	It works with XBee devices running "AT mode" firmware and sending all
	serial data to a single node.

*  SXA-io.dcp: Use the Simple XBee API (SXA) layer to manipulate XBee I/O
   pins on local or remote modules.  All of the SXA samples allow discovery
   of remote XBee modules and printing a list of available modules.  The
   target module is selected with keyboard commands.

*  SXA-command.dcp: Use SXA to issue AT commands to local and remote modules.

*  SXA-stream.dcp: Use SXA to send data streams between local and remote
	modules.

*  SXA-socket.dcp: Use SXA to set up a reliable data stream between local
   and remote modules, using the same API functions as would be used for
   TCP/IP connections.  A variation of TCP is used which provides equivalent
   reliability properties over a packetized link between two XBee modules as
   would be achieved over the Internet.  This sample requires two Rabbit
   modules, each with an XBee attached.

