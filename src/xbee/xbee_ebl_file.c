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
	@file xbee/xbee_ebl_file.c

	Code for decoding headers from Ember firmware images (.EBL files).

	Documented in xbee/ebl_file.h.
*/

/*** BeginHeader */
#include <stdio.h>
#include <time.h>

#include "xbee/byteorder.h"		// for be16toh, etc.
#include "xbee/ebl_file.h"
/*** EndHeader */

/*** BeginHeader platform_info */
/*** EndHeader */
const micro_info_t platform_avr[] = {
	{ 1, "AVR ATMega 64" },
	{ 2, "AVR ATMega 128" },
	{ 3, "AVR ATMega 32" },
	MICRO_INFO_END
};

const micro_info_t platform_xap2b[] = {
	{ 1, "EM250" },
	{ 2, "EM260" },
	MICRO_INFO_END
};

const micro_info_t platform_cortex_m3[] = {
	{ 1, "EM350" },
	{ 2, "EM360" },
	{ 3, "EM357" },
	{ 4, "EM367" },
	{ 5, "EM351" },
	{ 6, "EM35x" },
	{ 7, "STM32W108" },
	MICRO_INFO_END
};

const platform_info_t platform_info[] = {
	{ 1, platform_avr },
	{ 2, platform_xap2b },
	{ 4, platform_cortex_m3 },
	PLATFORM_INFO_END
};

/*** BeginHeader ebl_target_desc */
/*** EndHeader */
const char *ebl_target_desc( uint8_t plat_info, uint8_t micro_info)
{
	static char unknown[10];	// "P:xx M:xx"
	const platform_info_t *plat;
	const micro_info_t *micro;
	
	for (plat = platform_info; plat->micro_info_table != NULL; ++plat)
	{
		if (plat->type == plat_info)
		{
			for (micro = plat->micro_info_table; micro->description != NULL; ++micro)
			{
				if (micro->type == micro_info)
				{
					return micro->description;
				}
			}
		}
	}
	
	sprintf( unknown, "P:%02X M:%02X", plat_info, micro_info);
	return unknown;
}

/*** BeginHeader ebl_phy_desc */
/*** EndHeader */
const char *ebl_phy_desc( uint8_t phy_info)
{
	static char unknown[7];	// "PHY xx"

	switch (phy_info)
	{
		case 0: return "NULL";
		case 1: return "EM2420";
		case 2: return "EM250";
		case 3: return "EM3xx";
	}
	
	sprintf( unknown, "PHY %02X", phy_info);
	return unknown;
}

/*** BeginHeader ebl_header_dump */
/*** EndHeader */
void ebl_header_dump_em2xx( const em2xx_header_t *ebl, uint16_t flags)
{
	time_t timestamp;

	if (ebl == NULL)
	{
		return;
	}

	if (flags & EBL_HEADER_DUMP_IMAGE_INFO)
	{
		printf( "Image Info: %.32s\n", ebl->image_info);
	}
	if (flags & EBL_HEADER_DUMP_TIMESTAMP)
	{
		// get timestamp of build, based on Unix epoch (1/1/1970)
		timestamp = be32toh( ebl->timestamp_be);
		
		// convert to platform's epoch
		timestamp = timestamp + (ZCL_TIME_EPOCH_DELTA - ZCL_TIME_EPOCH_DELTA_1970);
		
		// convert to struct tm and then print
		printf( "Build Time (GMT): %s", asctime( gmtime( &timestamp)));
	}	
	
}

void ebl_header_dump_em3xx( const em3xx_header_t *ebl, uint16_t flags)
{
	time_t timestamp;
	
	if (ebl == NULL)
	{
		return;
	}

	if (flags & EBL_HEADER_DUMP_EMBER_VER)
	{
		printf( "Ember Stack: %X.%X build 0x%02X  ",
			ebl->znet_release >> 4, ebl->znet_release & 0x0F, ebl->znet_build);
	}
	if (flags & EBL_HEADER_DUMP_TARGET_DESC)
	{
		printf( "Target: %s  ", ebl_target_desc( ebl->plat_info,
				ebl->micro_info));
	}
	if (flags & EBL_HEADER_DUMP_PHY_DESC)
	{
		printf( "Phy: %s  ", ebl_phy_desc( ebl->phy_info));
	}
	if (flags & (EBL_HEADER_DUMP_EMBER_VER 
		| EBL_HEADER_DUMP_TARGET_DESC 
		| EBL_HEADER_DUMP_PHY_DESC))
	{
		puts( "");
	}

	if (flags & EBL_HEADER_DUMP_IMAGE_INFO)
	{
		printf( "Image Info: %.32s\n", ebl->image_info);
	}
	if (flags & EBL_HEADER_DUMP_TIMESTAMP)
	{
		// get timestamp of build, based on Unix epoch (1/1/1970)
		timestamp = le32toh( ebl->timestamp_le);
		
		// convert to platform's epoch
		// Subtract 1970 delta to get timestamp with 1/1/2000 epoch, then add 
		// platform's delta to get to the platform's epoch.
		timestamp = timestamp +
			(ZCL_TIME_EPOCH_DELTA - ZCL_TIME_EPOCH_DELTA_1970);
		
		// convert to struct tm and then print
		printf( "Build Time (GMT): %s", asctime( gmtime( &timestamp)));
	}	
}

int ebl_header_dump( const void *buffer, uint16_t flags)
{
	const ebl_file_header_t *ebl_file = buffer;
	uint16_t ebltag, signature;
	
	if (buffer == NULL)
	{
		return -EINVAL;
	}

	ebltag = be16toh( ebl_file->common.ebltag_be);
	signature = be16toh( ebl_file->common.signature_be);

	if (ebltag != EBLTAG_HEADER)
	{
		printf( "Not an EBL file (ebltag = 0x%04X, not 0x%04X)\n", 
			ebltag, EBLTAG_HEADER);

		return -EILSEQ;
	}

	switch (signature)
	{
		case IMAGE_SIGNATURE_EM250:
		case IMAGE_SIGNATURE_EM260:
			ebl_header_dump_em2xx( &ebl_file->em2xx, flags);
			break;
			
		case IMAGE_SIGNATURE_EM350:
			ebl_header_dump_em3xx( &ebl_file->em3xx, flags);
			break;
			
		default:
			printf( "don't know how to handle signature 0x%04X\n", signature);
			return -EILSEQ;
	}

	return 0;
}

