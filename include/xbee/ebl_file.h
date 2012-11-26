/*
 * Copyright (c) 2012 Digi International Inc.,
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
	@file xbee/ebl_file.h

	Code for decoding headers from Ember firmware images (.EBL files).
*/

#ifndef __XBEE_EBL_FILE_H
#define __XBEE_EBL_FILE_H

#include "xbee/platform.h"

#define IMAGE_INFO_MAXLEN 32

#define EBLTAG_HEADER				0x0000

/// signature_be field for EM250 firmware images
#define IMAGE_SIGNATURE_EM250		0xE250
/// signature_be field for EM260 firmware images
#define IMAGE_SIGNATURE_EM260		0xE260
/// signature_be field for EM351/EM357 firmware images
#define IMAGE_SIGNATURE_EM350		0xE350

/// Header for EM3XX-based firmware images
typedef struct em3xx_header_t {
	uint16_t	ebltag_be;			///< Set to EBLTAG_HEADER (0x0000)
	uint16_t	length_be;			///< Set to 140
	uint16_t	version_be;			///< Version of the ebl format
	uint16_t	signature_be;		///< Set to IMAGE_SIGNATURE_EM350 (0xE350)
	uint8_t	reserved0[32];
	uint8_t	plat_info;			///< type of platform
	uint8_t	micro_info;			///< type of micro
	uint8_t	phy_info;			///< type of phy
	uint8_t	reserved1;
	uint8_t	znet_build;			///< EmberZNet build
	uint8_t	znet_release;		///< EmberZNet release (0x46 = 4.6.x)
	uint8_t	reserved2[2];
	uint32_t	timestamp_le;		///< .ebl file build time; seconds since 1/1/1970
	uint8_t	image_info[32];	///< string
	uint8_t	reserved3[60];
} em3xx_header_t;

/// Header for EM2XX-based firmware images
typedef struct em2xx_header_t {
	uint16_t	ebltag_be;			///< Set to EBLTAG_HEADER (0x0000)
	uint16_t	length_be;			///< Set to 60
	uint8_t	reserved0[2];
	/// Set to IMAGE_SIGNATURE_EM250 (0xE250) or IMAGE_SIGNATURE_EM260 (0xE260)
	uint16_t	signature_be;
	uint32_t	timestamp_be;		///< .ebl file build time; seconds since 1/1/1970
	uint8_t	reserved1[16];
	uint8_t	image_info[32];	///< PLAT-MICRO-PHY-BOARD string
	uint8_t	reserved2[4];
} em2xx_header_t;

/// Union of header formats for all Ember firmware images.
typedef union ebl_file_header_t {
	struct {
		uint16_t		ebltag_be;		///< Set to EBLTAG_HEADER (0x0000)
		uint16_t		length_be;		///< Header length
		uint8_t		reserved[2];
		uint16_t		signature_be;	///< One of the IMAGE_SIGNATURE_* macros
	} common;							///< Common fields of all .EBL files
	em2xx_header_t	em2xx;
	em3xx_header_t	em3xx;
} ebl_file_header_t;

/// Structure used to hold table of micro_info values from EM3XX header.
typedef struct micro_info_t {
	uint8_t		type;
	const char	*description;
} micro_info_t;
#define MICRO_INFO_END		{ 0xFF, NULL }

/// Structure used to hold table of plat_info values from EM3XX header.
typedef struct platform_info_t {
	uint8_t					type;
	const micro_info_t	*micro_info_table;
} platform_info_t;
#define PLATFORM_INFO_END	{ 0xFF, NULL }

/// Table of tables used to decode plat_info and micro_info fields from .EBL
/// file header.  See ebl_target_desc()
extern const platform_info_t platform_info[];

/** @brief
	Returns a description of the platform based on the plat_info and micro_info
	fields in the em3xx_header_t structure.

	@param[in]	plat_info	plat_info field from EM3xx file header
	@param[in]	micro_info	micro_info field from EM3xx file header

	@return	String describing platform or, for unknown combinations, "P:XX M:YY"
				where XX is plat_info in hex and YY is micro_info in hex.
*/
const char *ebl_target_desc( uint8_t plat_info, uint8_t micro_info);

/** @brief
	Returns a description of the PHY based on the phy_info field in the
	em3xx_header_t structure.

	@param[in]	phy_info		phy_info field from EM3xx file header

	@return	String describing PHY or, for uknown values, "PHY XX" where XX is
				phy_info in hex.
*/
const char *ebl_phy_desc( uint8_t phy_info);

/** @brief
	Prints a description of the .EBL file header.

	@param[in]	buffer	buffer with first 128 bytes of .EBL file
	@param[in]	flags		combination of the following macros:
			- #EBL_HEADER_DUMP_TARGET_DESC - target description (EM3XX only)
			- #EBL_HEADER_DUMP_PHY_DESC - PHY description (EM3XX only)
			- #EBL_HEADER_DUMP_EMBER_VER - Ember stack version (EM3XX only)
			- #EBL_HEADER_DUMP_TIMESTAMP - build date/timestamp
			- #EBL_HEADER_DUMP_IMAGE_INFO - "image info" string
			- #EBL_HEADER_DUMP_EVERYTHING - information on all possible fields

	@retval	-EINVAL	invalid parameter passed to function
	@retval	-EILSEQ	not a valid EBL file header
	@retval	0			parsed header and printed info
*/
int ebl_header_dump( const void *buffer, uint16_t flags);
#define EBL_HEADER_DUMP_TARGET_DESC		(1<<0)
#define EBL_HEADER_DUMP_PHY_DESC			(1<<1)
#define EBL_HEADER_DUMP_EMBER_VER		(1<<2)
#define EBL_HEADER_DUMP_TIMESTAMP		(1<<3)
#define EBL_HEADER_DUMP_IMAGE_INFO		(1<<4)
#define EBL_HEADER_DUMP_EVERYTHING		0xFFFF

// If compiling in Dynamic C, automatically #use the appropriate C file.
#ifdef __DC__
	#use "xbee_ebl_file.c"
#endif

#endif		// __XBEE_EBL_FILE_H
