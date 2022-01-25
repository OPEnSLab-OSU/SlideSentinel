#include <SdFat.h>

#include "Diagnostics.h"
#include "config.h"

// Name of the folder we will log data to on the base
#define SD_LOG_FOLDER "rover"

#define PROPERTIES_FILE "props.json"
#define DIAGNOSTIC_FILE "diag.json"
#define DATA_FILE       "data.json"
#define ERROR_FILE      "error.json"

#define SECTORS_TO_MEGABYTES 0.000512

class SDManager{
    public:
        SDManager(int csPin, int spiSpeed);

        /*Check if the SD card is active*/
        bool checkSD();

        /*Initialize the SD card*/
        bool initSD();

        // Log the rover diagnostics to the SD card
        bool logRoverDiagnostics(int roverNum, Diagnostics diagnostics);

        // Log the base diagnostics to the SD card
        //bool logBaseDiagnostics();

        // Log the data from the rovers to the SD card
        bool logData(int roverNum, char* data);

        // Get the current amount of free space on the SD card
        int getFreeSpace();

        // Get the total overall size of the SD card
        int getTotalSize();
    
    private:
        SdFat m_sd;             // SD Card interface  
        SdFile m_sdRoot;        // Root of the SD card
        SdFile m_file;          // General purpose file object for creating the required file

        int csPin;              // CS Pin where the SD card is attached
        int spiSpeed;           // SPI clock speed

        /* Returns the current error state of the card */
        int getErrorCode(); 
       
        /* Log any generic data to the SD card */
        bool log(const int roverNum, const char* message, const char* file);

        /* Creates the required number of log directories*/
        bool createLogDirectories();

        /* Create a file on the SD card with the given name*/
        bool createFile(const char* fileName);

        /* Write a line to the end of the given file */
        bool writeLine(const char* fileName, const char* message);

        /* Return to the original logging directory after having altered some log*/
        bool returnToLogDir();

        
       
};