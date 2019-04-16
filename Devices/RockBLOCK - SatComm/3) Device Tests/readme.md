# Working with the ROCKBLOCK+

## Interface
- 9-30VDC 
- GND 
- RX
- TX
- Network Availability
- Ring Indicator
- On/Off

The RX, TX, Network Availability, Ring Indicator, and On/Off connections all require an RS232 interface to communicate with a device
During testing the MAX3232 was used which worked perfectly for simply sending and receiving data. The PCB will use the MAX3243 
which presents 5 input lines and three RS232 outputs. Thus the PCB will support On/Off signalling, hardware polling for network availability 
and Ring Indicators. Furthermore the Network Avaliability and Ring Indicator pins will be tied to external interrupts on the Feather. 

## Testing Files Descriptions

### AT\_Hello\_World 
This file will initialize the modem and send a verification message if the device is properly wired
The script will then read the signal modems current signal strength on a scale of 0 to 5
Finally the script will send the message "Hello World" to the ROCKBLOCK+

### RS232\_Interface\_Test
This file simply write the '@' character to the serial buffer and was used to verify that the MAX232 chip was configured properly


