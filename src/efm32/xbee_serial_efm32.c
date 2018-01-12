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
	@file xbee_serial_efm32.c
	Serial Interface for XBee Module (EFM32 Microcontroller)
*/
// NOTE: Documentation for the public functions can be found in xbee/serial.h.

#include "xbee/platform.h"
#include "xbee/serial.h"
#include "xbee_serial_config_efm32.h"
#include "xbee/cbuf.h"
#include <errno.h>

#define BUFFER_UPPER_BOUND	4  // how much space before rts is de-asserted
#define	BUFFER_LOWER_BOUND	RX_BUFF_SIZE/3  // how much before it's re-asserted


/* DO NOT CHANGE THINGS BELOW (they change based off of USART_NUMBER) */
#define XBEE_RX_IRQn		CONCAT3(USART, USART_NUMBER, _RX_IRQn)			/* IRQRX number */
#define XBEE_TX_IRQn		CONCAT3(USART, USART_NUMBER, _TX_IRQn)			/* IRQTX number */
#define XBEE_RX_IRQ_NAME	CONCAT3(USART, USART_NUMBER, _RX_IRQHandler)	/* USART IRQ Handler */
#define XBEE_TX_IRQ_NAME 	CONCAT3(USART, USART_NUMBER, _TX_IRQHandler)	/* USART IRQ Handler */
#define XBEE_USART			CONCAT(USART, USART_NUMBER)						/* USART instance */
#define XBEE_CLK			CONCAT3(cmuClock, _USART, USART_NUMBER)			/* HFPER Clock */
#define XBEE_ROUTE_LOC_TX	CONCAT(USART_ROUTELOC0_TXLOC_LOC,TX_LOC)		/* HFPER Clock */
#define XBEE_ROUTE_LOC_RX	CONCAT(USART_ROUTELOC0_RXLOC_LOC,TX_LOC)
#define XBEE_ROUTE_LOC_CTS	CONCAT(USART_ROUTELOC1_CTSLOC_LOC, CTS_LOC)

/* Helper Macros, DO NOT CHANGE */
#define XBEE_USART_STRING	TOSTRINGMACRO(CONCAT(USART_, USART_NUMBER))       /* String version of USART_# */
#define TOSTRINGMACRO(s)	TOSTRINGMACRO_(s)
#define TOSTRINGMACRO_(s)	#s
#define CONCAT_(A, B)		A ## B
#define CONCAT(A, B)		CONCAT_(A, B)
#define CONCAT3_(A, B, C)	A ## B ## C
#define CONCAT3(A, B, C)	CONCAT3_(A, B, C)

/* Local Prototypes */
void checkRxBufferUpper(void);
void checkRxBufferLower(void);
void routeSerial(void);
void serialInit(xbee_serial_t *serial);

USART_TypeDef *usart = XBEE_USART;
static xbee_cbuf_t *rx_buffer;
static xbee_cbuf_t *tx_buffer;
static uint8_t internal_rx_buffer[RX_BUFF_SIZE + XBEE_CBUF_OVERHEAD];
static uint8_t internal_tx_buffer[TX_BUFF_SIZE + XBEE_CBUF_OVERHEAD];

static bool_t		flow_control_enabled = TRUE;	/* is FC enabled?   */
static bool_t		tx_break = FALSE;					/* are we breaking  */

/******************************************************************************
 * IRQS
 *****************************************************************************/
void XBEE_RX_IRQ_NAME(void)
{
	uint32_t interruptFlags = USART_IntGetEnabled(XBEE_USART);
	USART_IntClear(XBEE_USART, interruptFlags);

	if (XBEE_USART->STATUS & USART_STATUS_RXDATAV) {
		// Store Data
		xbee_cbuf_putch(rx_buffer, USART_Rx(XBEE_USART));
		checkRxBufferUpper();
	}
}

void XBEE_TX_IRQ_NAME(void)
{
	uint32_t interruptFlags = USART_IntGetEnabled(XBEE_USART);
	USART_IntClear(XBEE_USART, interruptFlags);

	if (XBEE_USART->STATUS & USART_STATUS_TXBL) {
		int tx;
		if (tx_break) {
			USART_IntDisable(XBEE_USART, UART_IF_TXBL);
			return;
		}
		tx = xbee_cbuf_getch(tx_buffer);
		if (tx != -1) {
			USART_Tx(XBEE_USART, (uint8_t) tx);
		}
		else { //nothing to send
			USART_IntDisable(XBEE_USART, UART_IF_TXBL);
		}
	}
}


/******************************************************************************
 * LOCAL FUNCTIONS
 *****************************************************************************/
void checkRxBufferUpper(void)
{
	if ((xbee_cbuf_free(rx_buffer)) <= BUFFER_UPPER_BOUND) {
		/* The buffer is almost full, de-assert RTS */
		GPIO_PinOutSet(XBEE_RTSPORT, XBEE_RTSPIN);
	}
}


void checkRxBufferLower(void)
{
	if (xbee_cbuf_used(rx_buffer) <= BUFFER_LOWER_BOUND) {
		/* The buffer is empty enough, assert RTS */
		GPIO_PinOutClear(XBEE_RTSPORT, XBEE_RTSPIN);
	}
}

/******************************************************************************
 * @brief This function may need to be changed based on target board
 *****************************************************************************/
void routeSerial(void)
{
	usart->ROUTEPEN = USART_ROUTEPEN_RXPEN | USART_ROUTEPEN_CTSPEN;

	/* Enable pins at correct UART/USART location. */
	usart->ROUTELOC0 = ( usart->ROUTELOC0
							& ~( _USART_ROUTELOC0_TXLOC_MASK | _USART_ROUTELOC0_RXLOC_MASK ))
							 | ( XBEE_ROUTE_LOC_TX << _USART_ROUTELOC0_TXLOC_SHIFT )
							 | ( XBEE_ROUTE_LOC_RX << _USART_ROUTELOC0_RXLOC_SHIFT );

	usart->ROUTELOC1 = ( usart->ROUTELOC1 & ~( _USART_ROUTELOC1_CTSLOC_MASK)) |
								( XBEE_ROUTE_LOC_CTS << _USART_ROUTELOC1_CTSLOC_SHIFT);
}

/* It should be safe to call this multiple times */
void serialInit(xbee_serial_t *serial)
{
#ifdef EFM32_MICRIUM
	RTOS_ERR err;
	if (OSRunning == OS_STATE_OS_RUNNING) {
		OSSchedLock(&err);
		APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE, ;);
	}
	CPU_INT_SRC_HANDLER_SET_NKA(XBEE_RX_IRQn + 16, &(XBEE_RX_IRQ_NAME));
	CPU_INT_SRC_HANDLER_SET_NKA(XBEE_TX_IRQn + 16, &(XBEE_TX_IRQ_NAME));
#endif // Don't allow context switching until after initialization
	
	rx_buffer = (xbee_cbuf_t *) internal_rx_buffer;
	xbee_cbuf_init(rx_buffer, RX_BUFF_SIZE);

	tx_buffer = (xbee_cbuf_t *) internal_tx_buffer;
	xbee_cbuf_init(tx_buffer, TX_BUFF_SIZE);

	/* Enable peripheral clocks */
	CMU_ClockDivSet(cmuClock_HF, cmuClkDiv_1);
	CMU_ClockEnable(cmuClock_HFPER, TRUE);
	CMU_ClockEnable(cmuClock_GPIO, TRUE);
	CMU_ClockEnable(XBEE_CLK, TRUE);


	/* Configure GPIO pins */
	/* Config flow control pins */
	GPIO_PinModeSet(XBEE_RTSPORT, XBEE_RTSPIN, gpioModePushPull, 0);
	GPIO_PinOutClear(XBEE_RTSPORT,XBEE_RTSPIN);
	GPIO_PinModeSet(XBEE_CTSPORT, XBEE_CTSPIN, gpioModeInput, 0);

	/* Config Rx pin*/
	GPIO_PinModeSet(XBEE_RXPORT, XBEE_RXPIN, gpioModeInputPull, 1);

	USART_InitAsync_TypeDef init = USART_INITASYNC_DEFAULT;
	init.enable = usartDisable;

	USART_InitAsync(usart, &init);

	routeSerial();

	/* Clear previous RX and TX interrupts */
	USART_IntClear(XBEE_USART, USART_IF_RXDATAV);
	NVIC_ClearPendingIRQ(XBEE_RX_IRQn);
	NVIC_ClearPendingIRQ(XBEE_TX_IRQn);

	/* Enable interrupts */
	USART_IntEnable(XBEE_USART, USART_IF_RXDATAV);
	NVIC_EnableIRQ(XBEE_RX_IRQn);
	NVIC_EnableIRQ(XBEE_TX_IRQn);
	/* Finally enable it */
	USART_Enable(usart, usartEnable);

	xbee_ser_break(serial, FALSE);
	xbee_ser_flowcontrol(serial, TRUE);
	
	USART_IntSet(XBEE_USART, UART_IF_TXBL); //Trigger transfer check
	
#ifdef EFM32_MICRIUM
	if (OSRunning == OS_STATE_OS_RUNNING) {
		OSSchedUnlock(&err);
		APP_RTOS_ASSERT_CRITICAL(err.Code == RTOS_ERR_NONE, ;);
	}
#endif // allow context switching after initialization
}

/******************************************************************************
 * PUBLIC FUNCTIONS
 *****************************************************************************/
bool_t xbee_ser_invalid(xbee_serial_t *serial)
{
	return (serial == NULL || serial->baudrate == 0);
}

const char *xbee_ser_portname(xbee_serial_t *serial)
{
	return XBEE_USART_STRING;
}

int xbee_ser_write(xbee_serial_t *serial, const void FAR *buffer, int length)
{
	int ret = 0;
	if (xbee_ser_invalid(serial)) {
		return -EINVAL;
	}

	if (length < 0 || tx_break) {
		return -EIO;
	}
	
	if (length > TX_BUFF_SIZE) {
		length = TX_BUFF_SIZE;
	}
	ret = xbee_cbuf_put(tx_buffer, buffer, length);
	USART_IntEnable(XBEE_USART, UART_IF_TXBL);
	return ret;
}

int xbee_ser_read(xbee_serial_t *serial, void FAR *buffer, int bufsize)
{
	int ret;
	if (xbee_ser_invalid(serial)) {
		return -EINVAL;
	}

	if (bufsize < 0) {
		return -EIO;
	}
	
	if (bufsize > RX_BUFF_SIZE) {
		bufsize = RX_BUFF_SIZE;
	}
	ret = xbee_cbuf_get(rx_buffer, buffer, bufsize);
	if (flow_control_enabled) {
		checkRxBufferLower();
	}
	return ret;
}

int xbee_ser_putchar(xbee_serial_t *serial, uint8_t ch)
{
	if (xbee_ser_invalid(serial)) {
		return -EINVAL;
	}

	if (tx_break){
		return -EIO;
	}
	
	USART_IntEnable(XBEE_USART, UART_IF_TXBL);
	if (xbee_cbuf_putch(tx_buffer, ch)) {
		return 0;
	}
	return -ENOSPC;
}

int xbee_ser_getchar(xbee_serial_t *serial)
{
	if (xbee_ser_invalid(serial)) {
		return -EINVAL;
	}

	int ch = xbee_cbuf_getch(rx_buffer);
	
	if (ch == -1) {
		return -ENODATA;
	}
	
	if (flow_control_enabled) {
		checkRxBufferLower();			
	}
	return ch;
}

int xbee_ser_tx_free(xbee_serial_t *serial)
{
	if (xbee_ser_invalid(serial)) {
		return -EINVAL;
	}
	return xbee_cbuf_free(tx_buffer);
}

int xbee_ser_tx_used(xbee_serial_t *serial)
{
	if (xbee_ser_invalid(serial)) {
		return -EINVAL;
	}
	return xbee_cbuf_used(tx_buffer);
}

int xbee_ser_tx_flush(xbee_serial_t *serial)
{
	if (xbee_ser_invalid(serial)) {
		return -EINVAL;
	}
	xbee_cbuf_flush(tx_buffer);
	return 0;
}

int xbee_ser_rx_free(xbee_serial_t *serial)
{
	if (xbee_ser_invalid(serial)) {
		return -EINVAL;
	}
	return xbee_cbuf_free(rx_buffer);
}

int xbee_ser_rx_used(xbee_serial_t *serial)
{
	if (xbee_ser_invalid(serial)) {
		return -EINVAL;
	}
	return xbee_cbuf_used(rx_buffer);
}

int xbee_ser_rx_flush(xbee_serial_t *serial)
{
	if (xbee_ser_invalid(serial)) {
		return -EINVAL;
	}
	xbee_cbuf_flush(rx_buffer);
	return 0;
}

int xbee_ser_open(xbee_serial_t *serial, uint32_t baudrate)
{
	if (xbee_ser_invalid(serial)) {
		return -EINVAL;
	}
	serialInit(serial);
	return xbee_ser_baudrate(serial, baudrate);
}

int xbee_ser_baudrate(xbee_serial_t *serial, uint32_t baudrate)
{
	if (xbee_ser_invalid(serial)) {
		return -EINVAL;
	}
	switch (baudrate)
	{
		case 9600:
		case 19200:
		case 38400:
		case 57600:
		case 115200:
		case 230400:
		case 460800:
		case 921600:
			break;
		default:
			return -EIO;
	}

	serial->baudrate = baudrate;
	USART_BaudrateAsyncSet(XBEE_USART, 0, baudrate, usartOVS16);
	return 0;
}

int xbee_ser_close(xbee_serial_t *serial)
{
	if (xbee_ser_invalid(serial)) {
		return -EINVAL;
	}
	/* Disable interrupts */
	USART_IntDisable(XBEE_USART, USART_IF_RXDATAV);
	USART_IntDisable(XBEE_USART, UART_IF_TXBL);
	NVIC_DisableIRQ(XBEE_RX_IRQn);
	NVIC_DisableIRQ(XBEE_TX_IRQn);
	xbee_ser_rx_flush(serial);
	xbee_ser_tx_flush(serial);
	return 0;
}

int xbee_ser_break(xbee_serial_t *serial, bool_t enabled)
{
	if (xbee_ser_invalid(serial)) {
		return -EINVAL;
	}

	if (enabled) {
		tx_break = TRUE;
		USART_IntDisable(XBEE_USART, UART_IF_TXBL);
		GPIO_PinModeSet(XBEE_TXPORT, XBEE_TXPIN, gpioModePushPull, 0);
		usart->ROUTEPEN &= ~(_USART_ROUTEPEN_MASK & USART_ROUTEPEN_TXPEN);
		GPIO_PinOutClear(XBEE_TXPORT, XBEE_TXPIN);
	}
	else {
		GPIO_PinModeSet(XBEE_TXPORT, XBEE_TXPIN, gpioModePushPull, 1);
		GPIO_PinOutSet(XBEE_TXPORT, XBEE_TXPIN);
		usart->ROUTEPEN |= _USART_ROUTEPEN_MASK & USART_ROUTEPEN_TXPEN;
		tx_break = FALSE;
		USART_IntEnable(XBEE_USART, UART_IF_TXBL);
	}
	return 0;
}

int xbee_ser_flowcontrol(xbee_serial_t *serial, bool_t enabled)
{

	if (xbee_ser_invalid(serial)) {
		return -EINVAL;
	}

	flow_control_enabled = enabled; /* To disable software RTS */

	if (enabled) {
		/* Ensure we have RTS asserted correctly */
		checkRxBufferLower();
		checkRxBufferUpper();
		/* Enable CTS flow control */
		usart->CTRLX |= (_USART_CTRLX_MASK & (USART_CTRLX_CTSEN));
	}
	else {
		/* Disable CTS and RTS flow control */
		usart->CTRLX &= (_USART_CTRLX_MASK & (~USART_CTRLX_CTSEN));
		GPIO_PinOutClear(XBEE_RTSPORT, XBEE_RTSPIN);
	}
	return 0;
}

int xbee_ser_set_rts(xbee_serial_t *serial, bool_t asserted)
{
	int ret;
	
	ret = xbee_ser_flowcontrol(serial, FALSE);
	if (ret == 0) {
		
		if (asserted) {
			GPIO_PinOutClear(XBEE_RTSPORT, XBEE_RTSPIN);
		}
		else {
			GPIO_PinOutSet(XBEE_RTSPORT, XBEE_RTSPIN);
		}
	}
	return ret;
}

int xbee_ser_get_cts(xbee_serial_t *serial)
{
	if (xbee_ser_invalid(serial)) {
		return -EINVAL;
	}
	return (0 == GPIO_PinInGet(XBEE_CTSPORT, XBEE_CTSPIN));
}
