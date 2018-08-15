# Arduino_Voltimeter
14 bits arduino voltimeter with SCPI usb and ethernet interfaces

##Required hardware

**ADS1115**
>
Conections:
SCL -> A5
SDA -> A4
ADDR -> A3
ALRT -> A2
A0 , A1 -> BNC voltage input  (diferetial measurement)

**ENC28J60** (optional, for ethernet support)
>
Conections
CS (ChipSelect) -> PIN2
VCC -> 3.3V
GND, SCK, SO, SI, -> Arduino SPI port

##SCPI Commands

**Termination chars**
>
Read termination : LF (\n)
Write termination : LF (\n)

**Visa Strings**
>
Serial port : 'ASRL/dev/ttyACM<X>::INSTR'
Ethernet 'TCPIP0::<IP>::5025::SOCKET'

***IDN? - Identification query**
>
Syntax: *IDN?
Return value: Character string

**MEASure:VOLTage? - Query**
>
Syntax: MEAS:VOLT?
Return value: Measured voltage (in volts)

**CONFigure:GAIN - Command**
>
Syntax: CONF:GAIN {2/3 | 1 | 2 | 4 | 8 |16}
Description: Configures the AD preamplifier 

**CONFigure:GAIN? - Query**
>
Syntax: CONF:GAIN?
Return value: AD preamplifier gain configuration

**SYSTem:COMMunicate:LAN: - Commands**  
>
Syntax: SYSTem:COMMunicate:LAN:ADDRess <IP\>
Description: Sets the IP address
>
Syntax: SYSTem:COMMunicate:LAN:DGATeway <IP\>
Description : Sets the default gateway address
>
Syntax: SYSTem:COMMunicate:LAN:MAC <MAC\>
Description sets: MAC address
>
Default Values
ADDR (IP) = 192.168.13.7
GateWay = 0.0.0.0
MAC = 0x74, 0x69, 0x69, 0x2D, 0x30, 0x31


**SYSTem:COMMunicate:LAN: - Queries**  
>
Syntax: SYSTem:COMMunicate:LAN:ADDRess?
Return value: IP address
>
Syntax: SYSTem:COMMunicate:LAN:DGATeway?
Return value: Default gateway address
>
Syntax: SYSTem:COMMunicate:LAN:MAC?
Return value: MAC address
