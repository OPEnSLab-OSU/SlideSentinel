///////////////////////////////////////////////////////////////////////////////
///
/// @file		SatComm.cpp
/// @brief		File containing Satellite Data communication implementations
/// @author		Will Richards
/// @date		2022
/// @copyright	GNU General Public License v3.0
///
///////////////////////////////////////////////////////////////////////////////
#include "SatComm.h"
#include "base_pcb_config.h"

SatComm::SatComm(){}

SatComm::~SatComm(){
    if(m_modem != nullptr)
        delete m_modem;
}

/**
 * Initialize the SatComm modem
 */ 
bool SatComm::initSatComm(){
    pinMode(SATCOMM_ONOFF, HIGH);

    if(SatCommSerial == nullptr){
        Serial.println("[SatComm] Serial was not assigned!");
        return false;
    }

    // Create a new modem
    m_modem = new IridiumSBD(*(SatCommSerial), -1, RING_PIN);

    pinMode(RING_PIN, INPUT);       // Allow us to read from the RING pin
    pinMode(NET_AV_PIN, INPUT);     // Allow us to read the network availability 

    // Initialize the serial communication
    SatCommSerial->begin(IRIDIUM_BAUD);

    // Set the RX and TX pins to be in sercom mode
    pinPeripheral(IRIDIUM_RX, PIO_SERCOM);
    pinPeripheral(IRIDIUM_TX, PIO_SERCOM);

    delay(50);

    Serial.println("[SatComm] Initializing Driver... this may take up to 2 minutes");

    // Start the modem driver
    int result; 
    
    result = m_modem->begin();
    if(result != 0){
        Serial.println("[SatComm] Failed to initialize SatComm! Error: " + initializationErrorCodes[result]);
        return false;
    }

    Serial.println("[SatComm] Successfully initialized SatComm! Getting Firmware Version...");

    // Get the firmware version running on the SatComm
    String firmwareVersion = getFirmwareVersion();

    // If we got no version from the firmware check we failed to initialize
    if(firmwareVersion.length() <= 0) return false;

    Serial.println("[SatComm] Firmware Version: " + firmwareVersion);

    // Wait for a signal to see if we can get a satellite connection
    if(!waitForSignal()) return false;

    // Attempt to update the system time
    if(!updateSystemTime()) return false;

    Serial.println("[SatComm] Initialization Complete!");
    return true;
}

/**
 * Waits up to 15 seconds for a signal to be acquired
 */ 
bool SatComm::waitForSignal(){
    int signalQuality = 0;
    Serial.println("[SatComm] Waiting for signal, this may take up to 15 seconds...");
    int startTime = millis();

    // If the board thinks we have a signal it will output HIGH on this pin
    while(millis() < startTime+10000){
        if(digitalRead(NET_AV_PIN)){
            int err = m_modem->getSignalQuality(signalQuality);

            // Did we successfully get a signal strength
            if(err != 0){
                Serial.println("[SatComm] Failed to read signal quality! Error: " + initializationErrorCodes[err]);
                return false;
            }else if(signalQuality > 0){
                Serial.println("[SatComm] Modem acquired a satellite signal! Signal Strength: " + String(signalQuality));
                return true;
            }
        }
    }
    Serial.println("[SatComm] Unable to locate signal");
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
 * Transmit our binary data to the satellite 
 */
bool SatComm::transmit(JsonObject json){
    if(waitForSignal()){
        // Minify the JSON and convert it to uint8_t array
        String minified = minifyJson(json);
        uint8_t data[50];
        minified.getBytes(data, 50);
        size_t packetSize = 50;

        // Upload the data!
        int result = m_modem->sendSBDBinary(data, packetSize);

        if(result != ISBD_SUCCESS){
            Serial.println("[SatComm] Failed to transmit data! Error: " + initializationErrorCodes[result]);
            return false;
        }
        else{
            Serial.println("[SatComm] Successfully transmitted data!");
            return true;
        }
    }

    Serial.println("[SatComm] Unable to acquire signal to transmit data");
    return false;

}

/**
 * Get the current firmware version running on the device
 */ 
bool SatComm::updateSystemTime(){
    
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
        return false;
    }
    else{
        Serial.print("[SatComm] Retrieved system time from SatComm! Current Time: " + getCurrentTimeString());
        return true;
    }
}

/**
 * Create a minified 50 character string to send over SatComm
 */
String SatComm::minifyJson(JsonObject json){
    String minifiedString = "";

    // Loop over the first 10 of the latitude cause we will just remove the period later
    for(int i = 0; i < 10; i++){
        minifiedString += json["Latitude"].as<String>()[i];
    }
    minifiedString += ",";
    
    // Start at 1 to skip the negative sign
    for(int i = 1; i < 11; i++){
        minifiedString += json["Longitude"].as<String>()[i];
    }
    minifiedString += ",";

    // Replace the decimal place with nothing
    minifiedString.replace(".", "");

    // If there are less than 0 satellites we want to 0 pad so the packet is always the same length
    if(json["Satellites"].as<int>() < 10)
        minifiedString += "0";
    minifiedString += String(json["Satellites"].as<int>());
    minifiedString += ",";

    // Weeks since epoch 
    minifiedString += String(json["Week"].as<int>());
    minifiedString += ",";

    // GPS Time
    minifiedString += String(json["Time"].as<int>());
    minifiedString += ",";

    // H Accuracy
    for(int i = 0; i < 5; i++){
        minifiedString += json["H Accuracy"].as<String>()[i];
    }
    minifiedString += ",";

    // V Accuracy
    for(int i = 0; i < 4; i++){
        minifiedString += json["V Accuracy"].as<String>()[i];
    }
}