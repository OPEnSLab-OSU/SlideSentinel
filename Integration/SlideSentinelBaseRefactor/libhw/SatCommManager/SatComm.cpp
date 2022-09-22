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

SatComm::SatComm(){
    m_modem = new IridiumSBD(SatCommSerial, -1, RING_PIN);
    didRing = false;
}

SatComm::~SatComm(){
    if(m_modem != nullptr)
        delete m_modem;
}

/**
 * Initialize the SatComm modem
 */ 
bool SatComm::initSatComm(){
    Serial.println("[SatComm] Initializing Driver...");
    pinMode(RING_PIN, INPUT);       // Allow us to read from the RING pin
    pinMode(NET_AV_PIN, INPUT);     // Allow us to read the network availability

    // Set the RX and TX pins to be in sercom mode
    pinPeripheral(IRIDIUM_RX, PIO_SERCOM);
    pinPeripheral(IRIDIUM_TX, PIO_SERCOM);

    // Initialize the serial communication
    SatCommSerial.begin(IRIDIUM_BAUD);

    // Start the modem driver
    int result; 
    result = m_modem->begin();
    if(result != ISBD_SUCCESS && result != ISBD_ALREADY_AWAKE){
        Serial.println("[SatComm] Failed to initialize SatComm! Error: " + initializationErrorCodes[result]);
        return false;
    }

    // Get the firmware version running on the SatComm
    String firmwareVersion = getFirmwareVersion();

    // If we got no version from the firmware check we failed to initialize
    if(firmwareVersion.length() <= 0) return false;


    Serial.println("[SatComm] Successfully initialized SatComm! Firmware Version: " + firmwareVersion);
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