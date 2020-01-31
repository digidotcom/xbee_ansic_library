/*
 * Copyright (c) 2010-2019 Digi International Inc.,
 * All rights not expressly granted are reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Digi International Inc., 9350 Excelsior Blvd., Suite 700, Hopkins, MN 55343
 * =======================================================================
 */
/**
    @defgroup hal_digiapix HAL: libdigiapix (Digi ConnectCore)
    Support for Digi International products using libdigiapix to control
    GPIO pins.

    @ingroup hal
    @{
    @file xbee_platform_digiapix.c
    Based on POSIX platform, with additional code to interface with
    /RESET, SLEEP_REQ, ON/nSLEEP and XBEE_IDENT pins.

    Initial version supports the XBee socket on the ConnectCore 6UL SBC Pro.
    For other products, update gpio_config[] with the correct kernel_num
    value for each pin.
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

#include <libdigiapix/gpio.h>

#include "xbee/platform.h"

uint32_t xbee_seconds_timer()
{
    // On BSD OSes, time can include leap seconds.  That's OK, because this
    // function is only used to track elapsed time, not determine time-of-day.
    return time(NULL);
}


uint32_t xbee_millisecond_timer()
{
    struct timeval t;

    gettimeofday(&t, NULL);
    return (uint32_t)(t.tv_sec * 1000 + t.tv_usec / 1000);
}


typedef struct {
    const char      *name;
    int             xbee_io_num;        // XBee DIO number
    unsigned int    kernel_num;         // digiapix kernel number
    gpio_mode_t     starting_mode;      // how to configure pin at startup
} gpio_xbee_t;

enum {
    IO_XBEE_RESET,
    IO_XBEE_ON_NSLEEP,
#if ENABLE_EXTRA_IO
    IO_XBEE_SLEEP_REQ,
    IO_XBEE_IDENT,
#endif
};

#define CC6UL_IO(x)     (465U + (x))

const gpio_xbee_t gpio_config[] = {
    { "RESET",      -1, CC6UL_IO(7),    GPIO_OUTPUT_HIGH },
    { "ON_nSLEEP",  9,  CC6UL_IO(11),   GPIO_INPUT },
#if ENABLE_EXTRA_IO
    { "SLEEP_REQ",  8,  CC6UL_IO(9),    GPIO_OUTPUT_LOW },
    { "IDENT",      0,  CC6UL_IO(33),   GPIO_OUTPUT_HIGH }, // double-check def.
#endif
};
gpio_t *gpio[_TABLE_ENTRIES(gpio_config)];      // gpio_t pointer for pin

static bool_t initialized = FALSE;

int digiapix_platform_init(void)
{
    if (! initialized) {
        for (size_t i = 0; i < _TABLE_ENTRIES(gpio_config); ++i) {
            const gpio_xbee_t *xb_io = &gpio_config[i];
            gpio[i] = ldx_gpio_request(xb_io->kernel_num,
                    xb_io->starting_mode, REQUEST_SHARED);
            if (gpio[i] == NULL) {
                printf("%s: request of %s failed\n", __FUNCTION__, xb_io->name);
                return -EIO;
            }
        }
        initialized = TRUE;
    }

    return 0;
}

/** @sa xbee_reset_fn
 *
 */
void digiapix_xbee_reset(struct xbee_dev_t *xbee, bool_t asserted)
{
    int err = ldx_gpio_set_value(gpio[IO_XBEE_RESET],
            asserted ? GPIO_LOW : GPIO_HIGH);
    if (err == EXIT_FAILURE) {
        printf("%s: error setting reset\n", __FUNCTION__);
    }
}

/** @sa xbee_is_awake_fn
 *
 */
int digiapix_xbee_is_awake(struct xbee_dev_t *xbee)
{
    int result = ldx_gpio_get_value(gpio[IO_XBEE_ON_NSLEEP]);
    if (result == GPIO_VALUE_ERROR) {
        printf("%s: error getting value\n", __FUNCTION__);
        return -EIO;        // treated as "awake" per function API
    }

    return result == GPIO_HIGH;
}

///@}
