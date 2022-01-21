#include "SDManager.h"

SDManager::SDManager(int csPin, int spiSpeed): csPin(csPin),spiSpeed(spiSpeed){}

/**
 * Initialize the SD card, this is seperated from the constructor to give more control
 */ 
bool SDManager::initSD(){
    pinMode(csPin, OUTPUT);
    return sd.begin(csPin, SD_SCK_HZ(spiSpeed));
}

/**
 * Check the current functional status of the SD card, returns true if we are okay false if something is broken
 */ 
bool SDManager::checkSD(){
    return sd.card()->errorCode() == 0;
}

/**
 * Get the current error code on the card
 */
int SDManager::getErrorCode(){
    return sd.card()->errorCode();
}

