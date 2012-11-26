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

#include "common.h"
#include "flashLT.h"

// config block size parameters
#define FLASH_CFGBLK_SIZE           FLASH_PAGE_SIZE
#define FLASH_CFGBLK_DATA_SIZE      ((FLASH_CFGBLK_SIZE) - 2)
  // mark byte is included in the data size for checksum calculation

// offsets into config block
#define FLASH_CFGBLK_MARK_OFFSET    ((FLASH_CFGBLK_SIZE) - 3)
#define FLASH_CFGBLK_CKSUM_OFFSET   ((FLASH_CFGBLK_SIZE) - 2)

// config block addresses
#define FLASH_CFGBLK_BASE_ADDR(x)   (APP_RESERVED_FLASH_START_ADDRESS + FLASH_CFGBLK_SIZE*((x)-1))
#define FLASH_CFGBLK_MARK_ADDR(x)   (FLASH_CFGBLK_BASE_ADDR(x) + FLASH_CFGBLK_MARK_OFFSET)
#define FLASH_CFGBLK_CKSUM_ADDR(x)  (FLASH_CFGBLK_BASE_ADDR(x) + FLASH_CFGBLK_CKSUM_OFFSET)

// config block values
#define FLASH_CFGBLK_MARK(x)        *(uint8 *)FLASH_CFGBLK_MARK_ADDR(x)
#define FLASH_CFGBLK_CKSUM(x)       *(uint16 *)FLASH_CFGBLK_CKSUM_ADDR(x)

// pointer to valid config block (NULL if none)
#define FLASH_CFGBLK  ( VALID_CFGBLK_NUM ? (const void *)FLASH_CFGBLK_BASE_ADDR(VALID_CFGBLK_NUM) : NULL )

/*****************************************************************/
/*
#define MESSAGE_SIZE 20

typedef struct {
  byte message[MESSAGE_SIZE];
  byte zero;
  byte checksum;
} CONFIG_FLASH_STRUCT;
*/

extern  uint8 valid_cfgBlk_num;

#define VALID_CFGBLK_NUM        valid_cfgBlk_num
#define INVALID_CFGBLK_NUM      (3 - valid_cfgBlk_num)

/*****************************************************************/

uint8 find_cfgBlock(void);
uint8 erase_cfgBlock(uint8);
uint8 program_cfgBlock(uint8, uint16, const void *, uint16);
uint16 fletcher16(uint16, uint16, uint16);

uint8 createNewCfgBlock(void);
uint8 writeNewCfgBlock(uint16, const void *, uint16);
uint8 finalizeNewCfgBlock(void);
