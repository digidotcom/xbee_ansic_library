/*
 * Copyright (c) 2024 Digi International Inc.,
 * All rights not expressly granted are reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Digi International Inc., 9350 Excelsior Blvd., Suite 700, Hopkins, MN 55343
 * ===========================================================================
 */

/**
    @addtogroup xbee_gnss
    @{
    @file xbee/gnss_frames.h

    Frame definitions and support functions for GNSS frames
    (0x3D and 0xBD-0xBF).  For use with XBee Cellular devices that support
    GNSS (Global Navigation Satellite System).
*/

#ifndef XBEE_GNSS_H
#define XBEE_GNSS_H

#include "xbee/platform.h"
#include "xbee/device.h"

#if !XBEE_CELLULAR_ENABLED
   #error "XBEE_CELLULAR_ENABLED must be defined as non-zero " \
      "to use this header."
#endif

XBEE_BEGIN_DECLS

// GNSS frame types
#define XBEE_FRAME_GNSS_START_STOP 0x3D
#define XBEE_FRAME_GNSS_START_STOP_RESP 0xBD
#define XBEE_FRAME_GNSS_RAW_NMEA_RESP 0xBE
#define XBEE_FRAME_GNSS_ONE_SHOT_RESP 0xBF

/// Format of XBee API frame type 0x3D (#XBEE_FRAME_GNSS_START_STOP).
/// Sent from host to XBee, which responds with XBEE_FRAME_START_STOP_RESP.
typedef XBEE_PACKED(xbee_frame_gnss_start_stop_t, {
    uint8_t     frame_type;     ///< XBEE_FRAME_GNSS_START_STOP (0x3D)
    uint8_t     frame_id;       ///< identifier to match response frame
    uint8_t     request;
#define XBEE_GNSS_REQUEST_START_ONE_SHOT          0       ///< Start one shot
#define XBEE_GNSS_REQUEST_STOP_ONE_SHOT           4       ///< Stop one shot
#define XBEE_GNSS_REQUEST_START_RAW_NMEA          5       ///< Start RAW NMEA 0183 output
#define XBEE_GNSS_REQUEST_STOP_RAW_NMEA           6       ///< Stop RAW NMEA 0183 output
    uint16_t    timeout_be;     ///< timeout in seconds for one shot only, a value of 0 is return cached value
}) xbee_frame_gnss_start_stop_t;



/// Format of XBee API frame type 0xBD (#XBEE_FRAME_GNSS_START_STOP_RESP).
/// Sent from XBee to host
typedef XBEE_PACKED(xbee_frame_gnss_start_stop_resp_t, {
    uint8_t     frame_type;     ///< XBEE_FRAME_GNSS_START_STOP_RESP (0xBD)
    uint8_t     frame_id;       ///< identifier to match response frame
    uint8_t     request;        ///< matches type of the request frame
    uint8_t     status;         ///<
#define XBEE_GNSS_START_STOP_STATUS_SUCCESS     0
#define XBEE_GNSS_START_STOP_STATUS_UNSUCCESSFUL 1
}) xbee_frame_gnss_start_stop_resp_t;


/// Format of XBee API frame type 0xBE (#XBEE_FRAME_GNSS_RAW_NMEA_RESP).
/// Sent from XBee to host
typedef XBEE_PACKED(xbee_frame_gnss_raw_nmea_resp_t, {
    uint8_t     frame_type;     ///< XBEE_FRAME_GNSS_RAW_NMEA_RESP (0xBE)
    uint8_t     frame_id;       ///< identifier to match response frame
    uint8_t     payload[1];     ///< variable-length payload
}) xbee_frame_gnss_raw_nmea_resp_t;



/// Format of XBee API frame type 0xBF (#XBEE_FRAME_GNSS_ONE_SHOT_RESP).
/// Sent from XBee to host
typedef XBEE_PACKED(xbee_frame_gnss_one_shot_resp_t, {
    uint8_t     frame_type;     ///< XBEE_FRAME_GNSS_ONE_SHOT_RESP (0xBF)
    uint8_t     status;         ///<
#define XBEE_GNSS_ONE_SHOT_STATUS_VALID       0
#define XBEE_GNSS_ONE_SHOT_STATUS_INVALID     1
#define XBEE_GNSS_ONE_SHOT_STATUS_TIMEOUT     2
#define XBEE_GNSS_ONE_SHOT_STATUS_CANCELED    3
    uint32_t    lock_time_be;   ///< Lock time measured in seconds, from midnight, Jan-1-2000
    uint32_t    latitude_be;    ///< Latitude in decimal degrees * 1e7 (+/-9000000000 for +/-90.0). Positive/negative values are north/south of the equator.
    uint32_t    longitude_be;   ///< Longitude in decimal degrees * 1e7 (+/-1800000000 for +/-180.0). Positive/negative values are east/west of the prime meridian.
    uint32_t    altitude_be;    ///< Altitude in millimeters
    uint8_t     satellites;     ///< Total number of satellites in use.
}) xbee_frame_gnss_one_shot_resp_t;


// XBEE_GNSS_START_STOP_OPT_xxx values.
#define XBEE_GNSS_START_STOP_OPT_NONE       0x0000
#define XBEE_GNSS_START_STOP_OPT_NO_RESP    0x0001   // Don't send a response frame


/**
   @brief Start a GNSS one shot request.

   @param[in]  xbee     XBee device sending the message
   @param[in]  timeout  The timeout in seconds to wait for GNSS location data.  A value of 0 is return cached value.
   @param[in]  options  Bitmask of frame options.
                  XBEE_GNSS_START_STOP_OPT_NO_RESP - don't send a response frame

   @retval  0           Message sent with XBEE_GNSS_START_STOP_OPT_NO_RESP.
   @retval  >0          Frame ID of message, to correlate with the response
                        frame received from XBee module.
   @retval  -EINVAL     Invalid parameter passed to function.
*/
int xbee_gnss_start_one_shot(xbee_dev_t *xbee, uint16_t timeout, uint16_t options);


/**
   @brief Stop a GNSS one shot request.

   @param[in]  xbee     XBee device sending the message
   @param[in]  options  Bitmask of frame options.
                  XBEE_GNSS_START_STOP_OPT_NO_RESP - don't send a response frame

   @retval  0           Message sent with XBEE_GNSS_START_STOP_OPT_NO_RESP.
   @retval  >0          Frame ID of message, to correlate with the response
                        frame received from XBee module.
   @retval  -EINVAL     Invalid parameter passed to function.
*/
int xbee_gnss_stop_one_shot(xbee_dev_t *xbee, uint16_t options);

/**
   @brief Start a GNSS NMEA 0183 sentence output stream.

   @param[in]  xbee     XBee device sending the message
   @param[in]  options  Bitmask of frame options.
                  XBEE_GNSS_START_STOP_OPT_NO_RESP - don't send a response frame

   @retval  0           Message sent with XBEE_GNSS_START_STOP_OPT_NO_RESP.
   @retval  >0          Frame ID of message, to correlate with the response
                        frame received from XBee module.
   @retval  -EINVAL     Invalid parameter passed to function.
*/
int xbee_gnss_start_raw_nmea(xbee_dev_t *xbee, uint16_t options);

/**
   @brief Stop a GNSS NMEA 0183 sentence output stream.

   @param[in]  xbee     XBee device sending the message
   @param[in]  options  Bitmask of frame options.
                XBEE_GNSS_START_STOP_OPT_NO_RESP - don't send a response frame

   @retval  0           Message sent with XBEE_GNSS_START_STOP_OPT_NO_RESP.
   @retval  >0          Frame ID of message, to correlate with the response
                        frame received from XBee module.
   @retval  -EINVAL     Invalid parameter passed to function.
*/
int xbee_gnss_stop_raw_nmea(xbee_dev_t *xbee, uint16_t options);


XBEE_END_DECLS

#endif // XBEE_GNSS_H