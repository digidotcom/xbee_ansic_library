/*
	rtc.c

	Copyright (c)2010 Digi International Inc., All Rights Reserved

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

/**
	@file rtc.c
	Source for Freescale HCS08 real-time counter routines.

	This code provides two counters, automatically updated by the
	Freescale RTC:

	-	const volatile unsigned long MS_TIMER     // milliseconds since RTC start
	-	const volatile unsigned long SEC_TIMER    // seconds since RTC start

  This code uses the internal clock running at 31.250kHz with 4ms resolution.

  @note	If interrupts are blocked for more than 4ms, the RTC interrupt
  			will miss a 4ms clock tick and the counters will be off by 4ms.
*/

#include "derivative.h"
#include "rtc.h"

/// Milliseconds since RTC start (4ms resolution)
volatile unsigned long _MS_TIMER = 0;
/// Seconds since RTC start
volatile unsigned long _SEC_TIMER = 0;

//////////////////////////////////////////////////////////////////////

#define RTCSC_RTCLKS_LPO_DIV0		0x00		// uses prescaler table 0
#define RTCSC_RTCLKS_ERCLK_DIV1	0x20		// uses prescaler table 1
#define RTCSC_RTCLKS_IRCLK_DIV0	0x40		// use prescaler table 0
#define RTCSC_RTCLKS_IRCLK_DIV1	0x60		// use prescaler table 1

// Prescaler Divide-by values (RTCLKS[0] == 0)
#define RTCSC_RTCLKS_DIV0_OFF		0x00
#define RTCSC_RTCLKS_DIV0_8		0x01
#define RTCSC_RTCLKS_DIV0_32		0x02
#define RTCSC_RTCLKS_DIV0_64		0x03
#define RTCSC_RTCLKS_DIV0_128		0x04
#define RTCSC_RTCLKS_DIV0_256		0x05
#define RTCSC_RTCLKS_DIV0_512		0x06
#define RTCSC_RTCLKS_DIV0_1024	0x07
#define RTCSC_RTCLKS_DIV0_1		0x08
#define RTCSC_RTCLKS_DIV0_2		0x09
#define RTCSC_RTCLKS_DIV0_4		0x0A
#define RTCSC_RTCLKS_DIV0_10		0x0B
#define RTCSC_RTCLKS_DIV0_16		0x0C
#define RTCSC_RTCLKS_DIV0_100		0x0D
#define RTCSC_RTCLKS_DIV0_500		0x0E
#define RTCSC_RTCLKS_DIV0_1000	0x0F

// Prescaler Divide-by values (RTCLKS[0] == 1)
#define RTCSC_RTCLKS_DIV1_OFF		0x00
#define RTCSC_RTCLKS_DIV1_1024	0x01
#define RTCSC_RTCLKS_DIV1_2048	0x02
#define RTCSC_RTCLKS_DIV1_4096	0x03
#define RTCSC_RTCLKS_DIV1_8192	0x04
#define RTCSC_RTCLKS_DIV1_16384	0x05
#define RTCSC_RTCLKS_DIV1_32768	0x06
#define RTCSC_RTCLKS_DIV1_65536	0x07
#define RTCSC_RTCLKS_DIV1_1000	0x08
#define RTCSC_RTCLKS_DIV1_2000	0x09
#define RTCSC_RTCLKS_DIV1_5000	0x0A
#define RTCSC_RTCLKS_DIV1_10000	0x0B
#define RTCSC_RTCLKS_DIV1_20000	0x0C
#define RTCSC_RTCLKS_DIV1_50000	0x0D
#define RTCSC_RTCLKS_DIV1_100000	0x0E
#define RTCSC_RTCLKS_DIV1_200000	0x0F

void InitRTC(void)
{
#if 0
	// Use 1kHz clock to generate interrupt every 4ms.  This method is not very
	// accurate (>10% error), and should only be used when the system needs to
	// run at low power.

	RTCMOD = 0;									// generate interrupt on each count
	RTCSC = RTCSC_RTCLKS_LPO_DIV0			// 1kHz low power oscillator
  			| RTCSC_RTCLKS_DIV0_4			// divide by 4 for 4ms resolution
  			| RTCSC_RTIE_MASK;				// enable interrupt

#else
	// Use 31.250kHz clock (+/-1% accuracy) to generate interrupt every 4ms.
	ICSC1_IRCLKEN = 1;						//enable internal clock for timing
	RTCMOD = 124;		// 31.250kHz / 125 = 250Hz = 4ms resolution
	RTCSC = RTCSC_RTCLKS_IRCLK_DIV0		// internal (31.250kHz) clock
			| RTCSC_RTCLKS_DIV0_1			// divide by 1
			| RTCSC_RTIE_MASK;				// enable interrupt
#endif
}

//////////////////////////////////////////////////////////////////////

void wait_ms( unsigned short mSeconds)
{
	unsigned short t;

	t = SET_TIMEOUT_MS(mSeconds);
	while (! CHECK_TIMEOUT_MS(t))
	{
		__RESET_WATCHDOG();			//reset the watchdog timer
	}
}

#pragma TRAP_PROC
void vRTC(void)
{
	static unsigned char  sec_countdown = 250;

	_MS_TIMER += 4;
	if (--sec_countdown == 0) {
		++_SEC_TIMER;
		sec_countdown = 250;
	}

	RTCSC = RTCSC | 0x80;  // clear interrupt flag
}
