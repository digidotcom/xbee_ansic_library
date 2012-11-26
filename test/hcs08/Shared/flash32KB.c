/*-------------------------------------------------------
* Copyright (C) 2009 Digi International, All Rights Reserved.
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


#include "MC9S08QE32.h"  /*!< Chip definition file */
#include "common.h"
#include "flashLT.h"
#include "System.h"


extern char __SEG_START_FLASH_ROUTINES[];
extern char __SEG_SIZE_FLASH_ROUTINES[];



#ifdef INCLUDE_PART_ID_CODE
extern uint16 partId;
// Start addresses for the 32KB and the 16KB part respectively
// Note: These are offset by 1KB because the 1st 1KB
//       is reserved for app usage.
// TODO - Addresses are hardcoded but the PRM values can't be used.  Since
//        the BL doesn't place anything into the APP area, the linker
//        doesn't make __SEG_START_APP_ROM available.
#endif

#define FL_CMD_BLANK_CHECK    0x05
#define FL_CMD_BYTE_PROGRAM   0x20
#define FL_CMD_BURST_PROGRAM  0x25
#define FL_CMD_PAGE_ERASE     0x40
#define FL_CMD_MASS_ERASE     0x41

#pragma CODE_SEG DEFAULT
void CopyInRAM(void);

#pragma CODE_SEG FLASH_ROUTINES
uint8 erase(uint16);
uint8 program(uint16, uint16, uint16);

/*F***************************************************************
*
*   NAME:
*/
/*! \brief
*
*	\detail
*  returns 1 for ERROR
*  returns 0 for Success
*
*/
uint8 erase(uint16 address)
{
  uint16 ptr = address;
  uint8 result = 0;

  // Wait for command bufffer empty
  while (!(_FSTAT.Bits.FCBEF));

  if (_FSTAT.Bits.FACCERR) {
    _FSTAT.Bits.FACCERR = 1;
  }
  if (_FSTAT.Bits.FPVIOL) {
    _FSTAT.Bits.FPVIOL = 1;
  }

  // First step - write a dummy value to the page to be addressed.
  // NOTE: Don't use 0x00 as a data value here. The compiler produces code
  //       that does a "clr" opcode instead of a "mov". The flash state
  //       machine doesn't "see" the "clr" and the erase does not occur.
  *((volatile uint8 *)ptr) = 0x33;

  _FCMD.Byte = FL_CMD_PAGE_ERASE;
  _FSTAT.Bits.FCBEF = 1;

  // Spec says to wait 4 cycles
  asm {
    nop
    nop
    nop
    nop
  }

  if ((_FSTAT.Bits.FPVIOL) || (_FSTAT.Bits.FACCERR)) {
      result = 1;
  }
  while (!(_FSTAT.Bits.FCCF));

  return result;
}

/*F***************************************************************
*
*   NAME:   program(
*/
/*! \brief	writes flash
*
*	\detail		returns 1 for ERROR
*
*/
uint8 program(uint16 address, uint16 buffer,
                      uint16 length)
{
  uint16 pAddr = address;
  uint16 pData = buffer;
  //uint8 result = 0;

  if (_FSTAT.Bits.FACCERR) {
    _FSTAT.Bits.FACCERR = 1;
  }
  if (_FSTAT.Bits.FPVIOL) {
    _FSTAT.Bits.FPVIOL = 1;
  }

  while (length--)
  {
      WATCHDOG_RESET();
      while (!(_FSTAT.Bits.FCBEF));

      *((volatile uint8 *)pAddr) = *((uint8 *)pData);
      _FCMD.Byte = FL_CMD_BURST_PROGRAM;
      _FSTAT.Bits.FCBEF = 1;

      // Spec says to wait 4 cycles
      asm {
        nop
        nop
        nop
        nop
      }

      if ((_FSTAT.Bits.FPVIOL) || (_FSTAT.Bits.FACCERR)) {
          //result = 1;
          //break;
          return 1;
      }

      pAddr++;
      pData++;
  }

  while (!(_FSTAT.Bits.FCCF));

  //return result;
  return 0;

}

#pragma CODE_SEG DEFAULT
/*F***************************************************************
*
*   NAME:   programByte()
*
* \brief	  This function programs 1 byte of flash to the address
*
*	\detail		Returns 1 for ERROR
*/
uint8 programByte(uint16 address, byte value)
{
    return program(address,(uint16)&value, 1);
}


/*F***************************************************************
*
*   NAME:   CopyInRAMFlashFunctions()
*/
/*! \brief	Copy flash erase/write routines from ROM to RAM.
*
*	\detail		This section of the code is the one that copies
*            the routine into RAM. It is following the steps
*            documented in Technical Note 228.
*
*  The .PRM file describes the APP_FLASH_ROM segment to hold
*	CODE_SEG FLASH_ROUTINES.  This function is responsible for
*	copying those routines to RAM, where they are called.
*
*	The linker is responsible for associating the RAM addresses of
*	functions in that segment with their names.
*/
#include <string.h>
void CopyInRAMFlashFunctions(void)
{
	memcpy(&erase, __SEG_START_FLASH_ROUTINES,
		(unsigned) __SEG_SIZE_FLASH_ROUTINES);
}





