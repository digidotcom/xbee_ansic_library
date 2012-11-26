/*
	xbib.h

	Macros for accessing switches and LEDs on XBIB-U-DEV board.
*/

#define XBIB_SW1				PTAD_PTAD0
#define XBIB_SW2				PTAD_PTAD3
#define XBIB_SW3				PTBD_PTBD2
#define XBIB_SW4				PTBD_PTBD5

#define XBIB_SW_OPEN			1
#define XBIB_SW_CLOSED		0

#define XBIB_SW1_CLOSED		(XBIB_SW1 == XBIB_SW_CLOSED)
#define XBIB_SW1_OPEN		(XBIB_SW1 == XBIB_SW_OPEN)
#define XBIB_SW2_CLOSED		(XBIB_SW2 == XBIB_SW_CLOSED)
#define XBIB_SW2_OPEN		(XBIB_SW2 == XBIB_SW_OPEN)
#define XBIB_SW3_CLOSED		(XBIB_SW3 == XBIB_SW_CLOSED)
#define XBIB_SW3_OPEN		(XBIB_SW3 == XBIB_SW_OPEN)
#define XBIB_SW4_CLOSED		(XBIB_SW4 == XBIB_SW_CLOSED)
#define XBIB_SW4_OPEN		(XBIB_SW4 == XBIB_SW_OPEN)

#define XBIB_LED1				PTBD_PTBD4
#define XBIB_LED2				PTAD_PTAD2
#define XBIB_LED3				PTBD_PTBD3

#define XBIB_LED_ON			0
#define XBIB_LED_OFF			1

#define XBIB_LED1_ON			(XBIB_LED1 = XBIB_LED_ON)
#define XBIB_LED1_OFF		(XBIB_LED1 = XBIB_LED_OFF)
#define XBIB_LED2_ON			(XBIB_LED2 = XBIB_LED_ON)
#define XBIB_LED2_OFF		(XBIB_LED2 = XBIB_LED_OFF)
#define XBIB_LED3_ON			(XBIB_LED3 = XBIB_LED_ON)
#define XBIB_LED3_OFF		(XBIB_LED3 = XBIB_LED_OFF)

/*
// LED4 used by XBee for ASSOC/DIO5, don't use
#define XBIB_LED4_ON			(PTDD_PTDD4 = 0)
#define XBIB_LED4_OFF		(PTDD_PTDD4 = 1)
*/

// Perhaps an xbib_init() function should be defined to
// configure the I/O lines correctly as input or output
