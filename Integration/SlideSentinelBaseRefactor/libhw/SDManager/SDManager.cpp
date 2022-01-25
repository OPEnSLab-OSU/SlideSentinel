///////////////////////////////////////////////////////////////////////////////
///
/// @file		SDManager.cpp
/// @brief		File containing SDFat wrapper implementation.
/// @author		Will Richards
/// @date		2022
/// @copyright	GNU General Public License v3.0
///
///////////////////////////////////////////////////////////////////////////////

#include "SDManager.h"

SDManager::SDManager(int csPin, int spiSpeed): csPin(csPin),spiSpeed(spiSpeed){}

/**
 * Initialize the SD card, this is seperated from the constructor to give more control
 */ 
bool SDManager::initSD(){

    // Print out an initialization header
    Serial.println("\n-----------------------------SD Manager------------------------------");

    pinMode(csPin, OUTPUT);

    // Attempt to open SPI communication with the SD card and then try to open the root of the SD card
    if(!m_sd.begin(csPin, SD_SCK_HZ(spiSpeed)) || !m_sdRoot.open("/")){
        Serial.println("[SD Manager] Failed to initialize SD Card!");
        return false;
    }

    // Check if the default log folder doesn't exists on the SD card
    if(!m_sd.exists(SD_LOG_FOLDER)){

        Serial.println("[SD Manger] Default log folder was not found, attempt to create one.");
        if(!m_sd.mkdir(SD_LOG_FOLDER)){
            Serial.println("[SD Manager] Failed to create the default log directory!");
            return false;
        }
        else{
            Serial.println("[SD Manager] Default log folder successfully created!");
        }
    }

    // Attempt to create each of the directories we will log the individual rover data to
    if(!createLogDirectories()){
        Serial.println("[SD Manager] Failed to create required logging directories! ");
        return false;
    }

    // Print out that we have succsessfully initialized, print out some generic information about the connected SD card
    Serial.println("[SD Manager] Succsessfully Initialized!");
    Serial.println("[SD Manager] Total Capacity: " + String(getTotalSize()) + "MB");
    Serial.println("[SD Manager] Free Space:     " + String(getFreeSpace()) + "MB");

    // Print out an ending seperator to conclude the SD manager segment of initialization
    Serial.println("\n---------------------------------------------------------------------");

    return true;
}

/**
 * Create the individual directories we will store the rover data in
 */ 
bool SDManager::createLogDirectories(){
    String roverInstance = "";
    for(int roverNum = 0; roverNum < NUM_ROVERS; roverNum++){
        roverInstance = "ROVER" + String(roverNum);
        Serial.println("[SD Manager] Creating directory for : " + roverInstance);
        
        /* Change to SD Root, Open Default Log folder, Create the rover folders*/
        if(!(m_sd.chdir() && m_sd.chdir(SD_LOG_FOLDER) && m_sd.mkdir(roverInstance.c_str()) && m_sd.chdir(roverInstance) &&
             createFile(PROPERTIES_FILE) && createFile(DIAGNOSTIC_FILE) && createFile(DATA_FILE) && createFile(ERROR_FILE))){
            Serial.println("[SD Manager] Failed to create logging files!");
            return false;
        }
    }

    // Finally return to the base logging directory
    return returnToLogDir();
}

/**
 * General purpose logging method to log data from a specified rover
 */ 
bool SDManager::log(const int roverNum, const char* message, const char* file){

    // Rover instance to track data from an individual rover
    String roverFolder = "ROVER" + String(roverNum);
    if(!m_sd.chdir(roverFolder.c_str()) && !writeLine(file, message)){
        Serial.println("[SD Manager] Failed to log to file: " + String(file));
        return false;
    }

    // Return to the original log directory to prepare for new data
    return returnToLogDir();
}

/**
 * Log the diagnostic information about a specific rover
 */ 
bool SDManager::logRoverDiagnostics(int roverNum, Diagnostics diagnostics){

    // Create new document to store the JSON diagnostic as well as the string to deserialize it to
    StaticJsonDocument<JSON_ARRAY_SIZE(7) + JSON_OBJECT_SIZE(1)> doc;
    String diagnosticJSON = "";

    // Write the diagnostic data into the specified document
    diagnostics.write(doc);

    // Convert the JSON document into a string and write it into the diagnostic JSON string
    auto error = deserializeJson(doc, diagnosticJSON);

    // Check if we successfully deserialized the data
    if(error){
        Serial.println("[SD Manager] Failed to deserialize diagnostic data!");
        return false;
    }

    // Finally attempt to log the diagnostic data to the corresponding file
    if(!log(roverNum, diagnosticJSON.c_str(), DIAGNOSTIC_FILE)){
        Serial.println("[SD Manger] Failed to log diagnostic data to file!");
        return false;
    }

    return true;
}

/**
 *  Log some arbitrary rover data to the specified log
 */ 
bool SDManager::logData(int roverNum, char* data){
    if(!log(roverNum, data, DATA_FILE)){
        log(roverNum, "Failed to write data to SD card!", ERROR_FILE);
        return false;
    }
    return true;
}

/**
 * Get the remaining free space on the SD card in megabytes
 */ 
int SDManager::getFreeSpace(){
    int freeBlocks = m_sd.blocksPerCluster() * m_sd.freeClusterCount();

    // Convert the number of blocks into megabytes
    return (freeBlocks * SECTORS_TO_MEGABYTES);
}

/**
 * Get the overall capacity of the SD card
 */ 
int SDManager::getTotalSize(){
    uint32_t cardMegabytes = m_sd.card()->cardCapacity() * SECTORS_TO_MEGABYTES;
    return cardMegabytes;
}

/**
 * Return to the base of the SD logging directory to easily move to a new log
 */ 
bool SDManager::returnToLogDir(){
    
    // Return to the root directory and move into the logging directory to prepare for logging
    if( !(m_sd.chdir() && !m_sd.chdir(SD_LOG_FOLDER))){
        Serial.println("[SD Manager] Failed to open log folder!");
        return false;
    }

    return true;
}

/**
 * Create a new empty file in the current directory with the given name 
 */
bool SDManager::createFile(const char* fileName){
    return (m_file.open(fileName, O_WRONLY | O_CREAT | O_EXCL) && m_file.close());
}

/**
 * Append a line of text to the end of a given file
 */ 
bool SDManager::writeLine(const char* fileName, const char* messsage){

    // Open the for writting and append all writes to the end
    if(!m_file.open(fileName, O_WRONLY | O_APPEND)){
        Serial.println("[SD Manager] Failed to open file!");
        return false;
    }

    // Write the message to the file
    if(m_file.println(messsage) <= 0){
        Serial.println("[SD Manager] Nothing written to file!");
        return false;
    }

    // Attempt to close the file
    if(!m_file.close()){
        Serial.println("[SD Manager] Failed to close file! ");
        return false;
    }
    
    return true;
}

/**
 * Check the current functional status of the SD card, returns true if we are okay false if something is broken
 */ 
bool SDManager::checkSD(){
    return m_sd.card()->errorCode() == 0;
}

/**
 * Get the current error code on the card
 */
int SDManager::getErrorCode(){
    return m_sd.card()->errorCode();
}

