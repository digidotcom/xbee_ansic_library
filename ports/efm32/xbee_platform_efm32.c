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
	Platform-specific functions for use by the
	XBee Driver on EFM32 uC platform.
	Documented in platform.h
*/
#include "xbee/platform_efm32.h"

#ifdef EFM32_MICRIUM

	uint32_t xbee_millisecond_timer()
	{
		RTOS_ERR err;
		uint32_t millis = OSTimeGet(&err);
		APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE, ;);
		return  millis;
	}

#ifndef  RTOS_MODULE_COMMON_CLK_AVAIL // if the second clock is available
	#include  <rtos/common/include/clk.h>

	void xbee_platform_init(void)
	{
		RTOS_ERR err;
		Clk_Init(&err);
		/* Checks if the clock started or if the clock was already started */
		APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE ||
					 err.Code == RTOS_ERR_ALREADY_EXISTS , ;);
	}

	uint32_t xbee_seconds_timer()
	{
		return Clk_GetTS();
	}

#else  // If the CLK module isn't available, just use milliseconds
	void xbee_platform_init(void)
	{
		return; //nothing to initialize here
	}
	uint32_t xbee_seconds_timer()
	{
		static uint32_t last_ms = 0;
		static uint32_t current_s = 0;
		uint32_t delta_s = (xbee_millisecond_timer() - last_ms) / 1000;
		last_ms += delta_s * 1000;
		return current_s += delta_s;
	}
#endif
#else
	
/* Counts 1ms timeTicks */
static volatile uint32_t ms_ticks;
static volatile uint32_t s_ticks;
static bool_t initialized;

/**************************************************************************//**
 * @brief SysTick_Handler
 * Interrupt Service Routine for system tick counters
 *****************************************************************************/
void SysTick_Handler(void)
{
	static uint32_t s_tick_check = 0; // used to detect second increment
	/* increment counters */
	ms_ticks++;
	if (++s_tick_check == 1000) {
		s_tick_check = 0;
		++s_ticks;      
	}
}

void xbee_platform_init(void)
{
	if (!initialized) {
		/* Setup SysTick Timer for 1 msec interrupts  */
		if (SysTick_Config(CMU_ClockFreqGet(cmuClock_CORE) / 1000)) {
			while (1) {} // Block when error
		}
		ms_ticks = 0;
		s_ticks = 0;
		initialized = 1;
	}
}

uint32_t xbee_seconds_timer()
{
	if (!initialized) {
		xbee_platform_init();
	}
	return s_ticks;
}

uint32_t xbee_millisecond_timer()
{
	if (!initialized) {
		xbee_platform_init();
	}
	return ms_ticks;
}

#endif
