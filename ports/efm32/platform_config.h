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
	Platform header for efm32
*/
#ifndef __XBEE_PLATFORM_EFM32
#define __XBEE_PLATFORM_EFM32

	// The necessary includes for Micrium OS
#ifdef EFM32_MICRIUM
	#include <rtos/cpu/include/cpu.h>
	#include <rtos/common/include/common.h>
	#include <rtos/kernel/include/os.h>

	#include <rtos/common/include/rtos_err.h>
	#include <rtos/common/include/rtos_utils.h>
#endif 

	// The necessary includes for the efm32 board
	#include "em_device.h"
	#include "em_usart.h"
	#include "em_cmu.h"
	#include "em_core.h"
	#include "em_gpio.h"

	// stdint.h for int8_t, uint8_t, int16_t, etc. types
	#include <stdint.h>
	#include <errno.h>
	#include <inttypes.h>

	#define _f_memcpy memcpy
	#define _f_memset memset

	// Must define since GDK doesn't have endian.h (the values must be unique)
	#define LITTLE_ENDIAN 4321 
	#define BIG_ENDIAN 1234    
	#define BYTE_ORDER LITTLE_ENDIAN //This is hard-coded, but can be checked

	#define PACKED_STRUCT 	struct __attribute__((packed))

	typedef uint8_t bool_t; // could instead use <stdbool.h>
	
	typedef struct xbee_serial_t {
		uint32_t		baudrate;
	} xbee_serial_t;

	#define XBEE_MS_TIMER_RESOLUTION 1 // Our timer has 1 ms resolution
	#define ZCL_TIME_EPOCH_DELTA 0

#if defined(__cplusplus)
	extern "C" {
#endif
	void xbee_platform_init(void);
	#define XBEE_PLATFORM_INIT() xbee_platform_init()
#if defined(__cplusplus)
	}
#endif
#endif /* __XBEE_PLATFORM_EFM32 */
