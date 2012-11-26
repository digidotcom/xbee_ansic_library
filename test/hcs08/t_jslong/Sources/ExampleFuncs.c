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

#include <hidef.h>      /*!< for EnableInterrupts macro */
#include <string.h>
#include "derivative.h" /*!< include peripheral declarations */
#include "common.h"     /*!< Common definitions for BL and APP */
#include "Serial.h"
#include "rtc.h"
#include "pin_mapping.h"
#include "System.h"

#pragma DATA_SEG SHARED_DATA
volatile APP_RESET_CAUSES AppResetCause   @0x200;
volatile BL_RESET_CAUSES  BLResetCause    @0x204;
volatile uint16           AppString       @0x208;
volatile uint16           CmdMode         @0x20A;
volatile _Addr16          iOtaHostAddr16  @0x20C;
volatile _Addr64          iOtaHostAddr64  @0x20E;
#pragma DATA_SEG DEFAULT


void  InitGpio(void);
void  ResetEM250(void);
uint16 int GetADCVal(void);
void InitADC(void);
void vAdc(void);
void ResetHardware(APP_RESET_CAUSES Reset_Val);

uint16 lastADCvalue;

/************************************************************************
*
*   NAME: vInitGpio(void)
*
*   \brief: Initializes appropriate MC9S08 GPIO that are connected to
*           to the EM250.
*
*   \details:
*
*/
void InitGpio(void){
  char portE;

  PTADD = 0;//set all as inputs
  PTBDD = 0;//set all as inputs
  PTCDD = 0;//set all as inputs
  PTDDD = 0;//set all as inputs
  PTEPE = 0x70;//turn on Hardware detect pull-ups
  PTEDD = 0;//set all as inputs
  portE = PTED & HARDWARE_MASK_PORTE;//read hardware detect lines

  PTAPE = 1<<6 | 1<<5 | 1<<4;//disable pullups except for no connects
  PTBPE = 0xC1;//3<<6+1;//disable pullups except for no connects and DIN
  PTCPE = 7<<2;//disable pullups except for no connects and RESET_XBEE
  PTDPE = 0xF;//disable pullups except for no connects
  PTEPE = portE | 0x8F;//disable pullups except for no connects

  IO_DIO7_CTS_HOST = 0;   //Assert CTS Line
  IO_DIO7_CTS_HOST_D = 1; //Set CTS as output to computer

//SETUP DIO HERE
  IO_DIO0_ADC0_COMMISSIONING_PE = 1;// Set pullups
  IO_DIO1_ADC1_PE = 1;
  IO_DIO2_ADC2_PE = 1;
  IO_DIO3_ADC3_PE = 1;

  IO_DIO12_CD = 1;// Set IO Lines high for default
  IO_DIO11_ADC11_PWM1 = 1;
  IO_DIO4_ADC4 = 1;
  IO_RTS_RADIO = 0;//assert RTS

  IO_DIO12_CD_D         = 1;// Set IO Lines as Outputs
  IO_DIO11_ADC11_PWM1_D = 1;
  IO_DIO4_ADC4_D = 1;
  IO_RTS_RADIO_D = 1;

}
/****************************************************************
*
*   NAME: vResetEM250(void)
*
*   \brief: Resets EM250
*
*   \details: Pulls RESET_EM250 line low for ~50ms and then releases
*
*/
void ResetEM250(void)
{
  IO_RESET_RADIO_LOW();
  //wait ~52ms (EM250 spec says min 250nS...)
  wait_ms(52);
  IO_RESET_RADIO_HIGH();

}

/*******************************************************************************************
*
*   NAME: vResetHardware
*
*   \brief:Resets the EM250 and passes control to the Bootloader.
*
*   \details: After EM250 Reset writes to the shared memory variable(Bootloader/Application)
*            ,AppResetCause, to indicate to the Bootloader that it should do when control is
*            passed to it after forcing a WDTimeout.
*
*
*/
void ResetHardware(APP_RESET_CAUSES Reset_Val){

    ResetEM250();
    DisableInterrupts;
    AppResetCause = Reset_Val;
    if (APP_CAUSE_REQUEST_SOPT1_NOT_WRITTEN ==  AppResetCause){
      BLResetCause = BL_CAUSE_NOTHING;
    }
    for(;;);           //Wait for WDTimeout

}

/****************************************************************
*
*   NAME: vAdc(void)
*
*   \brief: A/D ISR
*
*   \details: Stores 12bit A/D value in lastADCvalue
*
*/
#pragma TRAP_PROC

void vAdc(void){
uint16 ADCVal;


     ADCVal = (int)ADCRH << 8; //read AD value
     ADCVal |= (int)ADCRL;

     lastADCvalue = ADCVal;
}
