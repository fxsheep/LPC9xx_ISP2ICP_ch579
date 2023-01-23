# LPC9xx_ISP2ICP_ch579
NXP LPC900 series ISP to ICP bridge on WCH CH579

## Usage
Wiring:

|   CH579   |   LPC9XX   |
|    ---    |    ----    |
|    PA2    |    VDD     |
|    PA3    | P1.5(nRST) |
|    PA4    | P0.5(PCL)  |
|    PA5    | P0.4(PDA)  |
|    VSS    |    VSS     |

Connect CH579 board with target MCU as wiring above. Power up/reset CH579, target will enter ICP mode, which is converted to ISP protocol on UART1(PA8-RX, PA9-TX) of CH579. An ISP programming utility (e.g. Flash Magic, until version 11.20.5190) can then be used on that serial port.
