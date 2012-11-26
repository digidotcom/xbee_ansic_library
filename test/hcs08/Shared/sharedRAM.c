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

#pragma DATA_SEG SHARED_DATA
volatile APP_RESET_CAUSES AppResetCause   @0x200;
//volatile unsigned short   Reserved        @0x202;
volatile BL_RESET_CAUSES  BLResetCause    @0x204;
//volatile unsigned char    AppUse[6]       @0x206;
volatile _Addr16          iOtaHostAddr16  @0x20C;
volatile _Addr64          iOtaHostAddr64  @0x20E;
#pragma DATA_SEG DEFAULT

