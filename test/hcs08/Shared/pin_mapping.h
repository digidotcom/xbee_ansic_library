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



/*
*  This is a pin mapping for the 20 pin XBEE to the Freescale processor
*  Each pin is numbered and has the Freescale Port,Pin and XBEE Pin Name associated with it.
*  Example Port A, Pin 3, XBee Name DIO1/ADC1 = A3-DIO1/ADC1
*
*/
/*******              Pin Mapping                      ***********
*                         *********
*                        *         *
*                       *           *
*                      *    *****    *
* VCC                1*    *     *    *20 A0-DIO0/ADC0/Commissioning
* B1-DOUT            2*   *       *   *19 A3-DIO1/ADC1
* B0-DIN             3*   * XBEE  *   *18 B2-DIO2/ADC2
* B4-CD/DIO12        4*   * Radio *   *17 B5/A7-DIO3/ADC3
* A5-RESET           5*   * EMBER *   *16 D7-RTS/DIO6 (Input)
* C5-PWM0/DIO10/RSSI 6*   *********   *15 D4-Associate/DIO5
* A2-PWM1/DIO11      7*               *14 VREF for Freescale
* A4-BKGD            8*   Freescale   *13 A1-ON_nSleep/DIO9
* D5-DTR/DIO8        9*               *12 C0-CTS/DIO7  (Output)
* GND               10*               *11 B3-DIO4
*                     *****************
* Additional Lines to Internal XBEE RF communications
* C7-DIN_XBEE_RADIO
* C6-DOUT_XBEE_RADIO
* C1-RTS_XBEE_RADIO
* D6-CTS_XBEE_RADIO
* C4-RESET_XBEE_RADIO
*
 ******************************************************************/

//input A0-DIO0/ADC0/Commissioning is commissioning (Careful!! This line can cause the Ember module to exit the network)
//Send EMBER command to disable commissioning if using this pin (ATD0 = 0)


/*  Undefined pins
VCC
GND
VREF
BKGD               PTAD_PTAD4
EXTERNAL_RESET     PTAD_PTAD5
*/

/*                                 pin definitions                                 */
/*                                 *           Pin Direction of IO (Input =0)      */
/*                                 *           *             pin pullup Enable     */
/*                                 *           *             *                     */
#define IO_DIO0_ADC0_COMMISSIONING PTAD_PTAD0
#define IO_DIO0_ADC0_COMMISSIONING_D           PTADD_PTADD0
#define IO_DIO0_ADC0_COMMISSIONING_PE                        PTAPE_PTAPE0
#define IO_DIO1_ADC1               PTAD_PTAD3
#define IO_DIO1_ADC1_D                         PTADD_PTADD3
#define IO_DIO1_ADC1_PE                                      PTAPE_PTAPE3
#define IO_DIO2_ADC2               PTBD_PTBD2
#define IO_DIO2_ADC2_D                         PTBDD_PTBDD2
#define IO_DIO2_ADC2_PE                                      PTBPE_PTBPE2
#define IO_DIO3_ADC3               PTBD_PTBD5
#define IO_DIO3_ADC3_D                         PTBDD_PTBDD5
#define IO_DIO3_ADC3_PE                                      PTBPE_PTBPE5
#define IO_DIO9_ADC9_ON_SLEEP      PTAD_PTAD1
#define IO_DIO9_ADC9_ON_SLEEP_D                PTADD_PTADD1
#define IO_DIO9_ADC9_ON_SLEEP_PE                             PTAPE_PTAPE1
#define IO_DIO4_ADC4               PTBD_PTBD3
#define IO_DIO4_ADC4_D                         PTBDD_PTBDD3
#define IO_DIO4_ADC4_PE                                      PTBPE_PTBPE3
#define IO_DIO8_DTR                PTDD_PTDD5
#define IO_DIO8_DTR_D                          PTDDD_PTDDD5
#define IO_DIO8_DTR_PE                                       PTDPE_PTDPE5
#define IO_DIO11_ADC11_PWM1        PTAD_PTAD2
#define IO_DIO11_ADC11_PWM1_D                  PTADD_PTADD2
#define IO_DIO11_ADC11_PWM1_PE                               PTAPE_PTAPE2
#define IO_DIO12_CD                PTBD_PTBD4
#define IO_DIO12_CD_D                          PTBDD_PTBDD4
#define IO_DIO12_CD_PE                                       PTBPE_PTBPE4

//Inputs to Freescale
#define IO_DOUT_RADIO              PTCD_PTCD6
#define IO_DOUT_RADIO_D                        PTCDD_PTCDD6
#define IO_DOUT_RADIO_PE                                     PTCPE_PTCPE6
#define IO_CTS_RADIO               PTDD_PTDD6
#define IO_CTS_RADIO_D                         PTDDD_PTDDD6
#define IO_CTS_RADIO_PE                                      PTDPE_PTDPE6
#define IO_DIO5_ASSOCIATE          PTDD_PTDD4
#define IO_DIO5_ASSOCIATE_D                    PTDDD_PTDDD4
#define IO_DIO5_ASSOCIATE_PE                                 PTDPE_PTDPE4
#define IO_DIN_HOST                PTBD_PTBD0
#define IO_DIN_HOST_D                          PTBDD_PTBDD0
#define IO_DIN_HOST_PE                                       PTBPE_PTBPE0
#define IO_DIO6_RTS_HOST           PTDD_PTDD7
#define IO_DIO6_RTS_HOST_D                     PTDDD_PTDDD7
#define IO_DIO6_RTS_HOST_PE                                  PTDPE_PTDPE7
#define IO_DIO10_PWM0_RSSI         PTCD_PTCD5
#define IO_DIO10_PWM0_RSSI_D                   PTCDD_PTCDD5
#define IO_DIO10_PWM0_RSSI_PE                                PTCPE_PTCPE5

//Outputs to Freescale
#define IO_DIN_RADIO               PTCD_PTCD7
#define IO_DIN_RADIO_D                         PTCDD_PTCDD7
#define IO_DIN_RADIO_PE                                      PTCPE_PTCPE7
#define IO_RTS_RADIO               PTCD_PTCD1
#define IO_RTS_RADIO_D                         PTCDD_PTCDD1
#define IO_RTS_RADIO_PE                                      PTCPE_PTCPE1
#define IO_DIO7_CTS_HOST           PTCD_PTCD0
#define IO_DIO7_CTS_HOST_D                     PTCDD_PTCDD0
#define IO_DIO7_CTS_HOST_PE                                  PTCPE_PTCPE0
#define IO_DOUT_HOST               PTBD_PTBD1
#define IO_DOUT_HOST_D                         PTBDD_PTBDD1
#define IO_DOUT_HOST_PE                                      PTBPE_PTBPE1

//Use Output Low / Input for High to reset the radio
#define IO_RESET_RADIO             PTCD_PTCD4
#define IO_RESET_RADIO_D                       PTCD_PTCD4
#define IO_RESET_RADIO_PE                                    PTCD_PTCD4
#define IO_RESET_RADIO_LOW()       IO_RESET_RADIO = 0; IO_RESET_RADIO_D = 1
#define IO_RESET_RADIO_HIGH()      IO_RESET_RADIO_D = 0;


//ADC Mapping
//VREF must be connected to make an ADC reading with the Freescale
#define IO_ADC0      0
#define IO_ADC1      3
#define IO_ADC2      6
#define IO_ADC3      9
#define IO_ADC9      1
#define IO_ADC4      7
#define IO_ADC11     2
#define IO_ADC_DOUT  5
#define IO_ADC_DIN   4

//ADC Helpers
#define ADC_CONVERSION_ACTIVE ADCSC2_ADACT
//#define ADC_CONVERSION_DONE   ADCSC1_COCO
#define StartADC(portNumber) ADCSC1 = ADCSC1_AIEN_MASK | portNumber

#define IO_DIO5_ASSOCIATE_INTERUPT2          KBI2PE_KBIPE4_MASK
#define IO_DIO5_ASSOCIATE_RISING_EDGE()      KBI2ES_KBEDG4=1
#define IO_DIO5_ASSOCIATE_FALLING_EDGE()     KBI2ES_KBEDG4=0
#define IO_DIO5_ASSOCIATE_TOGGLE_EDGE()      KBI2ES_KBEDG4=!KBI2ES_KBEDG4
#define KB_INTERUPT2_FLAG()                  KBI2SC_KBF
#define KB_INTERUPT2_CLEAR()                 KBI2SC_KBACK=1
#define KB_INTERUPT2_DETECT_EDGE_AND_LEVEL() KBI2SC_KBIMOD=1
#define KB_INTERUPT2_DETECT_EDGE()           KBI2SC_KBIMOD=0
#define KB_INTERUPT2_ENABLE(pins)            KBI2PE |= pins;   \
                                             KBI2SC_KBIE=1
#define KB_INTERUPT2_DISABLE_ALL()           KBI2PE=0;         \
                                             KBI2SC_KBIE=0
#define KB_INTERUPT2_DISABLE(pins)           KBI2PE &= ~pins;  \
                                             if(!KBI2PE)       \
                                             {                 \
                                                KBI2SC_KBIE=0; \
                                             }

#define KB_INTERUPT1_FLAG()                  KBI1SC_KBF
#define KB_INTERUPT1_CLEAR()                 KBI1SC_KBACK=1
#define KB_INTERUPT1_DISABLE()               KBI1SC_KBIE=0





