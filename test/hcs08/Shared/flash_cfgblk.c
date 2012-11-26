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
/**
	@file flash_cfgblk.c
	@ingroup flash_freescale

	Support code for configuration parameter blocks in flash.  The following
	low-level functions are available:

	    erase_cfgBlock()
	    program_cfgBlock()
	    find_cfgBlock()

	 These functions are used by the following high-level functions, which
	 automatically handle determination of which configuration block is
	 valid and which is invalid:

	    createNewCfgBlock()
	    writeNewCfgBlock()
	    finalizeNewCfgBlock()
*/


#include "MC9S08QE32.h"  /*!< Chip definition file */
#include "common.h"
#include "System.h"

#include "build_defines.h"
#include "flashLT.h"
#include "flash_cfgblk.h"

#include <hidef.h>      /*!< for EnableInterrupts macro */

#include <stdio.h>    // FOR DEBUGGING ONLY!


uint8 valid_cfgBlk_num = 0;   // value of current valid config block

/*****************************************************************/
/**
  Erase a particular configuration block.

	@param[in]	block
					Block number to erase.  Only 1 or 2 are valid.

	@retval	0 success
	@retval 1 flash erase failure
	@retval 2 invalid block number

	@see	find_cfgBlock(), program_cfgBlock(), writeNewCfgBlock(),
	      createNewCfgBlock()

*/

uint8 erase_cfgBlock(uint8 block)
{
  uint8 err;

  if ((block != 1) && (block != 2))

    return(2);

  DisableInterrupts;
  err = erase(FLASH_CFGBLK_BASE_ADDR(block));
  WATCHDOG_RESET();
  EnableInterrupts;

  return(err);
}

/*****************************************************************/
/**
  Erase a particular configuration block.


	@retval	The number of the valid configuration block (1 or 2).
	        0 if no valid block found.

	@see	erase_cfgBlock(), program_cfgBlock(), writeNewCfgBlock(),
	      programNewCfgBlock(), finalizeNewCfgBlock()
*/


uint8 find_cfgBlock()
{
  uint8   block, mark;
  uint16  cksum;


  block = 1;
  while (block <= 2) {

    mark = FLASH_CFGBLK_MARK(block);
    cksum = FLASH_CFGBLK_CKSUM(block);

    // check for valid mark byte (not 0x00 or 0xFF)
    if (mark && (mark != 0xFF)) {

      // test checksum
      cksum = fletcher16(FLASH_CFGBLK_BASE_ADDR(block),
                          FLASH_CFGBLK_DATA_SIZE, 0xFFFF);
      if (cksum != FLASH_CFGBLK_CKSUM(block))
        return(0);

      valid_cfgBlk_num = block;
      return(block);  // return valid block number
    }

    block++;  // try next block
  }

  return(0);  // ERROR: no valid configuration block found
}

/*****************************************************************/
/**
  Program part of a particular configuration block.


	@param[in]	block_num
					Block number to program.  Only 1 or 2 are valid.

	@param[in]	offset
					Offset within the block to start writing at.  Values over 511 are invalid.

	@param[in]	buffer
					Pointer to the data to be written.

	@param[in]	length
					Number of bytes to write.  Values over 512 are invalid.


	@retval	0 success
	@retval 1 flash write failure
	@retval 2 invalid block number
	@retval 3 invalid offset
	@retval 4 invalid length

	@see	erase_cfgBlock(), find_cfgBlock(),
	      writeNewCfgBlock(), finalizeNewCfgBlock()
*/

uint8 program_cfgBlock(uint8 block_num, uint16 offset,
            const void *buffer, uint16 length)
{
  uint8 err;

  if (!block_num || (block_num > 2))    return(2);
  if (offset > 511)                     return(3);
  if (length > 512)                     return(4);

  DisableInterrupts;
  err = program(FLASH_CFGBLK_BASE_ADDR(block_num) + offset,
                  (uint16)buffer, length);
  EnableInterrupts;

  return(err);
}

/*****************************************************************/
 /*
  Perform a fletcher-16 checksum calculation over a block of memory.


	@param[in]	address
					Block starting address.

	@param[in]	length
					Number of bytes to include in the checksum calculation.

	@param[in]	checksum
					Initial checksum value.  0xFFFF is common for the start
					of a new checksum calculation.


	@retval	checksum result

	@see	finalizeNewCfgBlock()
*/

/**
  @todo: an assembly version is also a possibility...?
*/

uint16 fletcher16(uint16 address, uint16 length, uint16 checksum)
{
  uint16  sum1, sum2;
  uint16  tlen;


  //sum1 = sum2 = 0xFF;
  sum2 = (checksum & 0xFF00)>>8;
  sum1 = checksum & 0x00FF;

  while (length) {
    // The max number of sums that we can do before requiring a
    // modular reduction is 21.
    if (length > 21)  tlen = 21;
    else              tlen = length;

    length -= tlen;
    do {
      sum1 += *(uint8 *)address++;
      sum2 += sum1;
    } while (--tlen);

    // reduce sums to 8 bits
    sum1 = (sum1 & 0xFF) + (sum1 >> 8);
    sum2 = (sum2 & 0xFF) + (sum2 >> 8);
  }

  // final reduction
  sum1 = (sum1 & 0xFF) + (sum1 >> 8);
  sum2 = (sum2 & 0xFF) + (sum2 >> 8);

  return( (sum2 & 0x00FF)<<8 | (sum1 & 0x00FF) );
}

/*****************************************************************/
/**
  Initialize a new configuration block in flash.  The function will
  erase the currently-invalid configuration block; if no blocks are
  valid it will erase the first one.

  #writeNewCfgBlock() should be used to add data to the new block,
  followed by #finalizeNewCfgBlock() to complete the process.


	@retval	0 success
	@retval 1 flash erase failure

	@see	find_cfgBlock(), writeNewCfgBlock(), finalizeNewCfgBlock()
*/

uint8 createNewCfgBlock(void)
{
  uint8 err;

  if (!valid_cfgBlk_num) {
    // if no valid config block, use block 1
    err = erase_cfgBlock(1);

  } else {
    // otherwise use whichever one is not valid
    err = erase_cfgBlock(INVALID_CFGBLK_NUM);
  }

  return(err);
}

/*****************************************************************/
/**
  Program part of a new configuration block.  This function assumes
  that #createNewCfgBlock() has been called first.

  When all writing is done, #finalizeNewCfgBlock() should be called
  to complete the process.


	@param[in]	offset
					Offset within the block to start writing at.  Values over 511 are invalid.

	@param[in]	buffer
					Pointer to the data to be written.

	@param[in]	length
					Number of byte to write.  Values over 512 are invalid.


	@retval	0 success
	@retval 1 flash write failure
	@retval 2 invalid block number
	@retval 3 invalid offset
	@retval 4 invalid length


	@see	find_cfgBlock(), createNewCfgBlock(), finalizeNewCfgBlock()
*/

uint8 writeNewCfgBlock(uint16 offset, const void *buffer, uint16 length)
{
  uint8 err;

  if (!valid_cfgBlk_num) {
    // if no valid config block, use block 1
    err = program_cfgBlock(1, offset, buffer, length);

  } else {
    // otherwise use whichever one is not valid
    err = program_cfgBlock(INVALID_CFGBLK_NUM, offset, buffer, length);
  }

  return(err);
}

/*****************************************************************/
/**
  Finalize writing of a new configuration block.  This function
  will write the mark byte and checksum out to the new configuration
  block, then invalidate the old block.

  #createNewCfgBlock() and #writeNewCfgBlock() should be called
  before this function.


	@retval	0 success
	@retval 1 flash write failure

	@see	find_cfgBlock(), writeNewCfgBlock(), programNewCfgBlock()
*/

uint8 finalizeNewCfgBlock(void)
{
  uint8   mark, err;
  uint16  cksum;


  // if no valid config block, block 1 will become valid block
  //  so we need to mark block 2 as "valid" for the rest of
  //  this function.
  if (!valid_cfgBlk_num)  valid_cfgBlk_num = 2;

  // mark previously-invalid block as now valid (must be done first
  //    since mark byte is part of checksum data)
  mark = 0x55;
  err = program_cfgBlock(INVALID_CFGBLK_NUM, FLASH_CFGBLK_MARK_OFFSET, &mark, 1);
  if (err)  return(err);

  // generate and store checksum
  cksum = fletcher16(FLASH_CFGBLK_BASE_ADDR(INVALID_CFGBLK_NUM),
                        FLASH_CFGBLK_DATA_SIZE, 0xFFFF);

  program_cfgBlock(INVALID_CFGBLK_NUM, FLASH_CFGBLK_CKSUM_OFFSET, &cksum, 2);

  // mark previously-valid block as now invalid
  mark = 0x00;
  err = program_cfgBlock(VALID_CFGBLK_NUM, FLASH_CFGBLK_MARK_OFFSET, &mark, 1);
  if (err)  return(err);

  // update to point to new block (which was previously invalid)
  valid_cfgBlk_num = INVALID_CFGBLK_NUM;

  return(0);
}
