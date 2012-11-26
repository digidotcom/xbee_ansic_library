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
* REQUEST FOR IMPROVEMENTS
* If you have suggestions for improvements, or source code which corrects,
* enhances, or extends the functionality of this software, please forward
* them to the Tech Support group in the Lindon, Utah office
* of Digi International addressed to the attention of 'Linus'.
*-------------------------------------------------------*/

#include <stddef.h>
#include <string.h> // For strlen, may be replaced later for smaller code
#include <assert.h>
#include "derivative.h"
#include "System.h"     /*!< Common definitions for BL and APP */
#include "pin_mapping.h"
#include "common.h"
#include "build_defines.h"
//#include "bootloader.h"       /*!< Generic project-wide defines */
#include "serial.h"

/*********************************************************************************
*
*   NAME: InitHostUART(void)
*
*   \brief:  Initializes SCI1
*
*   \details: Enables Receive Interrupt and sets Baud to 9600. SCI1 Communicates
*             with Hyperterminal/Host PC.
***********************************************************************************/
void InitHostUART(void){

  /* configure SCI1/Host UART mode */
  SCI1C1 = LOOPBACK_OFF | LOOPBACK_TWO_WIRE | UART_OFF_DURING_WAIT | CHARACTER_8BIT | WAKEUP_IDLE_LINE |IDLE_COUNT_AFTER_START_BIT | PARITY_OFF | PARITY_EVEN;
  SCI1C3 = TX_DATA_NORMAL | RX_PARITY_ERROR_INTERRUPT_DISABLE | RX_FRAMING_ERROR_INTERRUPT_DISABLE|RX_NOISE_ERROR_INTERRUPT_DISABLE|RX_OVERRUN_INTERRUPT_DISABLE|ONE_WIRE_RX_MODE;
  SCI1S2 = RX_BREAK_DETECT_10_BITS | TX_BREAK_10_BITS | RX_IDLE_DETECT_OFF | RX_DATA_NORMAL;
  SCI1BD = UART_BAUD_115200;
  SCI1C2 = RX_WAKEUP_CONTROL_OFF | RX_ENABLE | TX_ENABLE/* | RX_INTERRUPT_ENABLE*/ |IDLE_INTERRUPT_DISABLE | TX_COMPLETE_INTERRUPT_DISABLE |TX_BUFFER_READY_INTERRUPT_DISABLE;

}
/*********************************************************************************
*
*   NAME: InitRadioUART(void)
*
*   \brief:  Initializes SCI2
*
*   \details: Enables Receive Interrupt and sets Baud to 9600. SCI2 Communicates
*             with EM250, EM250 default Baud Rate is 9600.
***********************************************************************************/
void InitRadioUART(void){

  /* configure SCI2/Radio UART mode */
  SCI2C1 = LOOPBACK_OFF | LOOPBACK_TWO_WIRE | UART_OFF_DURING_WAIT | CHARACTER_8BIT | WAKEUP_IDLE_LINE |IDLE_COUNT_AFTER_START_BIT | PARITY_OFF | PARITY_EVEN;
  SCI2C3 = TX_DATA_NORMAL | RX_PARITY_ERROR_INTERRUPT_DISABLE | RX_FRAMING_ERROR_INTERRUPT_DISABLE|RX_NOISE_ERROR_INTERRUPT_DISABLE|RX_OVERRUN_INTERRUPT_DISABLE|ONE_WIRE_RX_MODE;
  SCI2S2 = RX_BREAK_DETECT_10_BITS | TX_BREAK_10_BITS | RX_IDLE_DETECT_OFF | RX_DATA_NORMAL;
  SCI2BD = UART_BAUD_115200;
  SCI2C2 = RX_WAKEUP_CONTROL_OFF | RX_ENABLE | TX_ENABLE /*| RX_INTERRUPT_ENABLE*/ |IDLE_INTERRUPT_DISABLE | TX_COMPLETE_INTERRUPT_DISABLE |TX_BUFFER_READY_INTERRUPT_DISABLE;


}

/*F***************************************************************
*
*   NAME:
*/
/*! \brief
*
*/

void InitUARTs(void){
  InitHostUART();
  InitRadioUART();
}



// *********************************************************************************
// ISR control routines
//**********************************************************************************
void DisableRadioUARTIsr(void){

  //SCI2C2 &= ~(SCI2C2_TE_MASK | SCI2C2_RE_MASK | SCI2C2_RIE_MASK);
  SCI2C2 &= ~(SCI2C2_RIE_MASK);//just disable the interupt
}

void EnableHostUARTIsr(void){

  SCI1C2 |= (SCI1C2_TE_MASK | SCI1C2_RE_MASK | SCI1C2_RIE_MASK);
}

void DisableHostUARTIsr(void){

  //SCI1C2 &= ~(SCI1C2_TE_MASK | SCI1C2_RE_MASK | SCI1C2_RIE_MASK);
  SCI1C2 &= ~(SCI1C2_RIE_MASK); //disable interupt only
}

void EnableRadioUARTIsr(void){

  SCI2C2 |= (SCI2C2_TE_MASK | SCI2C2_RE_MASK | SCI2C2_RIE_MASK);
}
