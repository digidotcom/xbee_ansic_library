/*
 * Copyright (c) 2010-2012 Digi International Inc.,
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
	@addtogroup SXA
	@{
	@file xbee/sxa.h
	Header for Simplified XBee API

*/
#ifndef __XBEE_SXA
#define __XBEE_SXA

#include "xbee/atcmd.h"
#include "xbee/wpan.h"
#include "xbee/discovery.h"
#include "xbee/io.h"
#include "xbee/reg_descr.h"

XBEE_BEGIN_DECLS

/** @internal Flag data for register value cache status

For this library to work, structure packing must be enabled (accomplished
using the PACKED_STRUCT macro).  This is because the size of the actual
data must be derivable from the size of the containing structure, minus
the size of the following flags field.

The flags field takes on one of the #_SXA_CACHED_OK etc. values.
*/
typedef uint8_t sxa_cache_flags_t;

// Note: even though any one cache entry can have only one of the following
// values, we use a bitfield-like encoding (instead of ordinal) to make it
// easier for code to test for any-one-out-of-N states.
enum
{
/// @internal Cached register value status: value unknown (never read)
	_SXA_CACHED_UNKNOWN =	0x00,
/// @internal Cached register value status: value read successfully from device
	_SXA_CACHED_OK	=			0x01,
/// @internal Cached register value status: value could not be read from device
	_SXA_CACHED_ERROR =		0x02,
/// @internal Cached register value status: value is being read from device
	_SXA_CACHED_BUSY =		0x04,
/// @internal Cached register value status: value is pending a radio query
	_SXA_CACHED_PENDING =	0x08,
/// @internal Cached register value status: not a valid cache group
	_SXA_CACHED_BAD_GROUP =	0x10
};

/// @internal  Size of cache flags and any other prefix data in each sxa_cached_*
/// struct.  Each struct must have same prefix data.  Only the final
/// 'value' field may be different.
#define _SXA_CACHED_PREFIX_SIZE	(sizeof(sxa_cache_flags_t))

/**
	Cached generic register value (no data)
*/
typedef PACKED_STRUCT
{
	sxa_cache_flags_t	flags;	///< Cache status flags (#_SXA_CACHED_OK etc.)
} sxa_cached_t;

/**
	Cached uint8_t register value
*/
typedef PACKED_STRUCT
{
	sxa_cache_flags_t	flags;	///< Cache status flags (#_SXA_CACHED_OK etc.)
   uint8_t	value;	///< Cached value
} sxa_cached_uint8_t;

/**
	Cached uint16_t register value
*/
typedef PACKED_STRUCT
{
	sxa_cache_flags_t	flags;	///< Cache status flags (#_SXA_CACHED_OK etc.)
   uint16_t	value;	///< Cached value
} sxa_cached_uint16_t;

/**
	Cached uint32_t register value
*/
typedef PACKED_STRUCT
{
	sxa_cache_flags_t	flags;	///< Cache status flags (#_SXA_CACHED_OK etc.)
   uint32_t	value;	///< Cached value
} sxa_cached_uint32_t;

/**
	Cached extended address register value
*/
typedef PACKED_STRUCT
{
	sxa_cache_flags_t	flags;	///< Cache status flags (#_SXA_CACHED_OK etc.)
   addr64	value;	///< Cached value
} sxa_cached_addr64;

/**
	Cached string register value.  This is in the form of a macro which
   generates an in-line struct definition, since strings are arrays
   with generally different maximum lengths.

   One extra char is automatically added, for the null terminator.
*/
#define _SXA_CACHED_STRING(name, len) \
	PACKED_STRUCT { sxa_cache_flags_t flags; uint8_t value[len+1]; } name

/**
	Cached binary register value.  This is in the form of a macro which
   generates an in-line struct definition, since binaries are arrays
   with generally different maximum lengths.
*/
#define _SXA_CACHED_BIN(name, len) \
	PACKED_STRUCT { sxa_cache_flags_t flags; uint8_t value[len]; } name


struct sxa_cached_group_t;
struct sxa_node_t;

/**
	@brief
	Function registered for cache group update post-processing.

	@param[in]	sxa		Relevant SXA.
	@param[in]	cgroup	Cache group being updated (within sxa)

*/
typedef void (*sxa_cache_upd_fn)(
	struct sxa_node_t FAR			*sxa,
   const struct sxa_cached_group_t FAR	*cgroup
	);

/**
	@brief Cached register group value.

   This struct is used to manage related groups of registers.  A group
   is used when registers are almost always used together or when special
   actions need to be taken on updates or queries.

   @note There is a static (const) array of these set up for the standard
   groups (_sxa_default_cache_groups).  In sxa_node_t.groups points to
   this array by default.

   The function _sxa_cache_group_by_id() returns one of the default
   cached groups by looking up its ID.
*/
typedef struct sxa_cached_group_t
{
	/// Arbitrary numeric ID of this group (see #SXA_CACHED_NODE_ID etc.)
	uint16_t							id;
	/// Offset (within sxa_node_t) of cache group status flags (#_SXA_CACHED_OK etc.)
   /// This field is zero to indicate end of array of sxa_cached_group_t.
	uint16_t							flags_offs;
   /// If not NULL, is a command list used to update all cached values in the group
	const xbee_atcmd_reg_t FAR *get_list;
   /// If not NULL, is a function to call (after get_list, if any) in order to
   /// perform any necessary post processing.
   sxa_cache_upd_fn 				get_fn;
} sxa_cached_group_t;

/*-------------------------------------------------

	This section based on NDS implementation.

-------------------------------------------------*/

//
// Gateway capabilities.
// This is a set of features supported by the network and gateway.
// This set may depend on the hardware and firmware versions of
// the gateway and gateway radio currently in use.
// Individual nodes may not support all of these features.
//

// Advanced addressing: ability to send and receive to any endpoint and
// cluster ID on remote nodes.  Without this capability, only a single
// endpoint and cluster ID (e.g. remote serial port) may be used on each node.
#define	ZB_CAP_ADV_ADDR		0x00000001

// Ability to access ZigBee device objects
#define	ZB_CAP_ZDO				0x00000002

// Ability to access Digi device objects on remote nodes.
// DDO access to the gateway radio is always available.
#define	ZB_CAP_REMOTE_DDO		0x00000004

// Ability to update firmware on the gateway radio.
#define	ZB_CAP_GW_FW			0x00000008

// Ability to update firmware on remote nodes via the gateway.
#define ZB_CAP_REMOTE_FW    	0x00000010

// Supports ZigBee mesh networking
#define ZB_CAP_ZIGBEE       	0x00000020

// Supports ZigBee Pro/2007 mesh networking
#define ZB_CAP_ZBPRO        	0x00000040

// Supports DigiMesh networking
#define ZB_CAP_DIGIMESH     	0x00000080

// Supports parent/child relationship
#define ZB_CAP_CHILDREN     	0x00000100

// Supports ZigBee Smart Energy profile
#define ZB_CAP_SE     			0x00000200

/*-------------------------------------------------
-------------------------------------------------*/


/**
	This structure and the attendant functions provide a useful layer for
   Rabbit applications who wish to use a simplified interface to the XBee API.

	The Simplified XBee API (SXA) has the following facilities:
   - discovery of nodes and maintenance of a node table
   - access to configurable I/Os on the XBee module
   - management of cached register values
   - establishment of point-to-point reliable data streams (xbee_sxa_socket)
   		[Rabbit implementation only].

   The first three items are device-independent.  Data streams requires
   Rabbit-compatible CPUs on both ends.

   It is assumed that XBee modules are used.  This library depends on
   proprietary XBee functions, so interoperability with other manufacturers'
   devices is not expected.  In fact, since SXA currently supports only
   ATND node discovery (DDO) and not ZDO discovery, only XBee devices
   will show up in the node list.  For upward compatibility, code can
   call sxa_is_digi() to determine whether the node is a 'Digi' (i.e.
   XBee) device, and thus supports remote AT commands.  The .caps
   field has additional information about node capabilities.

   Node discovery is the most important service.  Each node is added to a
   linked list which is dynamically allocated.  The application deals with
   pointers to individual table elements, as returned by table searching
   API functions.  Each new node (identified uniquely by IEEE 64-bit
   address) has a table entry allocated.  The entry is never freed so that
   applications do not have to be concerned with dangling pointers.  The
   assumption is that the total set of XBee nodes ever discovered (in one
   power cycle) is bounded to a reasonably small number (maybe in the low
   hundreds).

   Each table entry contains basic addressing data for the node, some of
   which may be dynamically updated.  The IEEE address is guaranteed to be
   constant.  The table entry contains pointers (initially set NULL) which
   may be set to point to other dynamically allocated information such as
   I/O configuration.  Some of the additional information is used to
   maintain "cached" settings of remote node configuration, which is
   necessary to avoid redundant radio traffic.  Other information is used to
   record updated state on the remote device, and to implement scheduling for
   remote commands.
*/

typedef struct sxa_node_t
{
	struct sxa_node_t
   			FAR		*next;	///< Next in linked list (or NULL)
	struct sxa_node_t
   			FAR		*next_local;	///< Next in linked list of local
            								///< devices (or NULL)
   int16_t				index;	///< Index (order of discovery: 0, 1, 2...)
   xbee_dev_t			*xbee;	///< Local device through which discovered
   uint32_t				stamp;	///< Time stamp of last received message
   									///< from xbee_seconds_timer().
   uint32_t				caps;		///< Capabilites bitmask (ZB_CAP_*)
	xbee_node_id_t		id;		///< Instance of basic node addressing and
   									///< identification info
   /// Either NULL (for local device) or points to next field.  This is
   /// for convenience with functions which take NULL address to indicate
   /// the local device.
   wpan_address_t FAR *addr_ptr;
   /// IEEE and network addresses (for remote devices, else N/A)
   wpan_address_t		address;

	/// Cached register value handling: number of entries in .queued
   /// Range [0..#_SXA_MAX_QUEUED].  Requests in excess of the maximum
   /// will not be queued and the application will need to try again later.
	uint16_t				nqueued;
#ifndef _SXA_MAX_QUEUED
	/// Cached register value handling: maximum number of queued updates
	#define _SXA_MAX_QUEUED	100
#endif
	/// Cached register value handling: list of queued updates.
   /// This may be NULL if there are no outstanding updates.  The list is
   /// allocated only when being used.
   const struct _xbee_reg_descr_t FAR * FAR * queued;
	/// Cached register value handling: current update (index into .queued)
   /// If >= .nqueued, then no current
   /// update is being processed, so scan list for next update (if any).
   uint16_t				q_index;
   /// Cached register value handling: Single entry command list for single
   /// register (miscellaneous) updates.  2 elements used because 2nd must
   /// terminate the list.
   xbee_atcmd_reg_t	misc_reg[2];

#if 0
   /// Bitmask indicating which cache groups have been obtained.
   /// See #SXA_CACHED_NODE_ID etc.
   uint16_t				cache_mask;
   /// Bitmask (corresponding to .cache_mask) of queries which did not
   /// complete because of timeout or other error.  This field prevents
   /// endless retrying of failed requests.
   uint16_t				error_mask;
   /// Bitmask (corresponding to .cache_mask) of query which is currently
   /// running.  Only one bit set.
   uint16_t				doing_mask;
#endif

	const sxa_cached_group_t FAR *groups;
	const sxa_cached_group_t FAR *doing_group;

   /// Cache flags for node_id group
   sxa_cache_flags_t node_id_cf;

   /// Cache flags for I/O configuration group
   sxa_cache_flags_t io_config_cf;
   /// I/O shadow state and configuration.  This contains a CLC which
   /// is used for the other queries as well.
   xbee_io_t			io;

   // Following fields cached with SXA_CACHED_DEVICE_INFO group
   /// Cache flags for device info group
   sxa_cache_flags_t device_info_cf;
	/// Value of XBee module's AO (API options) register.  Also sets value
   /// 0xFF if the AO command is not supported (fails to read).
	uint8_t				ao;
#define XBEE_ATAO_NOT_SUPPORTED 0xFF
	/// Value of XBee module's HV register.
	uint16_t				hardware_version;
	/// Value of XBee module's VR register (4-bytes on some devices)
	uint32_t				firmware_version;
	/// Value of XBee module's DD register (device type)
	uint32_t				dd;

// Module field in device type (DD)
#define MS_MOD_UNSPEC		0x00000000  // Unspecified
#define MS_MOD_SERIES1		0x00010000  // 802.15.4 or ZigBee (Freescale)
#define MS_MOD_ZNET25		0x00020000  // ZNet 2.5 (Ember)
#define MS_MOD_ZB				0x00030000  // ZB/SE (Ember)
#define MS_MOD_XBEE900		0x00040000  // XBee DigiMesh 900
#define MS_MOD_XBEE24		0x00050000  // XBee DigiMesh 2.4
#define MS_MOD_XBEE868		0x00060000  // XBee 868
#define MS_MOD_XBEE900DP	0x00070000  // XBee DP 900
#define MS_MOD_ZB_S2C		0x000a0000  // ZB/SE on S2C (Ember EM357)

#define MS_MOD_MIN          MS_MOD_SERIES1
#define MS_MOD_MAX          MS_MOD_ZB_S2C


   /// Cache flags for DH/DL (destination address) group
   sxa_cache_flags_t dhdl_cf;
   /// Value of DH and DL registers, combined into a single logical field.
	/// You always want both, so there is a command list to get them.
   addr64				dest_addr;

   // Miscellaneous cached data.  This is mainly to support iDigi which
   // wants access to all possible device registers.  The names of the
   // fields correspond to the iDigi RCI element tags in
   // <do_command target="zigbee"><query_state>...
   sxa_cached_uint8_t		aggregation;
   sxa_cached_uint8_t		assoc_led;
   sxa_cached_uint8_t		beacon_response;
   sxa_cached_uint8_t		broadcast_hops;
   sxa_cached_uint8_t		cca_threshold;
   sxa_cached_uint8_t		channel;
   _SXA_CACHED_STRING(command_char, 1);
   sxa_cached_uint16_t		cluster_id;
   sxa_cached_uint16_t		command_timeout;
   sxa_cached_uint8_t		coord_assoc;
   sxa_cached_uint8_t		coord_enable;
   sxa_cached_uint8_t		delay_slots;
   sxa_cached_uint8_t		dest_endpoint;
   sxa_cached_uint8_t		device_options;
   sxa_cached_uint16_t		discover_timeout;
   sxa_cached_uint8_t		encrypt_enable;
   sxa_cached_uint8_t		encrypt_options;
   sxa_cached_uint8_t		end_assoc;
   _SXA_CACHED_BIN(ext_pan_id, 8);
   sxa_cached_uint16_t		flow_threshold;
   sxa_cached_uint16_t		guard_times;
   sxa_cached_uint8_t		hop_sequence;
   sxa_cached_addr64			io_address;
   sxa_cached_uint8_t		io_enable;
   sxa_cached_uint16_t		initial_pan_id;
   sxa_cached_uint8_t		join_time;
   sxa_cached_uint8_t		join_notification;
   sxa_cached_uint8_t		join_verification;
   _SXA_CACHED_BIN(link_key, 16);
   sxa_cached_uint8_t		mac_mode;
   sxa_cached_uint8_t		mac_retries;
   sxa_cached_uint8_t		max_hops;
   sxa_cached_uint8_t		mesh_retries;
   sxa_cached_uint8_t		multi_transmit;
   sxa_cached_uint8_t		net_delay_slots;
   _SXA_CACHED_BIN(network_key, 16);
   sxa_cached_uint16_t		network_watchdog;
   sxa_cached_uint8_t		packet_timeout;
   sxa_cached_uint16_t		pan_id;
   sxa_cached_uint16_t		polling_rate;
   sxa_cached_uint8_t		power_level;
   sxa_cached_uint8_t		power_mode;
   sxa_cached_uint8_t		rssi_timer;
   sxa_cached_uint16_t		scan_channels;
   sxa_cached_uint8_t		scan_duration;
   sxa_cached_uint8_t		serial_parity;
   sxa_cached_uint32_t		serial_rate;
   sxa_cached_uint16_t		sleep_count;
   sxa_cached_uint16_t		sleep_disassoc;
   sxa_cached_uint8_t		sleep_mode;
   sxa_cached_uint8_t		sleep_options;
   sxa_cached_uint16_t		sleep_period;
   sxa_cached_uint8_t		sleep_sample_rate;
   sxa_cached_uint16_t		sleep_time;
   sxa_cached_uint16_t		sleep_wake;
   sxa_cached_uint8_t		source_endpoint;
   sxa_cached_uint8_t		stack_profile;
   sxa_cached_uint8_t		stop_bits;
   sxa_cached_uint16_t		supply_threshold;
   sxa_cached_uint8_t		xbee_retries;
   sxa_cached_uint16_t		wake_host_delay;
   sxa_cached_uint8_t		zigbee_enable;

   sxa_cached_uint8_t		association;
   sxa_cached_uint16_t		children;
   sxa_cached_uint8_t		max_payload;
   sxa_cached_uint16_t		config_code;
   sxa_cached_uint8_t		verify_cert;
   sxa_cached_uint16_t		supply_voltage;
   sxa_cached_uint8_t		temperature;
   sxa_cached_uint8_t		duty_cycle;
   sxa_cached_uint8_t		rssi;
   sxa_cached_uint8_t		tx_power;
   sxa_cached_uint8_t		sleep_status;
   sxa_cached_uint16_t		sleep_time_op;
   sxa_cached_uint16_t		wake_time;
   sxa_cached_uint32_t		ack_failures;
   sxa_cached_uint32_t		cca_failures;
   sxa_cached_uint32_t		good_packets;
   sxa_cached_uint32_t		missed_syncs;
   sxa_cached_uint32_t		missed_sync_msg;
   sxa_cached_uint32_t		rf_errors;
   sxa_cached_uint32_t		tx_errors;

} sxa_node_t;

/**
	@brief This enum gives numeric identification to all cache groups.
*/
enum
{
/// Dummy for when no caching required
	SXA_CACHED_NONE,
/// Got .id.node_id (normally obtained by initial discovery, however
/// app can explicitly refresh if desired).
	SXA_CACHED_NODE_ID,
/// I/O configuration: ATD0 etc., in .io field.
	SXA_CACHED_IO_CONFIG,
/// Device information: HV, VR and DD
	SXA_CACHED_DEVICE_INFO,
/// Dest addr: DH,DL.  This is a group since it is exceptional in requiring
/// two commands to get a single (logical) 64 bit value.
	SXA_CACHED_DHDL,

/// Miscellaneous stuff (one of the sxa_cached_* sub-structures) - these
/// have their own individual cache status flags.
/// This should be the last entry, and not have an entry in the group
/// table.  Miscellaneous entries have their cache status flag immediately
/// before the binary data, and thus do not need a group table entry.
	SXA_CACHED_MISC = 255
};

extern const FAR sxa_cached_group_t _sxa_default_cache_groups[];
const sxa_cached_group_t FAR * _sxa_cache_group_by_id(uint16_t id);
void _sxa_update_io_config_group(
	sxa_node_t FAR			*sxa,
   const sxa_cached_group_t FAR	*cgroup);
void _sxa_process_version_info(
	const xbee_cmd_response_t FAR 		*response,
   const struct xbee_atcmd_reg_t FAR	*reg,
	void FAR										*base);


/*---------------------------------------------------------------------------*/
/*                     Simple XBee API public functions                      */
/*---------------------------------------------------------------------------*/
extern sxa_node_t FAR *sxa_table;
extern int sxa_table_count;
extern sxa_node_t FAR *sxa_local_table;

sxa_node_t FAR *sxa_node_by_addr( const addr64 FAR *ieee_be);
sxa_node_t FAR *sxa_local_node( const xbee_dev_t *xbee);
sxa_node_t FAR *sxa_node_by_name( const char FAR *name);
sxa_node_t FAR *sxa_node_by_index( int index);
sxa_node_t FAR *sxa_node_add( xbee_dev_t *xbee, const xbee_node_id_t FAR *node_id);
void sxa_node_table_dump( void);

#define sxa_list_head() sxa_table
sxa_node_t FAR * (sxa_list_head)(void);
#define sxa_list_count() sxa_table_count
int (sxa_list_count)(void);
#define sxa_xbee(s) ((s)->xbee)
xbee_dev_t * (sxa_xbee)(sxa_node_t FAR *sxa);
#define sxa_wpan_address(s) ((s)->addr_ptr)
wpan_address_t FAR * (sxa_wpan_address)(sxa_node_t FAR *sxa);

int sxa_get_digital_input( const sxa_node_t FAR *sxa, uint_fast8_t index);
int sxa_get_digital_output( const sxa_node_t FAR *sxa, uint_fast8_t index);
int16_t sxa_get_analog_input( const sxa_node_t FAR *sxa, uint_fast8_t index);
int sxa_set_digital_output( sxa_node_t FAR *sxa,
				uint_fast8_t index, enum xbee_io_digital_output_state state);
int sxa_io_configure( sxa_node_t FAR *sxa,
				uint_fast8_t index, enum xbee_io_type type);
int sxa_io_set_options( sxa_node_t FAR *sxa,
			uint16_t sample_rate, uint16_t change_mask);
void sxa_io_dump( sxa_node_t FAR *sxa);
int sxa_io_query( sxa_node_t FAR *sxa);
int sxa_io_query_status( sxa_node_t FAR *sxa);

sxa_node_t FAR * sxa_init_or_exit(xbee_dev_t *xbee,
							const wpan_endpoint_table_entry_t *ep_table,
                     int verbose
                     );
void _sxa_launch_update(sxa_node_t FAR *sxa,
								const struct _xbee_reg_descr_t FAR *xrd,
                        uint16_t cache_group);
void sxa_tick(void);
int sxa_is_digi(const sxa_node_t FAR *sxa);

/*---------------------------------------------------------------------------*/
/*                               Node Discovery                              */
/*---------------------------------------------------------------------------*/
int _sxa_disc_process_node_data( xbee_dev_t *xbee, const void FAR *raw,
	int length);
int _sxa_disc_atnd_response( xbee_dev_t *xbee, const void FAR *raw,
	uint16_t length, void FAR *context);
int _sxa_disc_handle_frame_0x95( xbee_dev_t *xbee, const void FAR *raw,
	uint16_t length, void FAR *context);
int _sxa_disc_cluster_handler( const wpan_envelope_t FAR *envelope,
	void FAR *context);



/**
	Macro for registering a handler to receive node announcements via both
	ATND responses and 0x95 frames received.  Note that the XBEE_FRAME_NODE_ID
	(0x95) handler is only necessary when ATAO=0.  When ATAO is not zero,
	the 0x95 frames come in as 0x91 frames on the Digi Data Endpoint.
*/
#define XBEE_FRAME_HANDLE_ND_RESPONSE		\
		{ XBEE_FRAME_LOCAL_AT_RESPONSE, 0, _sxa_disc_atnd_response, NULL },	\
		{ XBEE_FRAME_NODE_ID, 0, _sxa_disc_handle_frame_0x95, NULL }

#define XBEE_DISC_CLIENT_CLUST_ENTRY		\
	{ DIGI_CLUST_NODEID_MESSAGE, _sxa_disc_cluster_handler, NULL, 		\
		WPAN_CLUST_FLAG_INPUT | WPAN_CLUST_FLAG_NOT_ZCL }



/*---------------------------------------------------------------------------*/
/*                               XBee I/O control                            */
/*---------------------------------------------------------------------------*/
int _sxa_io_process_response( const addr64 FAR *ieee_be,
		const void FAR *raw, int length);
int _sxa_io_response_handler(xbee_dev_t *xbee, const void FAR *raw,
									 uint16_t length, void FAR *context);
int _sxa_io_cluster_handler(const wpan_envelope_t FAR *envelope,
								   void FAR *context);
int _sxa_remote_is_cmd_response_handler(xbee_dev_t *xbee, const void FAR *raw,
										uint16_t length, void FAR *context);
int _sxa_local_is_cmd_response_handler(xbee_dev_t *xbee, const void FAR *raw,
										uint16_t length, void FAR *context);

/*---------------------------------------------------------------------------*/
/*                             XBee cache control                            */
/*---------------------------------------------------------------------------*/
sxa_cache_flags_t sxa_cache_status(sxa_node_t FAR * sxa,
							const _xbee_reg_descr_t FAR * zb,
                     uint16_t cache_group);
void _sxa_set_cache_status(sxa_node_t FAR * sxa,
							const _xbee_reg_descr_t FAR * zb,
                     uint16_t cache_group,
                     sxa_cache_flags_t flags);
void FAR * sxa_cached_value_ptr(sxa_node_t FAR * sxa,
							const _xbee_reg_descr_t FAR * zb);
uint16_t sxa_cached_value_len(sxa_node_t FAR * sxa,
							const _xbee_reg_descr_t FAR * zb);
int sxa_find_pending_cache(sxa_node_t FAR * sxa,
							const _xbee_reg_descr_t FAR * zb);
int sxa_schedule_update_cache(sxa_node_t FAR * sxa,
							const _xbee_reg_descr_t FAR * zb);


/**
	Macro for registering a handler to receive I/O samples via both
	ATIS responses and 0x92 frames received.  Note that the XBEE_FRAME_IO_RESPONSE
	(0x92) handler is only necessary when ATAO=0.  When ATAO is not zero,
	the 0x92 frames come in as 0x91 frames on the Digi Data Endpoint.
*/
#define XBEE_FRAME_HANDLE_IS_RESPONSE		\
	{ XBEE_FRAME_REMOTE_AT_RESPONSE, 0, _sxa_remote_is_cmd_response_handler, NULL }, \
	{ XBEE_FRAME_LOCAL_AT_RESPONSE, 0, _sxa_local_is_cmd_response_handler, NULL },	\
   { XBEE_FRAME_IO_RESPONSE, 0, _sxa_io_response_handler, NULL }

#define XBEE_IO_CLIENT_CLUST_ENTRY		\
	{DIGI_CLUST_IODATA, _sxa_io_cluster_handler, NULL,	\
			WPAN_CLUST_FLAG_INPUT | WPAN_CLUST_FLAG_NOT_ZCL}

XBEE_END_DECLS

// If compiling in Dynamic C, automatically #use the appropriate C file.
#ifdef __DC__
	#use "xbee_sxa.c"
#endif

#endif		// __XBEE_SXA

//@}
