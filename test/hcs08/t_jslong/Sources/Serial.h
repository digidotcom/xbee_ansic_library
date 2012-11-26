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

extern volatile unsigned char hostRxBuf[];
extern volatile unsigned char hostRxBufIndex;

#include "xbee/platform.h"			// for xbee_serial_t type

#define SCI1_RX_FULL SCI1S1_RDRF
#define SCI1_RX_DATA SCI1D

// Serial initialization and ISR control
void InitUARTs(void);
void DisableRadioUARTIsr(void);
void EnableRadioUARTIsr(void);
void EnableHostUARTIsr(void);
void DisableHostUARTIsr(void);

//SCIxC1 - flags to control SCI serial port options (status register)
#define LOOPBACK_OFF         (0*SCI1C1_LOOPS_MASK)
#define LOOPBACK_ON          (1*SCI1C1_LOOPS_MASK)
#define UART_OFF_DURING_WAIT (1*SCI1C1_SCISWAI_MASK)
#define UART_ON_DURING_WAIT  (0*SCI1C1_SCISWAI_MASK)
#define LOOPBACK_TWO_WIRE    (0*SCI1C1_RSRC_MASK)
#define LOOPBACK_ONE_WIRE    (1*SCI1C1_RSRC_MASK)
#define CHARACTER_8BIT       (0*SCI1C1_M_MASK)
#define CHARACTER_9BIT       (1*SCI1C1_M_MASK)
#define WAKEUP_IDLE_LINE     (0*SCI1C1_WAKE_MASK)
#define WAKEUP_ADDRESS_MARK  (1*SCI1C1_WAKE_MASK)
#define IDLE_COUNT_AFTER_START_BIT (0*SCI1C1_ILT_MASK)
#define IDLE_COUNT_AFTER_STOP_BIT  (1*SCI1C1_ILT_MASK)
#define PARITY_OFF           (0*SCI1C1_PE_MASK)
#define PARITY_ON            (1*SCI1C1_PE_MASK)
#define PARITY_EVEN          (0*SCI1C1_PT_MASK)
#define PARITY_ODD           (1*SCI1C1_PT_MASK)

//SCIxS2 - flags to control SCI serial port options (status register)
#define RX_BREAK_DETECT_10_BITS (0*SCI1S2_LBKDE_MASK)
#define RX_BREAK_DETECT_11_BITS (1*SCI1S2_LBKDE_MASK)
#define TX_BREAK_10_BITS        (0*SCI1S2_BRK13_MASK)
#define TX_BREAK_13_BITS        (1*SCI1S2_BRK13_MASK)
#define RX_IDLE_DETECT_OFF      (0*SCI1S2_RWUID_MASK)
#define RX_IDLE_DETECT_ON       (1*SCI1S2_RWUID_MASK)
#define RX_DATA_INVERTED        (1*SCI1S2_RXINV_MASK)
#define RX_DATA_NORMAL          (0*SCI1S2_RXINV_MASK)

//SCIxC3 - FIXME - does not exist on the target HCS08 processor?
#define RX_PARITY_ERROR_INTERRUPT_DISABLE  (0*SCI1C3_PEIE_MASK)
#define RX_PARITY_ERROR_INTERRUPT_ENABLE   (1*SCI1C3_PEIE_MASK)
#define RX_FRAMING_ERROR_INTERRUPT_DISABLE (0*SCI1C3_FEIE_MASK)
#define RX_FRAMING_ERROR_INTERRUPT_ENABLE  (1*SCI1C3_FEIE_MASK)
#define RX_NOISE_ERROR_INTERRUPT_DISABLE   (0*SCI1C3_NEIE_MASK)
#define RX_NOISE_ERROR_INTERRUPT_ENABLE    (1*SCI1C3_NEIE_MASK)
#define RX_OVERRUN_INTERRUPT_DISABLE       (0*SCI1C3_ORIE_MASK)
#define RX_OVERRUN_INTERRUPT_ENABLE        (1*SCI1C3_ORIE_MASK)
#define TX_DATA_NORMAL                     (0*SCI1C3_TXINV_MASK)
#define TX_DATA_INVERTED                   (1*SCI1C3_TXINV_MASK)
#define ONE_WIRE_RX_MODE                   (0*SCI1C3_TXDIR_MASK)
#define ONE_WIRE_TX_MODE                   (1*SCI1C3_TXDIR_MASK)

// FIXME: Unused
#define WRITE_HOST_9_BITS(bit9,byteData)  while (!SCI1S1_TDRE);   \
                                          SCI1C3_T8=bit9;          \
                                          SCI1D = byteData;       \
// FIXME: Unused
#define WRITE_RADIO_9_BITS(bit9,byteData)  while (!SCI2S1_TDRE);   \
                                           SCI2C3_T8=bit9;          \
                                           SCI2D = byteData;       \

//SCIxC2 - flags to control SCI serial port options (control register)
#define RX_WAKEUP_CONTROL_OFF             (0*SCI1C2_RWU_MASK)
#define RX_WAKEUP_CONTROL_ON              (1*SCI1C2_RWU_MASK)
#define RX_DISABLE                        (0*SCI1C2_RE_MASK)
#define RX_ENABLE                         (1*SCI1C2_RE_MASK)
#define TX_DISABLE                        (0*SCI1C2_TE_MASK)
#define TX_ENABLE                         (1*SCI1C2_TE_MASK)
#define IDLE_INTERRUPT_DISABLE            (0*SCI1C2_ILIE_MASK)
#define IDLE_INTERRUPT_ENABLE             (1*SCI1C2_ILIE_MASK)
#define RX_INTERRUPT_DISABLE              (0*SCI1C2_RIE_MASK)
#define RX_INTERRUPT_ENABLE               (1*SCI1C2_RIE_MASK)
#define TX_COMPLETE_INTERRUPT_DISABLE     (0*SCI1C2_TCIE_MASK)
#define TX_COMPLETE_INTERRUPT_ENABLE      (1*SCI1C2_TCIE_MASK)
#define TX_BUFFER_READY_INTERRUPT_DISABLE (0*SCI1C2_TIE_MASK)
#define TX_BUFFER_READY_INTERRUPT_ENABLE  (1*SCI1C2_TIE_MASK)

#define WRITE_HOST_BREAK()          SCI1C2_SBK = 1;
#define WRITE_RADIO_BREAK()         SCI2C2_SBK = 1;


