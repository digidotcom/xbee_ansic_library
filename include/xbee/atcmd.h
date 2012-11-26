/*
 * Copyright (c) 2008-2012 Digi International Inc.,
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
	@addtogroup xbee_atcmd
	@{
	@file xbee/atcmd.h
	Sending AT command requests and processing the responses.
*/

#ifndef __XBEE_COMMAND
#define __XBEE_COMMAND

#include "xbee/device.h"

XBEE_BEGIN_DECLS

#ifndef XBEE_CMD_REQUEST_TABLESIZE
	/// Maximum number of outstanding requests.  Two should be sufficient in
	/// most (all?) cases.  One might even be enough if space is tight.
	#define XBEE_CMD_REQUEST_TABLESIZE	2
#endif

/// Maximum number of bytes in the parameter sent to a command.  The NI
/// command takes a 20-byte string.  ATZT is 48 bytes, ATZU is 22.
/// Platforms can override this when limiting to AT commands with shorter
/// parameters (e.g., 16 bytes when not using the certificate parameters).
#ifndef XBEE_CMD_MAX_PARAM_LENGTH
	#define XBEE_CMD_MAX_PARAM_LENGTH	48
#endif

/// Timeout (in seconds) to wait for a response to a remote command.
/// Set to a high value to allow for a sleeping end device.
#define XBEE_CMD_REMOTE_TIMEOUT		180

/// Timeout (in seconds) to wait for a response to a local command.
#define XBEE_CMD_LOCAL_TIMEOUT		2

/// Datatype used for passing and storing XBee AT Commands.  Allows printing
/// (e.g., printf( "%.2s", foo.str)), easy copying (bar.w = foo.w) and
/// easy comparison (bar.w == foo.w).
typedef union xbee_at_cmd {
	char		str[2];
	uint16_t		w;
} xbee_at_cmd_t;

// Values for ATEO command
#define XBEE_CMD_ATEO_INSECURE_JOIN			(1<<0)
#define XBEE_CMD_ATEO_USE_TRUST_CENTER		(1<<1)
#define XBEE_CMD_ATEO_USE_HASH_LINK_KEY	(1<<2)
#define XBEE_CMD_ATEO_USE_AUTHENTICATION	(1<<3)

// We need to declare struct _xbee_cmd_t here so the compiler doesn't treat it
// as a local definition in the parameter lists for the function pointer
// typedefs that follow.
struct _xbee_cmd_t;

/**
	Structure used to pass AT Command responses to registered callback handlers.
   This is used for both local and remote command responses.
*/
typedef struct xbee_cmd_response {
	/// Local XBee device that sent the local (or received the
	/// remote) response.
	xbee_dev_t					*device;

	/// Context registered with callback in call to xbee_cmd_set_callback().
	/// This is usually a pointer to extra data needed by the callback.
	void					FAR	*context;

	/// The handle to the request that generated this response. Useful if the
	/// callback needs to (optionally) modify the original request and then
	/// send it again.
	int16_t						handle;

	/// The AT Command (e.g., VR, NI) sent in the request that generated
	/// this response.
	xbee_at_cmd_t				command;

	/** Additional information about the response.  Perform a bitwise AND (&)
		with the following values:

		- #XBEE_CMD_RESP_MASK_STATUS
			Status byte from the XBee response.  A non-zero
			status indicates an error.  May be one of the
			following values:
				- #XBEE_AT_RESP_SUCCESS (0)
				- #XBEE_AT_RESP_ERROR
				- #XBEE_AT_RESP_BAD_COMMAND
				- #XBEE_AT_RESP_BAD_PARAMETER

		- #XBEE_CMD_RESP_FLAG_TIMEOUT
			Did not receive a response before timing out.

		- #XBEE_CMD_RESP_ATND_RSSI_INVALID [DigiMesh only]
			The RSSI field of the DigiMesh ATND response is not valid.
	*/
	uint16_t						flags;
	/** @name
		Macros used for \c flags field of xbee_cmd_response_t
		@{
	*/
		#define XBEE_CMD_RESP_MASK_STATUS			0x000F
		#define XBEE_CMD_RESP_ATND_RSSI_INVALID	XBEE_AT_RESP_ATND_RSSI_INVALID
		#define XBEE_CMD_RESP_FLAG_TIMEOUT			0x8000
	//@}

	/// Number of bytes in .value_bytes or zero if there wasn't a value sent
	/// with the response.
	uint_fast8_t				value_length;

	/// The value sent with the response; contains .value_length bytes.  Do not
	/// modify the data pointed to by .value_bytes, it may be sent to more than
	/// one callback.
	const uint8_t		FAR	*value_bytes;

	/// The value sent with the response, if any, in host byte order.  Only set
	/// if .value_length is 4 bytes or less.
	uint32_t						value;

	/// Source of the response (NULL if a local AT command response)
	const wpan_address_t	FAR	*source;
} xbee_cmd_response_t;

/** @name
	Return values for xbee_cmd_callback_fn() functions.
	@{
*/
/// Returned by an xbee_cmd_callback_fn() if the handle can be released.
#define XBEE_ATCMD_DONE		0
/// Returned by an xbee_cmd_callback_fn() if more reponses are expected,
/// or it has re-used the command handle.
#define XBEE_ATCMD_REUSE	1
//@}

/**
	@brief
	Callback registered to an AT Command with xbee_cmd_set_callback().

	It should not modify the contents of the xbee_cmd_response_t structure
	passed to it.  It should return either #XBEE_ATCMD_DONE or
	#XBEE_ATCMD_REUSE.

	View the help on xbee_cmd_response_t for details on the members of that
	structure.

	@param[in]	response		response received for AT Command

	@retval	XBEE_ATCMD_DONE	done with the request handle
	@retval	XBEE_ATCMD_REUSE	more responses are expected, or the request
										handle will be reused

	@see xbee_cmd_set_callback(), xbee_cmd_response_t
*/
typedef int (*xbee_cmd_callback_fn)(
	const xbee_cmd_response_t		FAR	*response
);

/**
	This structure is used to keep track of outstanding local and remote AT
	requests.
*/
typedef struct xbee_cmd_request {
	/// Rolling identifier used to prevent use of stale request indicies.
	/// Handle is a combination of index and this sequence byte.
	uint8_t			sequence;

	/// Device that sent this request -- if we have a table for each XBee, this
	/// element isn't necessary.  NULL if slot is empty.
	xbee_dev_t		*device;

	/// expire entry if XBEE_CHECK_TIMEOUT_SEC(timeout) is true
	uint16_t			timeout;

	/// combination of XBEE_CMD_FLAG_* macros
	uint16_t			flags;
	/** @name
		@{
	*/
		/// set in xbee_cmd_set_target() and used as a quick test for remote
		/// vs. local in xbee_cmd_send()
		#define XBEE_CMD_FLAG_REMOTE       	0x0001

		/// queue change until ATAC or another request w/o this flag
		#define XBEE_CMD_FLAG_QUEUE_CHANGE	0x0002

		/// don't automatically release non-response requests (no callback)
		/// after sending (necessary for functions that reuse handle)
		#define XBEE_CMD_FLAG_REUSE_HANDLE	0x0004

		/// mask of flags user can set (passed into xbee_cmd_set_flags)
		#define XBEE_CMD_FLAG_USER_MASK	(XBEE_CMD_FLAG_QUEUE_CHANGE	\
													|XBEE_CMD_FLAG_REUSE_HANDLE)
	//@}

/*
	we probably don't need this member

	uint8_t			status;			// status byte from AT response
	   #define XBEE_CMD_STATUS_PENDING			254
	   #define XBEE_CMD_STATUS_TIMEOUT         255
*/

	/// bytes sent in the request or returned in the response
	uint8_t						param[XBEE_CMD_MAX_PARAM_LENGTH];

	/// number of bytes stored in .param[]
	uint8_t						param_length;

	/// function to pass responses to
	xbee_cmd_callback_fn		callback;

	/// context to pass to callback function
	void					FAR	*context;

	/// frame_id of last request sent
	uint8_t						frame_id;

#ifndef XBEE_CMD_DISABLE_REMOTE
	/// address of target device (64-bit IEEE and 16-bit network)
	wpan_address_t				address;
#endif

	/// command to send
	xbee_at_cmd_t				command;
} xbee_cmd_request_t;

/// Header for AT Commands sent to the local (serially-attached) XBee.
typedef PACKED_STRUCT xbee_header_local_at_req {
	/// #XBEE_FRAME_LOCAL_AT_CMD (0x08) or #XBEE_FRAME_LOCAL_AT_CMD_Q (0x09)
	uint8_t				frame_type;

	/// 1 to 255, or 0 to suppress the Transmit Status response
	uint8_t				frame_id;

	/// two-character AT Command
	xbee_at_cmd_t		command;
} xbee_header_local_at_req_t;

/// Header to AT Commands sent to a remote XBee on the network.
typedef PACKED_STRUCT xbee_header_remote_at_req {
	/// #XBEE_FRAME_REMOTE_AT_CMD (0x17)
	uint8_t				frame_type;

	/// 1 to 255, or 0 to suppress the Transmit Status response
	uint8_t				frame_id;

	/// 64-bit IEEE address (big-endian) of target
	addr64				ieee_address;

	/// 16-bit network address (big-endian) of target
	uint16_t				network_address_be;

	/// Options byte (see XBEE_REMOTE_AT_OPT_* macros)
	uint8_t				options;
		/** @name
			Values for \c .options member of xbee_header_remote_at_req_t.
			@{
		*/
		/// Queue changes until ATAC command or another request with
		/// XBEE_REMOTE_AT_OPT_IMMEDIATE set.
		#define XBEE_REMOTE_AT_OPT_QUEUE			0x00

		/// Apply changes immediately, don't wait for ATAC command.
		#define XBEE_REMOTE_AT_OPT_IMMEDIATE	0x02
		//@}
	xbee_at_cmd_t		command;
} xbee_header_remote_at_req_t;


/// Useful typedef to create either a local or remote request frame.
typedef union xbee_header_at_request {
	/// First byte of header determines type (local or remote).
	uint8_t								frame_type;

	/// Use if .frame_type is #XBEE_FRAME_LOCAL_AT_CMD or
	/// #XBEE_FRAME_LOCAL_AT_CMD_Q.
	xbee_header_local_at_req_t		local;

	/// Use if .frame_type is #XBEE_FRAME_REMOTE_AT_CMD.
	xbee_header_remote_at_req_t	remote;
} xbee_header_at_request_t;

/// Possible values of \c status byte in xbee_header_local_at_resp_t and
/// xbee_frame_local_at_resp_t.
enum xbee_at_resp_status {
	/// Mask to ignore extra bits used in the status response.
	XBEE_AT_RESP_STATUS_MASK = 0x0F,
	XBEE_AT_RESP_SUCCESS = 0,
	XBEE_AT_RESP_ERROR = 1,
	XBEE_AT_RESP_BAD_COMMAND = 2,
	XBEE_AT_RESP_BAD_PARAMETER = 3,
	XBEE_AT_RESP_TX_FAIL = 4,

	/// The RSSI field of the ATND response is not valid [DigiMesh]
	XBEE_AT_RESP_ATND_RSSI_INVALID = 0x40,
};

/// Macro for checking the status byte of an AT response.  If this library
/// is compiled without DigiMesh support, we can use a simpler test.
#ifdef XBEE_DISABLE_DIGIMESH
	#define XBEE_AT_RESP_STATUS(var)				(var)
#else
	#define XBEE_AT_RESP_STATUS(var)				((var) & XBEE_AT_RESP_STATUS_MASK)
#endif


typedef PACKED_STRUCT xbee_header_local_at_resp {
   /// #XBEE_FRAME_LOCAL_AT_RESPONSE (0x88)
	uint8_t				frame_type;

	/// ID from request, used to match response to request.
	uint8_t				frame_id;
	xbee_at_cmd_t		command;

	/// See enum #xbee_at_resp_status.  Note that DigiMesh uses the upper
	/// nibble for additional flags -- use XBEE_AT_RESP_STATUS() macro when
	/// comparing this field to the xbee_at_resp_status enum.
	uint8_t				status;

} xbee_header_local_at_resp_t;

typedef PACKED_STRUCT xbee_header_remote_at_resp {
   /// #XBEE_FRAME_REMOTE_AT_RESPONSE (0x97)
	uint8_t				frame_type;

	/// ID from request, used to match response to request.
	uint8_t				frame_id;

	/// 64-bit IEEE address (big-endian) of responder.
	addr64				ieee_address;

	/// 16-bit network address (big-endian) of responder.
	uint16_t				network_address_be;

	/// Command from original request.
	xbee_at_cmd_t		command;

	/// See enum #xbee_at_resp_status.  Note that DigiMesh uses the upper
	/// nibble for additional flags -- use XBEE_AT_RESP_STATUS() macro when
	/// comparing this field to the xbee_at_resp_status enum.
	uint8_t				status;
} xbee_header_remote_at_resp_t;


/// Response to an AT Command sent to the local serially-connected XBee.
typedef PACKED_STRUCT xbee_frame_local_at_resp {

   xbee_header_local_at_resp_t header;
	/// First byte of multi-byte value.
	uint8_t				value[1];
} xbee_frame_local_at_resp_t;

/// Response to an AT Command sent to a remote XBee.
typedef PACKED_STRUCT xbee_frame_remote_at_resp {
   xbee_header_remote_at_resp_t header;

	/// First byte of multi-byte value.
	uint8_t				value[1];
} xbee_frame_remote_at_resp_t;

/// Useful typedef for casting either a local or remote response frame.
typedef union xbee_frame_at_response {
	/// First byte of header determines type (local or remote).
	uint8_t								frame_type;

	/// Use if .frame_type is #XBEE_FRAME_LOCAL_AT_RESPONSE.
	xbee_frame_local_at_resp_t		local;

	/// Use if .frame_type is #XBEE_FRAME_REMOTE_AT_RESPONSE.
	xbee_frame_remote_at_resp_t	remote;
} xbee_frame_at_response_t;


// ---- API for command lists ----

struct xbee_atcmd_reg_t;	// forward

/**
	@brief
	Callback registered for individual command list commands.

	@param[in]	response		response received for AT Command.  From this,
   					the \c context field points to the start of the entire
                  set of commands in the list being processed.
	@param[in]	reg			Table element for this command.  Usually,
   					the callback will dereference reg->context to find
                  any required auxiliary data for processing this command.
   @param[in,out] base		Base address of structure to populate with
   					response value (typical use).

   @note
   This callback is called for a command response, and is intended to
   be used for manipulating the raw response data into something suitable
   for the application.  There is no corresponding callback for a request
   (i.e. called before setting parameter data); the application is expected
   to perform setting parameter manipulation before invoking
   xbee_cmd_list_execute().
*/
typedef void (*xbee_command_list_fn)(
	const xbee_cmd_response_t FAR 		*response,
   const struct xbee_atcmd_reg_t FAR	*reg,
	void FAR										*base
);

/**
	@brief Status codes for xbee_cmd_list_status()

   We force these to be -Exxx codes (or 0).
*/
enum xbee_command_list_status {
	XBEE_COMMAND_LIST_RUNNING = -EBUSY,
	XBEE_COMMAND_LIST_DONE = 0,
	XBEE_COMMAND_LIST_TIMEOUT = -ETIMEDOUT,
	XBEE_COMMAND_LIST_ERROR = -EIO,
};


/**
	@brief Context data passed to command list processor.

   This struct (or another struct with this as its first member) must
   be used as the context for the command list processor.
*/
typedef struct xbee_command_list_context_t {
	/// Pointer to start of command list
	const struct xbee_atcmd_reg_t FAR	*list;
   /// Pointer to base address of data to populate with command responses
   void FAR										*base;
   /// Execution status
   enum xbee_command_list_status			status;
   /// Where we are in the command list (index of last entry submitted).
   uint16_t										index;
} xbee_command_list_context_t;


enum xbee_command_list_type {
	XBEE_CLT_NONE,		///< No default action.  Often used for callbacks.
	XBEE_CLT_COPY,		///< Copy response data byte-for-byte to base struct.
	XBEE_CLT_COPY_BE,	///< Copy response, changing expected big-endian to
   						///< host byte order, and store in base struct.
                     ///< Data must be 1,2 or 4 bytes, assumed unsigned.
   XBEE_CLT_SET,		///< Use data in base struct as parameter for command,
   						///< with binary data copied byte-for-byte.
   XBEE_CLT_SET_STR,	///< Use data in base struct as parameter for command,
   						///< with null-terminated string data copied byte-for-byte.
   XBEE_CLT_SET_BE,	///< Use data in base struct as parameter for command,
   						///< changing host byte order to big-endian.
                     ///< Data must be 1,2 or 4 bytes, assumed unsigned.
   XBEE_CLT_SET_8,	///< Set value to immediate 8-bit constant in .bytes
   						///< field of the xbee_atcmd_reg_t entry.  No response.
   XBEE_CLT_LAST,		///< Last command, any response is discarded, no further
   						///< command list entries are processed.  This is useful
                     ///< if ND is the last command, since the command list
                     ///< will terminate, but discovered nodes will be
                     ///< handled subsequently (asynchronously).
};

/**
	@brief Entry for table of XBee registers to query at startup.

   Use #_XBEE_ATCMD_REG() to populate _xbee_atcmd_query_regs[].
*/
typedef struct xbee_atcmd_reg_t {
	xbee_at_cmd_t	command;				///< command to send to XBee device
   uint8_t			flags;				///< Option flags (meaningful to callback)
   enum xbee_command_list_type type;///< Type of processing
   /** Callback function to be invoked when this command receives a response.
       May be NULL, in which case a default processing function will be
       invoked.  The default function will simply move \c bytes length
       of data to \c offset in \c base (according to the \c type field).
   */
   xbee_command_list_fn	callback;
	uint8_t			bytes;				///< number of bytes in response to copy
   											///< or, for .type == XBEE_CLT_SET_8, is
                                    ///< value to set.
	uint16_t			offset;				///< offset into base address to copy bytes
} xbee_atcmd_reg_t;

/**
	@brief Macro used in creating command list tables.

   This macro does not set a callback.

	@param[in]	c1		first character of AT command
	@param[in]	c2		second character of AT command
	@param[in]	type	one of xbee_command_list_type enumerated values:
   						XBEE_CLT_NONE - no action except to issue the command.
                        'obj' and 'field' parameters are ignored.
							XBEE_CLT_COPY - copy response data directly
                     XBEE_CLT_COPY_BE - copy numeric data to host byte order
                     XBEE_CLT_SET - set command parameter directly
                     XBEE_CLT_SET_STR - set command parameter as null-
                     	terminated string.
                     XBEE_CLT_SET_BE - set numeric command parameter
	@param[in]	obj	typedef name of object at base address
	@param[in]	field	field in obj in which to store result

   @note The 'type' parameter allows for either setting the data in the
   base structure as the command parameter (XBEE_CLT_SET*) or the
   converse, which is copying the command result back to the base
   structure (XBEE_CLT_COPY*).

	@return	xbee_atcmd_reg_t record for constructing command list tables

   @see #XBEE_ATCMD_REG_CB, #XBEE_ATCMD_REG_SET_8
*/
#define XBEE_ATCMD_REG(c1,c2,type,obj,field) \
	{ { { c1, c2 } }, 0, type, NULL, \
		(uint8_t) sizeof((*(obj *)NULL).field), \
		(uint8_t) offsetof( obj, field) }

/**
	@brief Macro used in creating command list tables.

	@param[in]	c1		first character of AT command
	@param[in]	c2		second character of AT command
	@param[in]	cb		callback function (prototype \a xbee_command_list_fn)
	@param[in]	flags	8-bit flags for callback (arbitrary).

	@return	xbee_atcmd_reg_t record for constructing command list tables

   @see #XBEE_ATCMD_REG
*/
#define XBEE_ATCMD_REG_CB(c1,c2,cb,flags) \
	{ { { c1, c2 } }, flags, XBEE_CLT_NONE, cb, 0, 0 }


/**
	@brief Macro used in creating command list tables.

   This is a combination of #XBEE_ATCMD_REG (moving data automatically)
   and #XBEE_ATCMD_REG_CB (calling a callback function).  The callback
   function is invoked after the data move.

	@param[in]	c1		first character of AT command
	@param[in]	c2		second character of AT command
	@param[in]	type	one of xbee_command_list_type enumerated values:
   						XBEE_CLT_NONE - no action except to issue the command.
                        'obj' and 'field' parameters are ignored.
							XBEE_CLT_COPY - copy response data directly
                     XBEE_CLT_COPY_BE - copy numeric data to host byte order
                     XBEE_CLT_SET - set command parameter directly
                     XBEE_CLT_SET_STR - set command parameter as null-
                     	terminated string.
                     XBEE_CLT_SET_BE - set numeric command parameter
	@param[in]	obj	typedef name of object at base address
	@param[in]	field	field in obj in which to store result
	@param[in]	cb		callback function (prototype \a xbee_command_list_fn)
	@param[in]	flags	8-bit flags for callback (arbitrary).

	@return	xbee_atcmd_reg_t record for constructing command list tables

   @see #XBEE_ATCMD_REG, #XBEE_ATCMD_REG_CB
*/
#define XBEE_ATCMD_REG_MOVE_THEN_CB(c1,c2,type,obj,field,cb,flags) \
	{ { { c1, c2 } }, flags, type, cb,		\
   	(uint8_t) sizeof((*(obj *)NULL).field), \
		(uint8_t) offsetof( obj, field) }


/**
	@brief Macro used in creating command list tables.

   This macro does not set a callback.

	@param[in]	c1		first character of AT command
	@param[in]	c2		second character of AT command
	@param[in]	value	unsigned 8-bit value (compile-time constant) to
   						set.

	@return	xbee_atcmd_reg_t record for constructing command list tables

   @see #XBEE_ATCMD_REG
*/
#define XBEE_ATCMD_REG_SET_8(c1,c2,value) \
	{ { { c1, c2 } }, 0, XBEE_CLT_SET_8, NULL, value, 0 }

/**
	@brief Macro used in creating command list tables.  Used to terminate
   table, with no special final callback.  This \b must be the last entry
   if used, or #XBEE_ATCMD_REG_END_CB must be used.
*/
#define XBEE_ATCMD_REG_END \
	{ { { '\0', '\0' } } }

/**
	@brief Macro used in creating command list tables.  Used to terminate
   table, with a command (usually ND or WR).  This \b must be the last entry
   if used.
*/
#define XBEE_ATCMD_REG_END_CMD(c1,c2) \
	{ { { c1, c2 } }, 0, XBEE_CLT_LAST, NULL, 0, 0 }

/**
	@brief Macro used in creating command list tables.  Used to terminate
   table, with a final callback.  This \b must be the last entry
   if used, or #XBEE_ATCMD_REG_END must be used.

	@param[in]	cb		callback function (prototype \a xbee_command_list_fn).
							@note The 'response' parameter to the callback will
                     be the final command response received.
	@param[in]	flags	8-bit flags for callback (arbitrary).
*/
#define XBEE_ATCMD_REG_END_CB(cb, flags) \
	{ { { '\0', '\0' } }, flags, XBEE_CLT_NONE, cb, 0, 0 }

#define XBEE_ATCMD_REG_VALID(ptr)	(ptr->command.w != 0)


int xbee_cmd_list_execute(
			xbee_dev_t *xbee,
 			xbee_command_list_context_t FAR *clc,
         const xbee_atcmd_reg_t FAR *list,
         void FAR *base,
			const wpan_address_t FAR *address
         );

enum xbee_command_list_status (xbee_cmd_list_status)(
			xbee_command_list_context_t FAR *clc);
#define xbee_cmd_list_status(clc)		((clc)->status)
// ---- End of API for command lists ----


/// Is entry \a index available (unused)?
#define XBEE_CMD_REQUEST_EMPTY(index)	\
										(xbee_cmd_request_table[index].device == NULL)

/// Return the handle for entry \a index (combination of \a index and
/// \a .sequence member of entry).
#define XBEE_CMD_REQUEST_HANDLE(index)	\
	((index << 8) || xbee_cmd_request_table[index].sequence)

// documented in xbee_atcmd.c
extern FAR xbee_cmd_request_t
									xbee_cmd_request_table[XBEE_CMD_REQUEST_TABLESIZE];

// all functions are documented in xbee_atcmd.c
int xbee_cmd_tick( void);
xbee_cmd_request_t FAR *_xbee_cmd_handle_to_address( int16_t handle);
int xbee_cmd_init_device( xbee_dev_t *xbee);
int xbee_cmd_query_device( xbee_dev_t *xbee, uint_fast8_t refresh);
int xbee_cmd_query_status( xbee_dev_t *xbee);

int16_t xbee_cmd_create( xbee_dev_t *xbee, const char FAR command[3]);
int _xbee_cmd_release_request( xbee_cmd_request_t FAR *request);
int xbee_cmd_release_handle( int16_t handle);
int xbee_cmd_set_command( int16_t handle, const char FAR command[3]);
int xbee_cmd_set_callback( int16_t handle, xbee_cmd_callback_fn callback,
	void FAR *context);
int xbee_cmd_set_target( int16_t handle, const addr64 FAR *ieee,
	uint16_t network_address);
int xbee_cmd_set_flags( int16_t handle, uint16_t flags);
int xbee_cmd_clear_flags( int16_t handle, uint16_t flags);
int xbee_cmd_set_param( int16_t handle, uint32_t value);
int xbee_cmd_set_param_bytes( int16_t handle, const void FAR *data,
	uint8_t length);
int xbee_cmd_set_param_str( int16_t handle, const char FAR *str);
int xbee_cmd_send( int16_t handle);
int xbee_cmd_simple( xbee_dev_t *xbee, const char FAR command[3],
	uint32_t value);
int xbee_cmd_execute( xbee_dev_t *xbee, const char FAR command[3],
	const void FAR *data, uint8_t length);

#define XBEE_FRAME_HANDLE_LOCAL_AT		\
	{ XBEE_FRAME_LOCAL_AT_RESPONSE, 0, _xbee_cmd_handle_response, NULL },	\
	{ XBEE_FRAME_MODEM_STATUS, 0, _xbee_cmd_modem_status, NULL }
#define XBEE_FRAME_HANDLE_REMOTE_AT		\
	{ XBEE_FRAME_REMOTE_AT_RESPONSE, 0, _xbee_cmd_handle_response, NULL }

int _xbee_cmd_handle_response( xbee_dev_t *xbee, const void FAR *rawframe,
	uint16_t length, void FAR *context);

int _xbee_cmd_modem_status( xbee_dev_t *xbee,
	const void FAR *payload, uint16_t length, void FAR *context);

//@}

/**
	@ingroup xbee_device
	@brief
	Programs with the ZCL Identify Server Cluster can call this macro in
	their main loop to have the XBee module's association LED flash fast
	(100ms cycle) when in Identify Mode.

	@param[in]	xbee			device to identify

	@sa	xbee_identify, zcl_identify_isactive, ZCL_CLUST_ENTRY_IDENTIFY_SERVER
*/
#define xbee_zcl_identify( xbee)	\
	xbee_identify( xbee, zcl_identify_isactive() > 0)

/**
	@ingroup xbee_device
	@brief
	Programs with the ZCL Identify Server Cluster can call this function in
	their main loop to have the XBee module's association LED flash fast
	(100ms cycle) when in Identify Mode.

	@param[in]	xbee			device to identify
	@param[in]	identify		TRUE if XBee should be in identify mode

	@sa	xbee_zcl_identify, zcl_identify_isactive,
			ZCL_CLUST_ENTRY_IDENTIFY_SERVER
*/
void xbee_identify( xbee_dev_t *xbee, bool_t identify);

XBEE_END_DECLS

// If compiling in Dynamic C, automatically #use the appropriate C file.
#ifdef __DC__
	#use "xbee_atcmd.c"
#endif

#endif

