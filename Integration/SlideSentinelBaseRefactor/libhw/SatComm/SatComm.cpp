///////////////////////////////////////////////////////////////////////////////
///
/// @file		SDManager.cpp
/// @brief		File containing Satellite Data communication implementations
/// @author		Will Richards
/// @date		2022
/// @copyright	GNU General Public License v3.0
///
///////////////////////////////////////////////////////////////////////////////
#include "SatComm.h"

SatComm::SatComm(){}

SatComm::~SatComm(){
    if(m_modem != nullptr)
        delete m_modem;
}

/**
 * Initialize the SatComm modem
 */ 
bool SatComm::initSatComm(){
    if(SatCommSerial == nullptr){
        Serial.println("[SatComm] Serial was not assigned!");
        return false;
    }

    // Create a new modem
    m_modem = new IridiumSBD(*SatCommSerial, -1, RING_PIN);

    pinMode(RING_PIN, INPUT);       // Allow us to read from the RING pin
    pinMode(NET_AV_PIN, INPUT);     // Allow us to read the network availability 

    // Initialize the serial communication
    SatCommSerial->begin(IRIDIUM_BAUD);

    // Set the RX and TX pins to be in sercom mode
    pinPeripheral(IRIDIUM_RX, PIO_SERCOM);
    pinPeripheral(IRIDIUM_TX, PIO_SERCOM);

    if(SatCommSerial){
        Serial.println("[SatComm] Serial Interface has been started!");
    }

    delay(50);

    Serial.println("[SatComm] Initializing Driver... this may take up to 2 minutes");

    // Start the modem driver
    int result; 
    
    result = m_modem->begin();
    if(result != ISBD_SUCCESS || result != ISBD_ALREADY_AWAKE){
        Serial.println("[SatComm] Failed to initialize SatComm! Error: " + initializationErrorCodes[result]);
        return false;
    }

    Serial.println("[SatComm] Successfully initialized SatComm! Getting Firmware Version...");

    // Get the firmware version running on the SatComm
    String firmwareVersion = getFirmwareVersion();
    // If we got no version from the firmware check we failed to initialize
    if(firmwareVersion.length() <= 0) return false;


    Serial.println("[SatComm] Firmware Version: " + firmwareVersion);
    return true;
}

/**
 * Return the status of the signal connection
 */ 
bool SatComm::waitForSignal(){
    int signalQuality = 0;

    // If the board thinks we have a signal it will output HIGH on this pin
    if(digitalRead(NET_AV_PIN)){
        int err = m_modem->getSignalQuality(signalQuality);

        // Did we successfully get a signal strength
        if(err != ISBD_SUCCESS){
            Serial.println("[SatComm] Failed to read signal quality! Error: " + initializationErrorCodes[err]);
            return false;
        }else if(signalQuality > 0){
            Serial.println("[SatComm] Modem acquired a satellite signal! Signal Strength: " + String(signalQuality));
            return true;
        }
    }

    return false;
}

/**
 * Get the current firmware version running on the device
 */ 
String SatComm::getFirmwareVersion(){
    char firmwareVersion[12];
    int retry_count = 0;
    int result;

    Serial.println("[SatComm] Attempting to retrieve firmware version from modem...");

    // Try 5 times to retrieve the firmware version from the device
    do{
        result = m_modem->getFirmwareVersion(firmwareVersion, sizeof(firmwareVersion));
        retry_count++;
    } while(result != ISBD_SUCCESS && retry_count < 5);

    // Check if we failed to get the firmware version
    if(result != ISBD_SUCCESS){
        Serial.println("[SatComm] Failed to get firmware version from SatComm! Error: " + initializationErrorCodes[result]);
        return "";
    }
    else{
        return String(firmwareVersion);
    }
}

/**
 * Get the current time as a string
 */ 
String SatComm::getCurrentTimeString(){
    time_t time = mktime(&currentTime);
    return String(asctime(gmtime(&time)));
}

/**
 * Get the current firmware version running on the device
 */ 
void SatComm::updateSystemTime(){
    
    int retry_count = 0;
    int result;

    Serial.println("[SatComm] Attempting to retrieve system time from modem...");

    // Try 5 times to retrieve the firmware version from the device
    do{
        result = m_modem->getSystemTime(currentTime);
        retry_count++;
    } while(result != ISBD_SUCCESS && retry_count < 5);

    // Check if we failed to get the firmware version
    if(result != ISBD_SUCCESS){
        Serial.println("[SatComm] Failed to get system time from SatComm! Error: " + initializationErrorCodes[result]);
    }
    else{
        Serial.print("[SatComm] Retrieved system time from SatComm! Current Time: " + getCurrentTimeString());
    }
}