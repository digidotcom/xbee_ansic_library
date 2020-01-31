Digi XBee ANSI C Library: Sample Programs             {#samples}
=========================================
Each port includes a subset of the following sample programs.  Find
additional documentation in comments at the top of the source code.
Many of samples make use of shared source files in `samples/common`,
with platform-specific support code in each `samples/<target>` directory.

## General Samples

- **Apply Profile**:
  Connect to an XBee module and execute a sequence of AT Commands stored
  in a Legacy X-CTU Profile (`.PRO` file).

- **ATinter**:
  Short for "AT Interactive", this simple sample demonstrates the
  `xbee_atcmd` API for reading and setting AT parameters on the XBee
  radio.  This is a good starting sample to verify correct communication
  with an XBee module in API mode.

- **EBLinfo**:
  Displays information stored in the headers of XBee firmware images
  (`.EBL` files) used for XBee S2, S2B and S2C models.

- **GPM**:
  Sample program demonstrating the General Purpose Memory API, used to access
  flash storage on some XBee modules (868LP, 900HP, Wi-Fi).  This space is
  available for host use, and is also used to stage firmware images before
  installation.

- **Install EBIN**:
  Sends firmware updates (.ebin files) to XBee modules using the Gen3
  bootloader (including S3B, S6, S6B, XLR, Cellular, SX, SX868, S8).

- **Install EBL**:
  Sends firmware updates (.ebl and .gbl files) to XBee modules with
  Ember processors (e.g., ZigBee and Smart Energy firmware to XBee S2,
  S2B and S2C) and EFR32 processors (e.g., XBee3 Cellular, 802.15.4,
  DigiMesh, and Zigbee).

- **Network Scan**:
  Perform an `ATAS` "active scan" and report the results.  Intended for
  Zigbee and Wi-Fi networks, but could work for others after updating
  `src/xbee/xbee_scan.c` for new scan types.

- **Transparent Client**:
  Demonstrate the "transparent serial" cluster used by XBee modules in
  "AT mode", along with node discovery using the ATND command.  Send
  strings between XBee modules on a network.

- **User Data Relay**:
  Sample that sends/receives User Data Relay frames to Serial, Bluetooth
  and MicroPython destinations on XBee3 Cellular/802.15.4/Digimesh/Zigbee
  products.

- **XBee Term**:
  Simple, stand-alone terminal emulator to interact with a module's
  bootloader or its regular firmware in command mode

## Platform-Specific Samples

### Rabbit (Dynamic C) Samples

See `samples/rabbit/ReadMe.txt` for a description of that platform's
collection of sample programs.

### Windows (Win32) Samples

- **comports**:
  This sample doesn't use this library, but it's useful for seeing a
  list of what COM ports are available.

## Product-Specific Samples

### Programmable XBee Samples

- **PXBee OTA Update**:
  Test tool for sending OTA (over-the-air) firmware updates to
  Programmable XBee modules.

- **PXBee Update**:
  Install firmware update to a Programmable XBee module over a wired
  serial connection.

### XBee3 802.15.4, DigiMesh and Zigbee

- **Remote AT**:
  Send AT commands to remote nodes on the network.

- **XBee3 OTA tool**:
  Tool to send .ota (firmware) and .otb (file system) update images
  to remote devices.

- **XBee3 Secure Session**:
  Sample showing method to establish a Secure Session to a remote node.

- **XBee3 SRP Verifier**:
  Tool to configure salt and verifier for BLE and SecureSession access.

- **Zigbee OTA info**:
  Dump information from the header of .ota (firmware) and .otb (file
  system) update files.

### Cellular Samples

- **IPv4 Client**:
  Demonstrates communication via TCP over IPv4 frames.

- **SMS Client**:
  Demonstrates sending and receiving SMS messages.

- **Socket Test**:
  Tool used for manually/interactively exploring and testing
  xbee/xbee_socket.c APIs.

- **XBee netcat**:
  A version of the netcat (nc) tool that uses the XBee sockets API to
  open a TCP or UDP socket and pass its data to/from stdout/stdin.

- **XBee3 SRP Verifier**:
  Tool to configure salt and verifier for BLE access.

### Zigbee Samples

These samples target XBee modules joined to Zigbee-compliant networks.

- **Commissioning Client** and **Commissioning Server**:
  Client and server samples demonstrating the use of the ZCL Commissioning
  Cluster to configure network settings on a Zigbee device.

- **Zigbee Register Device**:
  Used to register a joining device to a trust center node of a network.

- **Zigbee Walker**:
  "Walks" a Zigbee device using discovery packets to list all ZCL
  attributes.  Starts with ZDO requests to enumerate a device's endpoints
  and clusters, and then uses ZCL requests to list the attribute ID, type
  and value for each cluster.

- **ZCLtime**:
  This utility converts a ZCL UTCTime value (32-bit number of seconds
  since 1/1/2000) to a `struct tm` and human-readable date string.
