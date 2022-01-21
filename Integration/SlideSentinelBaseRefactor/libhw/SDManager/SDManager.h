#include <SdFat.h>

class SDManager{
    public:
        SDManager(int csPin, int spiSpeed);

        /*Check if the SD card is active*/
        bool checkSD();

        /*Initialize the SD card*/
        bool initSD();

        // Log the rover diagnostics to the SD card
        bool logRoverDiagnostics();

        // Log the base diagnostics to the SD card
        bool logBaseDiagnostics();

        // Log the data from the rovers to the SD card
        bool logData();
    
    private:
        SdFat sd;

        /*Returns the current error state of the card*/
        int getErrorCode();
       
        // Log any generic data to the SD card
        bool log();

        
        int csPin;    // CS Pin where the SD card is attached
        int spiSpeed; // SPI clock speed
};