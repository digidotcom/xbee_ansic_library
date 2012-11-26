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
#ifndef COMMON
#define COMMON


// 'int' and 'word' are ambiguous,  their size depending on the compiler, target, or platform.
// We define the following types using symbols which clearly indicate their own size.
typedef unsigned char  uint8;
typedef unsigned short uint16;
typedef unsigned long  uint32;
typedef char  int8;
typedef short int16;
typedef long  int32;


typedef enum BL_RESET_CAUSES {
  BL_CAUSE_NOTHING   = 0x0000,  //reset pin, low voltage, Power On Reset,gotoAPP
  BL_CAUSE_NOTHING_COUNT   = 0x0001,//BL_Reset_Cause incremented each time until Bad App
  BL_CAUSE_BAD_APP   = 0x0010,
} BL_RESET_CAUSES;

typedef enum APP_RESET_CAUSES {
  APP_CAUSE_NOTHING                   = 0x0000,  //0x0000 to 0x00FD are considered valid for APP Use
  APP_CAUSE_COPY_FLASH_SUCCESS        = 0x00FE,
  APP_CAUSE_COPY_FLASH_FAILED         = 0x00FF,
  APP_CAUSE_FIRMWARE_UPDATE           = 0x5981,
  APP_CAUSE_FIRMWARE_UPDATE_APS       = 0x5983,  // APP_CAUSE_FIRMWARE_UPDATE | 0x0002
  APP_CAUSE_BYPASS_MODE               = 0x4682,
  APP_CAUSE_BOOTLOADER_MENU           = 0x6A18,
  APP_CAUSE_COPY_FLASH_TO_RUN_SPACE   = -0x5482, // (int)0xAB7E
  APP_CAUSE_REQUEST_SOPT1_NOT_WRITTEN = 0x7815,
} APP_RESET_CAUSES;

typedef struct {
	uint8 addr[8];
} _Addr64;

typedef struct {
	uint8 addr[2];
} _Addr16;


#define WDR() __RESET_WATCHDOG();
#define WATCHDOG_RESET() __RESET_WATCHDOG();

typedef char bool;
#define TRUE  1
#define FALSE 0

#endif
