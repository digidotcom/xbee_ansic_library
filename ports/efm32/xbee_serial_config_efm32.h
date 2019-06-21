/*
 * Copyright (c) 2017 Digi International Inc.,
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
	@addtogroup hal_efm32
	@{
	@file xbee_platform_efm32.c
	Serial configurations
*/
#ifndef __SERIAL_CONFIG_H
#define __SERIAL_CONFIG_H

	#define RX_BUFF_SIZE		255				/* Receive buffer = 2^n -1 for some integer n) */
	#define TX_BUFF_SIZE		RX_BUFF_SIZE	/* Transmit buffer (same as Rx buffer) */
	#define USART_NUMBER		0				/* The USART peripheral number you are using */


	/* USART Pin Locations */
	#define XBEE_TXPORT			gpioPortE		/* USART transmission port */
	#define XBEE_TXPIN			10				/* USART transmission pin */
	#define XBEE_RXPORT			gpioPortE		/* USART reception port */
	#define XBEE_RXPIN			11				/* USART reception pin */
	#define XBEE_RTSPORT		gpioPortE		/* USART RTS flow control port */
	#define XBEE_RTSPIN			13				/* USART RTS flow control pin */
	#define XBEE_CTSPORT		gpioPortB		/* USART CTS flow control port */
	#define XBEE_CTSPIN			11				/* USART CTS flow control pin */

	/* USART Route Locations (use datasheet for mapping locations) */
	#define TX_LOC				0
	#define RX_LOC				0
	#define CTS_LOC				5

#endif /* __SERIAL_CONFIG_H */
