This table lists the XBee API Frame Types and the corresponding source
files written to handle them.

| Type | Description                 | Source Files |
|:----:|-----------------------------|--------------|
|      |      **Host to XBee**       |
| 0x00 | Tx Request: 64-bit address  |
| 0x01 | Tx Request: 16-bit address  |
| 0x07 | IPv4 Remote AT Command      |
| 0x08 | AT Command                  | xbee/atcmd.h, xbee_atcmd.c
| 0x09 | AT Command (Queued)         | xbee/atcmd.h, xbee_atcmd.c
| 0x10 | Transmit Request            | xbee/wpan.h, xbee_wpan.c
| 0x11 | Transmit Request (Explicit) | xbee/wpan.h, xbee_wpan.c
| 0x17 | Remote AT Command           | xbee/atcmd.h, xbee_atcmd.c
| 0x18 | Secured Remote AT Command   |
| 0x1A | IPv6 Tx Request             |
| 0x1B | IPv6 Remote AT Command      |
| 0x1F | SMS Transmit                | xbee/sms.h, xbee_sms.c
| 0x20 | IPv4 Transmit               | xbee/ipv4.h, xbee_ipv4.c
| 0x21 | Create Source Route         | xbee/route.h, xbee_route.c
| 0x23 | IPv4 Tx Request with TLS    |
| 0x24 | Register Joining Device     |
| 0x28 | Send Data Request           |
| 0x2A | Device Response             |
| 0x2B | Cellular Firmware Update    |
| 0x2C | BLE Unlock Request          |
| 0x2D | User Data Relay Tx          | xbee/user_data.h, xbee_user_data.c
|      |      **XBee to Host**       |
| 0x80 | Rx Packet: 64-bit address   |
| 0x81 | Rx Packet: 16-bit address   |
| 0x82 | Rx Packet: 64-bit address IO|
| 0x83 | Rx Packet: 16-bit address IO|
| 0x87 | IPv4 Remote AT Cmd Response |
| 0x88 | AT Command Response         | xbee/atcmd.h, xbee_atcmd.c
| 0x89 | Tx Status                   | xbee/delivery_status.h, xbee/tx_status.h, xbee_tx_status.c
| 0x8A | Modem Status                | xbee_device.c
| 0x8B | Transmit Status             | xbee/delivery_status.h, xbee/wpan.h, xbee_wpan.c
| 0x8D | Route Information           |
| 0x8E | Aggregate Addressing Update |
| 0x8F | IPv4 I/O Data Sample        |
| 0x90 | Receive                     | xbee/wpan.h, xbee_wpan.c
| 0x91 | Receive (Explicit)          | xbee/wpan.h, xbee_wpan.c
| 0x92 | I/O Data Sample             | xbee/io.h, xbee_io.c
| 0x94 | XBee Sensor Reading         |
| 0x95 | Node Identification         | xbee/discovery.h, xbee_discovery.c
| 0x97 | Remote AT Command Response  | xbee/atcmd.h, xbee_atcmd.c
| 0x98 | Extended Modem Status       |
| 0x9A | IPv6 Receive                |
| 0x9B | IPv6 Remote AT Cmd Response |
| 0x9F | SMS Receive                 | xbee/sms.h, xbee_sms.c
| 0xA0 | Over-the-Air Update Status  |
| 0xA1 | Route Record                | xbee/route.h, xbee_route.c
| 0xA2 | Device Authenticated        |
| 0xA3 | Many-to-One Route Request   | xbee/route.h, xbee_route.c
| 0xA4 | Register Joining Dev Status |
| 0xA5 | Join Notification           |
| 0xA7 | IPv6 I/O Data Sample        |
| 0xAB | Cellular FW Update Response |
| 0xAC | BLE Unlock Response         |
| 0xAD | User Data Relay Rx          | xbee/user_data.h, xbee_user_data.c
| 0xB0 | IPv4 Receive                | xbee/ipv4.h, xbee_ipv4.c
| 0xB8 | Send Data Response          |
| 0xB9 | Device Request              |
| 0xBA | Device Response Status      |
| 0xFE | Frame Error                 |
