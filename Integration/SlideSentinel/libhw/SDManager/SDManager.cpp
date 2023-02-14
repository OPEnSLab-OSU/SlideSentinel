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

SDManager::SDManager(int csPin): csPin(csPin){ }

/**
 * Initialize the SD card, this is separated from the constructor to give more control
 */ 
bool SDManager::initSD(){

    // Set the output mode on the pin
    pinMode(csPin, OUTPUT); 

    // Attempt to open SPI communication with the SD card and then try to open the root of the SD card
    if(!m_sd.begin(csPin, SPI_HALF_SPEED)){
        Serial.println("[SD Manager] Failed to initialize SD Card!");
        return false;
    }
    else{
        Serial.println("[SD Manager] SD Card Initialized!");
    }

    if(!m_sdRoot.open("/")){
        Serial.println("[SD Manager] Failed to open root directory!");
        return false;
    }
    else{
        Serial.println("[SD Manager] Successfully opened root directory!");
    }

    // Check if the default log folder doesn't exists on the SD card
    if(!m_sd.exists(SD_LOG_FOLDER)){

        Serial.println("[SD Manager] Default log folder was not found, attempt to create one.");
        if(!m_sd.mkdir(SD_LOG_FOLDER)){
            Serial.println("[SD Manager] Failed to create the default log directory!");
            return false;
        }
        else{
            Serial.println("[SD Manager] Default log folder successfully created!");
        }
    }

    // Print out that we have succsessfully initialized, print out some generic information about the connected SD card
    Serial.println("[SD Manager] Succsessfully Initialized!");
    Serial.println("[SD Manager] Total Capacity: " + String(getTotalSize()) + "MB");
    Serial.println("[SD Manager] Free Space:     " + String(getFreeSpace()) + "MB");

    return true;
}

/**
 * Create the individual directories we will store the rover data in
 */ 
bool SDManager::createLogDirectory(int roverAddress){
    String roverInstance = "";
    String logFolder = "/" + String(SD_LOG_FOLDER);
   
    roverInstance = "ROVER" + String(roverAddress);
    Serial.println("[SD Manager] Creating directory for : " + roverInstance);

    if(m_sd.chdir(logFolder.c_str(), true), true){
        Serial.println("[SD Manager] Successfully opened log folder!");
        

        // Make and open the log folder
        m_sd.mkdir(roverInstance.c_str());
        m_sd.chdir(roverInstance.c_str(), true);

        // Loop over all the folders in the current directory to increment the number to log else where
        while(m_sd.exists(String(logCycle).c_str())){
            logCycle++;
        }

        m_sd.mkdir(String(logCycle).c_str());
        m_sd.chdir(String(logCycle).c_str(), true);

        createFile(PROPERTIES_FILE);
        createFile(DIAGNOSTIC_FILE);
        createFile(DATA_FILE);
        createFile(ERROR_FILE);

        Serial.println("[SD Manager] Log files created successfully!");
        
    }
    else{
        Serial.println("[SD Manager] Failed to open log folder!");
    }
    

    // Finally return to the base logging directory
    return returnToLogDir();
}

/**
 * General purpose logging method to log data from a specified rover
 */ 
bool SDManager::log(const int roverNum, const char* message, const char* file){

    // Rover instance to track data from an individual rover
    String roverFolder = "ROVER" + String(roverNum) + "/" + String(logCycle);

    // If the log directory doesn't already exist then create a new one based on the rover address
    if(!directoryExists(roverFolder.c_str()))
        createLogDirectory(roverNum);
    
    if(!m_sd.chdir(roverFolder.c_str(), true)){
        Serial.println("[SD Manager] Failed to open log directory!");
        return false;
    }

    // Log information to the correct log directory
    if(!writeLine(file, message)){
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
    StaticJsonDocument<1024> doc;
    String diagnosticJSON = "";

    // Write the diagnostic data into the specified document
    diagnostics.write(doc);

    // Convert the JSON document into a string and write it into the diagnostic JSON string
    auto error = serializeJson(doc, diagnosticJSON);

    // Check if we successfully deserialized the data
    if(error){
        Serial.println("[SD Manager] Failed to deserialize diagnostic data!");
        return false;
    }

    // Finally attempt to log the diagnostic data to the corresponding file
    if(!log(roverNum, diagnosticJSON.c_str(), DIAGNOSTIC_FILE)){
        Serial.println("[SD Manager] Failed to log diagnostic data to file!");
        return false;
    }

    return true;
}

/**
 *  Log some arbitrary JSON packet to the SD Card
 */ 
bool SDManager::logData(int roverNum, JsonObject json){
    String jsonString = "";
    serializeJson(json, jsonString);
    //Serial.println(jsonString);
    if(!log(roverNum, jsonString.c_str(), DATA_FILE)){
        log(roverNum, "Failed to write data to SD card!", ERROR_FILE);
        return false;
    }

    Serial.println("[SD Manager] Successfully logged to SD Card!");
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
 * Check if the given directory exists
 */ 
bool SDManager::directoryExists(const char* dir){
    return m_sd.exists(dir);
}

/**
 * Return to the base of the SD logging directory to easily move to a new log
 */ 
bool SDManager::returnToLogDir(){
    
    String logFolder = "/" + String(SD_LOG_FOLDER);
    // Return to the root directory and move into the logging directory to prepare for logging
    if(!m_sd.chdir(logFolder.c_str(), true)){
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
bool SDManager::writeLine(const char* fileName, const char* message){

    //Serial.println(message);
    // Open the for writting and append all writes to the end
    if(!m_file.open(fileName, O_WRONLY | O_APPEND)){
        Serial.println("[SD Manager] Failed to open file!");
        return false;
    }

    // Write the message to the file
    if(m_file.println(message) <= 0){
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

