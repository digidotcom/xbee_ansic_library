/*
	endian.h

	Copyright (c) 2010 Digi International Inc., All Rights Reserved

	This software contains proprietary and confidential information of Digi
	International Inc.  By accepting transfer of this copy, Recipient agrees
	to retain this software in confidence, to prevent disclosure to others,
	and to make no use of this software other than that for which it was
	delivered.  This is a published copyrighted work of Digi International
	Inc.  Except as permitted by federal law, 17 USC 117, copying is strictly
	prohibited.

	Restricted Rights Legend

	Use, duplication, or disclosure by the Government is subject to
	restrictions set forth in sub-paragraph (c)(1)(ii) of The Rights in
	Technical Data and Computer Software clause at DFARS 252.227-7031 or
	subparagraphs (c)(1) and (2) of the Commercial Computer Software -
	Restricted Rights at 48 CFR 52.227-19, as applicable.

	Digi International Inc. 11001 Bren Road East, Minnetonka, MN 55343
*/

/*
	Freescale processor is little endian.  These macros can be used in portable
	code to convert from host-byte-order to little-endian or big-endian byte
	order.
*/

#ifndef __ENDIAN_H
#define __ENDIAN_H

	#define LITTLE_ENDIAN	1234
	#define BIG_ENDIAN		4321
	#define PDP_ENDIAN		3412

	#define BYTE_ORDER		BIG_ENDIAN

#endif
