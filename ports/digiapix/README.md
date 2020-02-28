Platform Support: libdigiapix                         {#platform_digiapix}
=============================

Support of Digi International embedded hardware platforms that use
libdigiapix to control GPIO pins.

Initial development done with ConnectCore 6UL SBC Pro hardware, which has
an onboard XBee socket (/dev/ttymxc1).  You will need to update pin
assignments in `xbee_platform_digiapix.c` for use on other products.

Before cross-compiling the samples programs, you will need to follow
instructions for installing the Digi Embedded Yocto toolchain on your Ubuntu
system.  Check [Digi's Embedded Documentation][1] for all products, or the
page for [command-line development on the CC6UL][2].

[1]: https://www.digi.com/resources/documentation/digidocs/embedded/
[2]: https://www.digi.com/resources/documentation/digidocs/embedded/dey/2.6/cc6ul/yocto_c_cl-develop-app
