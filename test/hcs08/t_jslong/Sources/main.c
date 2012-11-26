// $Id$
/*-------------------------------------------------------
* Copyright (C) 2010 Digi International, All Rights Reserved.
*
*
* This software is provided as instructional material without charge
* by Digi International for use by its employees and customers
* subject to the following terms.
*
* PERMISSION
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software, to deal with it without restriction,
* including without limitation the rights to use, copy,  modify, merge, publish,
* distribute, sublicense, and/or sell copies of it, and to permit persons to
* whom it is furnished to do so, provided the above copyright notice
* and this permission notice are included in all derived works
* and the use of this software is restricted to Digi products.
*
* WARRANTY
* THIS SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE, OR NONINFRINGEMENT.
*
* LIABILITY
* IN NO EVENT SHALL DIGI INTERNATIONAL BE LIABLE FOR ANY CLAIM, DAMAGES,
* OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT, OR OTHERWISE,
* ARISING FROM, OUT OF, OR IN CONNECTION WITH THE SOFTWARE, OR THE USE
* OR OTHER DEALINGS WITH THE SOFTWARE.
*
*-------------------------------------------------------*/

#include <hidef.h>      /*!< for EnableInterrupts macro */
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

#include "derivative.h" /*!< include peripheral declarations */
#include "Serial.h"
#include "ExampleFuncs.h" // For GPIO init
#include "build_defines.h"
#include "pin_mapping.h"
#include "system.h"
#include "sharedRAM.h"

static const uint8 AppVersion[] = "t_jslong unit test";

//these are in a fixed location for the bootloader and Application to reference
static const unsigned long pAppVersion @0x0000F1BC = (unsigned long)AppVersion;

//rrm local funcs
void  InitClocks(void);
int t_all( void);

/****************************************************************
*
*   NAME:  main()
*
* \brief:  Main Entry into program
*
* \note:   // Characters from Host can be dropped since it is Polled
*
*****************************************************************/
void main(void)
{
	WATCHDOG_RESET();

	InitClocks();//change uP speed to 39.85Mhz
	InitRTC();
	InitGpio();
	InitUARTs();
	DisableHostUARTIsr();
	DisableRadioUARTIsr();
	EnableInterrupts;

	printf( "\n\nApp Version: %s\n", AppVersion);

  	t_all();

	for (;;)
	{
		WATCHDOG_RESET();

	}
}

/*************************************************************************************
*
*   NAME: InitClocks(void)
*
*   \brief:  Configures MC9S08 Internal Clocks
*
*   \details: Reads factory calibrated values stored in NVTRIM and NVFTRIM to ensure
*             accurate 32kHz Reference Clock.  Otherwise Clock can vary as much as 30%
*             Produces a 38MHz DCO output which in turn provides ~19MHz Bus Clock.
*
**************************************************************************************/
void InitClocks(void){

//-----------------------------------------
// Internal clock is calibrated to 31.25kHz NVICSTRM and NVFTRIM
// Internal clock can drift around 1% with temperature
//

  ICSTRM = NVICSTRM;//set internal clock to default 31.25kHz

/*****  DRS / DMX32 Table for FLL
*
*   CPU_Clock should not exceed  60 MHz if VCC > 2.4v
*   CPU_Clock should not exceed  40 MHz if VCC > 2.4v and VCC < 2.1v
*
*  DRS  |  DMX32   | REF Multipy |  Range
*---------------------------------------
*   0   |    0     |    512      |  Low
*   0   |    1     |    608      |  Low
*   1   |    0     |   1024      |  Mid
*   1   |    1     |   1216      |  Mid
*   2   |    0     |   1536      |  High
*   2   |    1     |   1824      |  High
*   3   |    x     |  Reserved   |  Reserved
*
*/
/******** Bus Clock *******
*
*  Bus Clock = (CPU_Clock / 2) / (2^ICSC2_BDIV)
*  Bus Clock should not exceed 20 MHz
*/

//System Clock
  ICSSC = MULTIPLY_BY_1216 | NVFTRIM ;//Set FLL for Mid range value 31.25kHz*1216=38MHz
  ICSC1 = CLOCK_SELECT_FLL | FLL_REF_SELECT_INTERNAL | ENABLE_ICSIRCLK | DISABLE_IREF_DURING_STOP;
//Bus clock
  ICSC2_BDIV = DIVIDE_BY_2;//Clock divide by 2 for bus clock = 38MHz/2 = 19MHz

}



