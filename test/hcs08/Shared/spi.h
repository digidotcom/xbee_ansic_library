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

void spi_init(char);
char spi_txrx(char);
void spi_read(void *, int);
void spi_write(const void *, int);
