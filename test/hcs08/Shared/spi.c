//////////////////////////////////////////////////////////////////////
//
//  Quick-and-dirty library for ERT SPI communications
//
//  Pin mapping:
//
//    /CS       PTB5
//    CLK       PTB2
//    MOSI      PTB3
//    MISO      PTB4
//
//    DATA_RDY  PTC5
//    INIT_DONE PTA2
//    RESET     not connected (tied high)
//
//////////////////////////////////////////////////////////////////////

#include "derivative.h"
#include "ert_interface.h"
#include "spi.h"

//////////////////////////////////////////////////////////////////////

void spi_init(char baudratedivisor)
{
  ERT_CS_HIGH();

  SPIC1 = 0x50; // enable SPI as master, CPOL=0, CPHA=0
  SPIC2 = 0x00; // no dedicated /SS
  SPIBR = baudratedivisor;
}

//////////////////////////////////////////////////////////////////////
// simultaneously transmit and receive a single byte

char spi_txrx(char txdata)
{
  char  rxdata;

   // wait for TX buffer to be empty
   while (!(SPIS & 0x20))
    /* do nothing */;

  SPID = txdata;

  // wait for data in RX buffer to be avail
  while (!(SPIS & 0x80))
    /* do nothing */;
  rxdata = SPID;

  return(rxdata);
}

//////////////////////////////////////////////////////////////////////
// read some number of bytes

void spi_read(void *buf, int len)
{
  char  *bufptr;

  bufptr = buf;
  while (len) {
    *bufptr = spi_txrx(0xFF);
    bufptr++;
    len--;
  }
}

//////////////////////////////////////////////////////////////////////
// write some number of bytes

void spi_write(const void *buf, int len)
{
  char  *bufptr;

  bufptr = buf;
  while (len) {
    (void)spi_txrx(*bufptr);  // ignore received byte
    bufptr++;
    len--;
  }
}

