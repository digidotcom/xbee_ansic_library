/*
 * Copyright (c) 2009-2012 Digi International Inc.,
 * All rights not expressly granted are reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Digi International Inc. 11001 Bren Road East, Minnetonka, MN 55343
 * =======================================================================
 */

/**
	@addtogroup xbee_device
	@{
	@file xbee/device.h
	Device layer for XBee module interface.

	@def XBEE_DEV_MAX_DISPATCH_PER_TICK
		Maximum number of frames to dispatch per call to xbee_tick().
*/

#ifndef __XBEE_DEVICE
#define __XBEE_DEVICE

#include "xbee/platform.h"
#include "xbee/serial.h"
#include "wpan/types.h"
#include "wpan/aps.h"

XBEE_BEGIN_DECLS

/** @name
	Flags used by functions in this module.
	@{
*/
#define XBEE_DEV_FLAG_NONE		0x0000
//@}

#ifndef XBEE_DEV_MAX_DISPATCH_PER_TICK
	#define XBEE_DEV_MAX_DISPATCH_PER_TICK 5
#endif

/** Possible values for the \c frame_type field of frames sent to and
	from the XBee module.  Values with the upper bit set (0x80) are frames
	we receive from the XBee module.  Values with the upper bit clear are
	for frames we send to the XBee.
*/
enum xbee_frame_type {
	/// Send an AT Command to the local device (see xbee_atcmd.c,
	/// xbee_header_at_request_t). [ZigBee, DigiMesh]
	XBEE_FRAME_LOCAL_AT_CMD					= 0x08,

	/// Queue an AT command for batch processing on the local device.
	/// [ZigBee, DigiMesh]
	XBEE_FRAME_LOCAL_AT_CMD_Q				= 0x09,

	/// Send data to a default endpoint and cluster on a remote device.
	/// [ZigBee, DigiMesh, not Smart Energy]
	XBEE_FRAME_TRANSMIT						= 0x10,

	/// Send data to a specific endpoint and cluster on a remote device
	/// (see xbee_wpan.c). [ZigBee, DigiMesh]
	XBEE_FRAME_TRANSMIT_EXPLICIT			= 0x11,

	/// Send an AT command to a remote device on the network (see xbee_atcmd.c,
	/// xbee_header_at_request_t). [ZigBee, DigiMesh, not Smart Energy]
	XBEE_FRAME_REMOTE_AT_CMD				= 0x17,

	/// Create Source Route (used with many-to-one routing) [ZigBee]
	XBEE_FRAME_CREATE_SRC_ROUTE			= 0x21,

	/// Register Joining Device (add device to trust center's key table)
	/// [Smart Energy, coordinator]
	XBEE_FRAME_REG_JOINING_DEV				= 0x24,

	/// Response from local device to AT Command (see xbee_atcmd.c,
	/// xbee_cmd_response_t). [ZigBee, DigiMesh]
	XBEE_FRAME_LOCAL_AT_RESPONSE			= 0x88,

	/// Current modem status (see xbee_frame_modem_status_t). [DigiMesh, ZigBee]
	XBEE_FRAME_MODEM_STATUS					= 0x8A,

	/// Frame sent upon completion of a Transmit Request. [DigiMesh, ZigBee]
	XBEE_FRAME_TRANSMIT_STATUS				= 0x8B,

	/// Route Information Frame, sent for DigiMesh unicast transmissions with
	/// NACK or Trace Route Enable transmit options set. [DigiMesh]
	XBEE_FRAME_ROUTE_INFORMATION			= 0x8D,

	/// Output when a node receives an address update frame and modifies its
	/// DH/DL registers. [DigiMesh]
	XBEE_FRAME_AGGREGATE_ADDRESSING		= 0x8E,

	/// Data received on the transparent serial cluster, when ATAO is set to 0.
	/// [ZigBee, DigiMesh]
	XBEE_FRAME_RECEIVE						= 0x90,		// ATAO == 0

	/// Data received for specific endpoint/cluster (see xbee_wpan.c), when
	/// ATAO is non-zero. [ZigBee, DigiMesh]
	XBEE_FRAME_RECEIVE_EXPLICIT			= 0x91,		// ATAO != 0

	/// [ZigBee, not Smart Energy]
	XBEE_FRAME_IO_RESPONSE					= 0x92,

	/// [ZigBee, not Smart Energy]
	XBEE_FRAME_SENDOR_READ					= 0x94,

	/// [ZigBee, DigiMesh, not Smart Energy]
	XBEE_FRAME_NODE_ID						= 0x95,

	/// Response from remote device to AT Command (see xbee_atcmd.c,
	/// xbee_cmd_response_t). [ZigBee, DigiMesh, not Smart Energy]
	XBEE_FRAME_REMOTE_AT_RESPONSE			= 0x97,

	/// Over-the-Air Firmware Update Status [ZigBee, not Smart Energy]
	XBEE_FRAME_FW_UPDATE_STATUS			= 0xA0,

	/// Route records received in response to a Route Request. [ZigBee]
	XBEE_FRAME_ROUTE_RECORD					= 0xA1,

	/// Information on device authenticated on Smart Energy network.
	/// [Smart Energy, coordinator]
	XBEE_FRAME_DEVICE_AUTHENTICATED		= 0xA2,

	/// Many-to-One Route Request Indicator [ZigBee]
	XBEE_FRAME_ROUTE_REQUEST_INDICATOR	= 0xA3,

	/// Frame sent in response to Register Joining Device frame
	/// (XBEE_FRAME_REG_JOINING_DEV). [Smart Energy, coordinator]
	XBEE_FRAME_REG_JOINING_DEV_STATUS	= 0xA4,

	/// Frame notifying trust center that a device has attempted to
	/// join, rejoin or leave the network.  Enabled by setting bit 1 of ATDO.
	/// [Smart Energy, coordinator]
	XBEE_FRAME_JOIN_NOTIFICATION_STATUS	= 0xA5,
};

/**
	@name
	Values for the \c options field of many receive frame types.
*/
//@{
/// XBee Receive Options: packet was acknowledged [ZigBee, DigiMesh]
#define XBEE_RX_OPT_ACKNOWLEDGED    0x01

/// XBee Receive Options: broadcast packet [ZigBee, DigiMesh]
#define XBEE_RX_OPT_BROADCAST       0x02

/// XBee Receive Options: APS-encrypted packet [ZigBee]
#define XBEE_RX_OPT_APS_ENCRYPT		0x20

/// XBee Receive Options: packet from end device (if known) [ZigBee]
#define XBEE_RX_OPT_FROM_END_DEVICE	0x40		// appeared in ZB 2x7x

/// XBee Receive Options: Mask for transmission mode [DigiMesh]
#define XBEE_RX_OPT_MODE_MASK						0xC0

	/// XBee Receive Options: Mode not specified [DigiMesh]
	#define XBEE_RX_OPT_MODE_NONE						(0)

	/// XBee Receive Options: Point-Multipoint [DigiMesh]
	#define XBEE_RX_OPT_MODE_POINT_MULTIPOINT		(1<<6)

	/// XBee Receive Options: Repeater Mode [DigiMesh]
	#define XBEE_RX_OPT_MODE_REPEATER				(2<<6)

	/// XBee Receive Options: DigiMesh (not available on 10k product) [DigiMesh]
	#define XBEE_RX_OPT_MODE_DIGIMESH				(3<<6)
//@}

/// Max payload for all supported XBee types is 256 bytes.  Actual firmware used
/// may affect that size, and even enabling encryption can have an affect.
/// Smart Energy and ZigBee are limited to 128 bytes, DigiMesh is 256 bytes.
#ifndef XBEE_MAX_RFPAYLOAD
	#define XBEE_MAX_RFPAYLOAD 128
#endif

/// Max Frame Size, including type, is for 0x91, Receive Explicit.  Note that
/// this is only for received frames -- we send 0x11 frames with 20 byte header.
#define XBEE_MAX_FRAME_LEN		(XBEE_MAX_RFPAYLOAD + 18)


// We need to declare struct xbee_dev_t here so the compiler doesn't treat it
// as a local definition in the parameter lists for the function pointer
// typedefs that follow.
struct xbee_dev_t;


/** @name Function Pointer Prototypes
	Function pointer prototypes, forward declaration using "struct xbee_dev_t"
	instead of "xbee_dev_t" since we use the types in the xbee_dev_t definition.
*/
//@{

/**
	@brief
	Standard API for an XBee frame handler in xbee_frame_handlers global.
	These functions are only called when xbee_dev_tick() or wpan_tick()
	are called and a complete frame is ready for processing.

	@note 		There isn't an actual xbee_frame_handler_fn function in
					the XBee libraries.  This documentation exists as a template
					for writing frame handlers.

	@param[in] xbee
					XBee device that received frame.
	@param[in] frame
					Pointer to frame data.  Data starts with the frame type (the
					0x7E start byte and frame length are stripped by lower layers
					of the driver).
	@param[in] length
					Number of bytes in frame.
	@param[in] context
					Handler-specific "context" value, chosen when the handler was
					registered with xbee_frame_handler_add.

	@retval	0	successfully processed frame
	@retval	!0	error processing frame
*/
/*
					Possible errors that will need unique -Exxx return values:
	            -	Invalid xbee_dev_t
	            -	No wpan_if assigned to xbee_dev_t
	            -	Invalid length (must be > 0)
	            -	Frame pointer is NULL

	@todo			What will _xbee_frame_dispatch do with those return values?
					Does there need to be a return value equivalent to "thanks
					for that frame, but please remove me from the dispatch table"?
					Right now, the dispatcher is ignoring the return value.
					Could be useful when registering to receive status information
					on outbound frames -- once we have status, we don't need
					to receive any additional notifications.
*/
typedef int (*xbee_frame_handler_fn)(
	struct xbee_dev_t				*xbee,
	const void 				FAR	*frame,
	uint16_t 						length,
	void 						FAR	*context
);

/**
	@brief
	Function to check the XBee device's AWAKE pin to see if it is awake.

	@param[in] xbee	XBee device that received frame

	@retval	!0	XBee module is awake.
	@retval	0	XBee module is asleep.
*/
typedef int (*xbee_is_awake_fn)( struct xbee_dev_t *xbee);

/**
	@brief
	Function to toggle the /RESET pin to the XBee device.

	@param[in] xbee		XBee device that received frame
	@param[in] asserted	non-zero to assert /RESET, zero to de-assert
*/
typedef void (*xbee_reset_fn)( struct xbee_dev_t *xbee, bool_t asserted);

/// forward definition of structure defined in xbee/discovery.h
struct xbee_node_id_t;
/**
	@brief Function to process parsed Node ID messages.

	Programs can register a Node ID message handler with this signature to
	receive Node ID messages (either from ATND responses, Join Notifications,
	or as a result of Commissioning Button presses).

	@param[in]	xbee		XBee device that received the message
	@param[in]	rec		parsed Node ID message or NULL if a targeted discovery
								timed out

	@sa xbee_disc_add_node_id_handler, xbee_disc_remove_node_id_handler,
		xbee_disc_discover_nodes
*/
typedef void (*xbee_disc_node_id_fn)( struct xbee_dev_t *xbee,
		const struct xbee_node_id_t *rec);
//@}

typedef struct xbee_dispatch_table_entry {
	uint8_t						frame_type;	///< if 0, match all frames
	uint8_t						frame_id;	///< if 0, match all frames of this type
	xbee_frame_handler_fn	handler;
	void 					FAR	*context;
} xbee_dispatch_table_entry_t;



enum xbee_dev_rx_state {
	XBEE_RX_STATE_WAITSTART = 0,	///< waiting for initial 0x7E
	XBEE_RX_STATE_LENGTH_MSB,		///< waiting for MSB of length (first byte)
	XBEE_RX_STATE_LENGTH_LSB,		///< waiting for LSB of length (second byte)
	XBEE_RX_STATE_RXFRAME			///< receiving frame and/or trailing checksum
};

enum xbee_dev_flags
{
	XBEE_DEV_FLAG_CMD_INIT			= 0x0001,	///< xbee_cmd_init called
	XBEE_DEV_FLAG_QUERY_BEGIN		= 0x0002,	///< started querying device
	XBEE_DEV_FLAG_QUERY_DONE		= 0x0004,	///< querying completed
	XBEE_DEV_FLAG_QUERY_ERROR		= 0x0008,	///< querying timed out or error
	XBEE_DEV_FLAG_QUERY_REFRESH	= 0x0010,	///< need to re-query device
	XBEE_DEV_FLAG_QUERY_INPROGRESS= 0x0020,	///< query is in progress

	XBEE_DEV_FLAG_IN_TICK			= 0x0080,	///< in xbee_dev_tick

	XBEE_DEV_FLAG_COORDINATOR		= 0x0100,	///< Node Type is Coordinator
	XBEE_DEV_FLAG_ROUTER				= 0x0200,	///< Node Type is Router
	XBEE_DEV_FLAG_ENDDEV				= 0x0400,	///< Node Type is End Device
	XBEE_DEV_FLAG_ZNET				= 0x0800,	///< Firmware is ZNet
	XBEE_DEV_FLAG_ZIGBEE				= 0x1000,	///< Firmware is ZigBee
	XBEE_DEV_FLAG_DIGIMESH			= 0x2000,	///< Firmware is DigiMesh

	// (cast to int required by Codewarrior/HCS08 platform if enum is signed)
	XBEE_DEV_FLAG_USE_FLOWCONTROL	= (int)0x8000,	///< Check CTS before sending
};

enum xbee_dev_mode {
	XBEE_MODE_UNKNOWN = 0,	///< Haven't started communicating with XBee yet
	XBEE_MODE_BOOTLOADER,	/**< XBee is in the bootloader, not running
											firmware */

	// Modes used by "AT firmware" and some bootloaders:
	XBEE_MODE_API,				///< XBee is using API firmware
	XBEE_MODE_IDLE,			///< idle mode, data sent is passed to remote XBee
	XBEE_MODE_PRE_ESCAPE,	///< command mode, can send AT commands to XBee
	XBEE_MODE_POST_ESCAPE,	///< wait for guard-time ms before sending +++
	XBEE_MODE_COMMAND,		///< wait guard-time ms for "OK\r" before command mode
	XBEE_MODE_WAIT_IDLE,		///< waiting for OK response to ATCN command
	XBEE_MODE_WAIT_RESPONSE	///< sent a command and now waiting for a response
};

/**
	@note
	-	This structure must start with a wpan_dev_t so that the device
		can be used with the wpan library.

	-	Uses function pointers for setting the reset pin and reading the awake
		pin on the XBee.  User code should not call the reset function directly;
		use xbee_dev_reset() instead so the various network layers will know
		about the reset.
*/
typedef struct xbee_dev_t
{
	/// Generic WPAN device required by the \ref zigbee layers of the API.
	wpan_dev_t		wpan_dev;

	/// Platform-specific structure required by xbee_serial.c
	xbee_serial_t	serport;

	/// Optional function to control reset pin.
	xbee_reset_fn		reset;

	/// Optional function to read AWAKE pin.
	xbee_is_awake_fn	is_awake;

	/// Optional function to receive parsed Node ID messages.
	xbee_disc_node_id_fn		node_id_handler;

	/// Value of XBee module's HV register.
	uint16_t				hardware_version;
	/** @name
		Macros related to the \c hardware_version field of xbee_dev_t.
		@{
	*/
		#define XBEE_HARDWARE_MASK				0xFF00
		#define XBEE_HARDWARE_S1				0x1700
		#define XBEE_HARDWARE_S1_PRO			0x1800
		#define XBEE_HARDWARE_S2				0x1900
		#define XBEE_HARDWARE_S2_PRO			0x1A00
		#define XBEE_HARDWARE_900_PRO			0x1B00
		#define XBEE_HARDWARE_868_PRO			0x1D00
		#define XBEE_HARDWARE_S2B_PRO			0x1E00
		#define XBEE_HARDWARE_S2C_PRO			0x2100
		#define XBEE_HARDWARE_S2C				0x2200
		#define XBEE_HARDWARE_S3B				0x2300
		#define XBEE_HARDWARE_S8				0x2400
	//@}

	/// Value of XBee module's VR register (4-bytes on some devices)
	uint32_t				firmware_version;
	/** @name
		Macros related to the \c firmware_version field of xbee_dev_t.
		@{
	*/
		#define XBEE_PROTOCOL_MASK				0xF000
		// Series 2 (2.4 GHz) hardware
		#define XBEE_PROTOCOL_ZNET				0x1000
		#define XBEE_PROTOCOL_ZB				0x2000
		#define XBEE_PROTOCOL_SMARTENERGY	0x3000
		#define XBEE_PROTOCOL_ZB_S2C			0x4000
		#define XBEE_PROTOCOL_SE_S2C			0x5000
		// Series 4 (900 MHz) hardware
		#define XBEE_PROTOCOL_MESHLESS		0x1000
		#define XBEE_PROTOCOL_DIGIMESH		0x8000

		#define XBEE_NODETYPE_MASK				0x0F00
		#define XBEE_NODETYPE_COORD			0x0100
		#define XBEE_NODETYPE_ROUTER			0x0300
		#define XBEE_NODETYPE_ENDDEV			0x0900
	//@}

	/// Multi-purpose flags for tracking information about this device.
	enum xbee_dev_flags			flags;

	/// Buffer and state variables used for receiving a frame.
	struct rx {
		/// current state of receiving a frame
		enum xbee_dev_rx_state	state;

		/// bytes in frame being read; does not include checksum byte
		uint16_t						bytes_in_frame;

		/// bytes read so far
		uint16_t						bytes_read;

		/// bytes received, starting with frame_type, +1 is for checksum
		uint8_t	frame_data[XBEE_MAX_FRAME_LEN + 1];
	} rx;

	uint8_t		frame_id;				///< last frame_id used for sending

	// Need some state variables here if AT mode is supported (necessary when
	// using modules with AT firmware instead of API firmware, or when doing
	// firmware updates on DigiMesh 900 with API firmware).  Current state:
	// idle mode, command mode, pre-escape guard-time, post-escape guard-time.
	// Current timeout: value of MS_TIMER when guard-time expired or we expect
	// to return to idle mode from command mode.
	// If we track MS_TIMER from the last byte we sent to the XBee, we can
	// potentially skip over the pre-escape guard time and send the escape chars.

	/// Current mode of the XBee device (e.g., boot loader, API, command).
	#ifdef XBEE_DEVICE_ENABLE_ATMODE
		enum xbee_dev_mode	mode;
		uint32_t			mode_timer;		///< MS_TIMER value used for timeouts
		uint16_t			guard_time;		///< value of GT (default 1000) * 1ms
		uint16_t			idle_timeout;	///< value of CT (default 100) * 100ms
		char				escape_char;	///< value of CC (default '+')
	#endif
} xbee_dev_t;

/**
	@brief
	Macro function to get a pointer to the LSB of the radio's firmware version.

	Typically used to define ZCL_STACK_VERSION_ADDR for the Basic cluster.

	@param[in]	x	name of xbee_dev_t to reference \c firmware_version from.

	@return	address of the low byte of the firmware version, taking the
				processor's endian-ness into account
*/
#if BYTE_ORDER == BIG_ENDIAN
	#define XBEE_DEV_STACK_VERSION_ADDR(x)	((uint8_t *)&(x).firmware_version + 3)
#else
	#define XBEE_DEV_STACK_VERSION_ADDR(x)	((uint8_t *)&(x).firmware_version)
#endif

/**
	Static table used for dispatching frames.

	The application needs to define this table, and it should end with
	the XBEE_FRAME_TABLE_END marker.

*/
extern const xbee_dispatch_table_entry_t xbee_frame_handlers[];

#define XBEE_FRAME_TABLE_END		{ 0xFF, 0, NULL, NULL }

/// @addtogroup xbee_device
/// @{
uint8_t xbee_next_frame_id( xbee_dev_t *xbee);

int xbee_dev_init( xbee_dev_t *xbee, const xbee_serial_t *serport,
	xbee_is_awake_fn is_awake, xbee_reset_fn reset);

void xbee_dev_dump_settings( xbee_dev_t *xbee, uint16_t flags);
	#define XBEE_DEV_DUMP_FLAG_NONE			0x0000
#ifndef XBEE_DEV_DUMP_FLAG_DEFAULT
	#define XBEE_DEV_DUMP_FLAG_DEFAULT		XBEE_DEV_DUMP_FLAG_NONE
#endif

int xbee_dev_reset( xbee_dev_t *xbee);

int xbee_dev_tick( xbee_dev_t *xbee);

int xbee_frame_write( xbee_dev_t *xbee, const void FAR *header,
	uint16_t headerlen, const void FAR *data, uint16_t datalen,
	uint16_t flags);
#define XBEE_WRITE_FLAG_NONE		0x0000

void xbee_dev_flowcontrol( xbee_dev_t *xbee, bool_t enabled);
/// @}

// private functions exposed for unit testing

void _xbee_dispatch_table_dump( const xbee_dev_t *xbee);

uint8_t _xbee_checksum( const void FAR *bytes, uint_fast8_t length,
	uint_fast8_t initial);

int _xbee_frame_load( xbee_dev_t *xbee);

int _xbee_frame_dispatch( xbee_dev_t *xbee, const void FAR *frame,
	uint16_t length);


typedef PACKED_STRUCT xbee_frame_modem_status_t {
	uint8_t			frame_type;				//< XBEE_FRAME_MODEM_STATUS (0x8A)
	uint8_t			status;
} xbee_frame_modem_status_t;

/** @name
	Values for \c status member of xbee_frame_modem_status_t.
	@{
*/
/// XBee Modem Status: Hardware reset [ZigBee and DigiMesh]
#define XBEE_MODEM_STATUS_HW_RESET					0x00
/// XBee Modem Status: Watchdog timer reset [ZigBee and DigiMesh]
#define XBEE_MODEM_STATUS_WATCHDOG					0x01
/// XBee Modem Status: Joined network (routers and end devices) [ZigBee]
#define XBEE_MODEM_STATUS_JOINED						0x02
/// XBee Modem Status: Disassociated (left network) [ZigBee]
#define XBEE_MODEM_STATUS_DISASSOC					0x03
/// XBee Modem Status: Coordinator started [ZigBee]
#define XBEE_MODEM_STATUS_COORD_START				0x06
/// XBee Modem Status: Network security key was updated [ZigBee]
#define XBEE_MODEM_STATUS_NETWORK_KEY_UPDATED	0x07
/// XBee Modem Status: Network Woke Up [DigiMesh]
#define XBEE_MODEM_STATUS_WOKE_UP					0x0B
/// XBee Modem Status: Network Went To Sleep [DigiMesh]
#define XBEE_MODEM_STATUS_SLEEPING					0x0C
/// XBee Modem Status: Voltage supply limit exceeded (XBee-PRO only) [ZigBee]
#define XBEE_MODEM_STATUS_OVERVOLTAGE				0x0D
/// XBee Modem Status: Key establishment complete [Smart Energy]
#define XBEE_MODEM_STATUS_KEY_ESTABLISHED			0x10
/// XBee Modem Status: Modem config changed while join in progress [ZigBee]
#define XBEE_MODEM_STATUS_CONFIG_CHANGE_IN_JOIN	0x11
/// XBee Modem Status: Network stack error [ZigBee]
#define XBEE_MODEM_STATUS_STACK_ERROR				0x80
//@}

/**
	@brief
	Frame handler for 0x8A (XBEE_FRAME_MODEM_STATUS) frames -- dumps modem status
	to STDOUT for debugging purposes.

	View the documentation of xbee_frame_handler_fn() for this function's
	parameters and return value.

	@see XBEE_FRAME_MODEM_STATUS_DEBUG, xbee_frame_handler_fn()
*/
int xbee_frame_dump_modem_status( xbee_dev_t *xbee,
	const void FAR *frame, uint16_t length, void FAR *context);

/**
	Add this macro to the list of XBee frame handlers to have modem status changes
	dumped to STDOUT.
*/
#define XBEE_FRAME_MODEM_STATUS_DEBUG	\
	{ XBEE_FRAME_MODEM_STATUS, 0, xbee_frame_dump_modem_status, NULL }

XBEE_END_DECLS

// If compiling in Dynamic C, automatically #use the appropriate C file.
#ifdef __DC__
   #use "xbee_device.c"
#endif

#endif		// #ifdef __XBEE_DEVICE
