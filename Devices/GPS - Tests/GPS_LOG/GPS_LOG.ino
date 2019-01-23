// ================================================================
// ===              INCLUDE CONFIGURATION FILE                  ===
// ===    INCLUDE DECLARATIONS, STRUCTS, AND FUNCTIONS FROM     ===
// ===            OTHER FILES AS SET IN CONFIG.H                ===
// ================================================================

// ********************** PROGRAM DEXCRIPTION *********************
// The LOOM_HUB acts as the data manager for the Slide Sentinel project
// It receives and handles uploads of all nmea and accelerometer data,
//  it does not handle any data going to the rovers, but will take
//  data from all nodes, determine the "best" nmea data and upload it
//  in a dump to cellular periodically. Transmission is handled by 
//  a second microprocessor on board the hub which is dedicated to sending 
//  complete RTK transmissions.
// ****************************************************************


// Config has to be first has it hold all user specified options
#include "config.h"

// Preamble includes any relevant subroutine files based
// on options specified in the above config
#include "loom_preamble.h"

#include <Arduino.h>   // required before wiring_private.h
#include "wiring_private.h" // pinPeripheral() function


#define BAUD 57600    // reading and writing occurs at 
#define DEBUG 1       // turn on debug mode
#define DEBUG_SD 1
#define CELLULAR 0



char nmea_data[1024];
bool is_read;

// RX pin 6, TX pin A5, configuring for rover to GPS UART
Uart Serial3 (&sercom5, 6, A5, SERCOM_RX_PAD_2, UART_TX_PAD_0);
void SERCOM5_Handler()
{
  Serial3.IrqHandler();
}

void busyWait(){
  while(1);
}

// ================================================================
// ===                           SETUP                          ===
// ================================================================
void setup()
{
  // LOOM_begin calls any relevant (based on config) LOOM device setup functions
  Loom_begin();
  //setup code here, to run once:
  Serial.begin(115200);         //Opens the main serial port to communicate with the computer
  Serial3_setup();
  digitalWrite(6, INPUT_PULLUP);
//  attachInterrupt(digitalPinToInterrupt(13), busyWait) // infinite loop to disconnect devi
  //Any custom setup code
}

// ================================================================
// ===                        MAIN LOOP                         ===
// ================================================================
void loop()
{
  int i = 0; 
  memset(nmea_data, '\0', 1024);
  while(Serial3.available()){
    nmea_data[i++] = Serial3.read();
    if(i >= 1022) break;
  }
  if(strlen(nmea_data)){
    sd_save_elem_nodelim ("GPS.csv", nmea_data);
  }
  
} // End loop section


bool sd_save_elem_nodelim(char *file, char* data)
{  
  #if is_lora == 1
    digitalWrite(8, HIGH);  // if using LoRa
  #endif
  SD.begin(chipSelect); // It seems that SD card may become 'unsetup' sometimes, so re-setup
  
  sdFile = SD.open(file, FILE_WRITE);

  if (sdFile) {
    LOOM_DEBUG_Println4("Saving ", data, " to SD file: ", file);
    sdFile.print(data);
  } else {
    LOOM_DEBUG_Println2("Error opening: ", file);
    return false;
  }
  sdFile.close();
  return true;
}

void Serial3_setup() {
  Serial3.begin(115200);          //tx from rover to pin 6
  digitalWrite(6,INPUT_PULLUP);
  // Assign pins 6 & A5 SERCOM functionality, internal function
  pinPeripheral(6, PIO_SERCOM);  //Private functions for serial communication
  pinPeripheral(A5, PIO_SERCOM_ALT);
}
