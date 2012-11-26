/*
	rtc.h

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
	@file rtc.h
	Header for Freescale HCS08 real-time counter routines.
*/

// Macros cast underlying globals as constants so compiler will warn if
// user code tries to modify them.
#define MS_TIMER ((const volatile unsigned long) _MS_TIMER)
#define SEC_TIMER ((const volatile unsigned long) _SEC_TIMER)

extern volatile unsigned long _MS_TIMER, _SEC_TIMER;

/**
	Macro used to load a variable with an expiration time, later used with
	CHECK_TIMEOUT_MS() macro.  For timeouts longer than 32767 milliseconds
	(with less accuracy), use SET_TIMEOUT_SEC()/CHECK_TIMEOUT_SEC().

	@note You must call InitRTC() at startup in order to enable the Real-Time
			Counter that increments the global variables tracking elapsed time.

	@param[in]	delay		Timeout in milliseconds, from 1 to 32767.

	@return Value to assign to an unsigned 16-bit variable (unsigned short,
				uint16_t) for use with CHECK_TIMEOUT_MS().

	@sa InitRTC(), CHECK_TIMEOUT_MS(), SET_TIMEOUT_SEC(), CHECK_TIMEOUT_SEC()

	@code
		unsigned short t;

		t = SET_TIMEOUT_MS(5000);		// 5 second timeout
		for (;;)
		{
			// do stuff or wait for something to happen

			if (CHECK_TIMEOUT_MS(t))
			{
				// 5 seconds elapsed
				break;
			}
		}
	@endcode
*/
#define SET_TIMEOUT_MS(delay) ((unsigned short)MS_TIMER + (delay))

/**
	Macro used to check a timer set by SET_TIMEOUT_MS().  See the documentation
	for SET_TIMEOUT_MS() for a code example on proper use of this macro.

	@param[in]	timer		16-bit variable set by SET_TIMEOUT_MS().

	@retval	TRUE		Time specified in call to SET_TIMEOUT_MS() has elapsed.
	@retval	FALSE		Time specified has not yet elapsed.

	@sa InitRTC(), SET_TIMEOUT_MS(), SET_TIMEOUT_SEC(), CHECK_TIMEOUT_SEC()
*/
#define CHECK_TIMEOUT_MS(timer) \
	((short)((unsigned short)MS_TIMER - (timer)) >= 0)


/**
	Macro used to load a variable with an expiration time, later used with
	CHECK_TIMEOUT_SEC() macro.  For a more precise timeout, for values of 30
	seconds or less, use SET_TIMEOUT_MS()/CHECK_TIMEOUT_MS() instead.

	@note You must call InitRTC() at startup in order to enable the Real-Time
			Counter that increments the global variables tracking elapsed time.

	@param[in]	delay		Timeout in seconds, from 1 to 32767.

	@return Value to assign to an unsigned 16-bit variable (unsigned short,
				uint16_t) for use with CHECK_TIMEOUT_SEC().

	@sa InitRTC(), CHECK_TIMEOUT_SEC(), SET_TIMEOUT_MS(), CHECK_TIMEOUT_MS()

	@code
		unsigned short t;

		t = SET_TIMEOUT_SEC(600);		// 10 minute timeout
		for (;;)
		{
			// do stuff or wait for something to happen

			if (CHECK_TIMEOUT_SEC(t))
			{
				// 10 minutes elapsed
				break;
			}
		}
	@endcode
*/
#define SET_TIMEOUT_SEC(delay) ((unsigned short)SEC_TIMER + (delay))

/**
	Macro used to check a timer set by SET_TIMEOUT_SEC().  See the documentation
	for SET_TIMEOUT_SEC() for a code example on proper use of this macro.

	@param[in]	timer		16-bit variable set by SET_TIMEOUT_SEC().

	@retval	TRUE		Time specified in call to SET_TIMEOUT_SEC() has elapsed.
	@retval	FALSE		Time specified has not yet elapsed.

	@sa InitRTC(), SET_TIMEOUT_SEC(), SET_TIMEOUT_MS(), CHECK_TIMEOUT_MS()
*/
#define CHECK_TIMEOUT_SEC(timer) \
	((short)((unsigned short)SEC_TIMER - (timer)) >= 0)

void InitRTC(void);
void wait_ms( unsigned short mSeconds);
void vRTC(void);
