#ifndef _Rover_H_
#define _Rover_H_
#include "MAX4820.h"
#include "MAX3243.h"
#include "SN74LVC2G53.h"
#include "rover_pcb_config.h"
#include "rover_config.h"
#include "rover_network_config.h"
#include "GNSSController.h"
#include "RadioManager.h"
#include "SPI.h"
#include "Diagnostics.h"

#include <HardwareSerial.h>
#include <RH_Serial.h>
#include <RHReliableDatagram.h>
#include <ArduinoJson.h>
#include <RTClib.h>

/**
 * @brief The Rover class is responsible for controlling all subparts of the rover.
 *  
 */

class Rover {

public:
    /* Data Struct for rover info that gets sent to base. */
    struct RoverInfo {
        /* @var id ID used by RadioHead library, sent to Base to determine which rover is contacting it.*/
        int id;
        /* @var radioBaud Baud rate used by Freewave radio, should always be 115200 unless otherwise specified.*/
        int radioBaud;
        /* @var init_retries Used by RadioHead to determine how many times to resend messages.*/
        int init_retries;
        /* @var timeoute Time waited until a message is considered failed.*/
        int timeout;
        /* @var serverAddr Server address that the rover sends a RadioHead message to. This should be 0 unless multiple bases in the same area.*/
        int serverAddr;
    };

    /* State of all relays/timers/multiplexer/etc */
    struct RoverDiagnostics {
        
    };

    /* Use in the setMux() function */
    enum MuxFormat {
        RadioToFeather = 0,
        RadioToGNSS = 1
    };

    /* 
        Different Ways to Package Data for transmit
        REQUEST - Notify base that rover is awaiting instructions
        UPLOAD - Upload polled data to base
        ALERT - High priority message such as accelerometer trip
    */
    enum DataType{
        REQUEST,
        UPLOAD,
        ALERT
    };
    

    /* Construct a new rover passing in a serial interface*/
    Rover(Uart& ser);

    /* Powers radio via relay, called in wake cycle in main, and waits 20 seconds to ensure proper power on */
    void wake();

    /* Set the RTC Time*/
    void setRTCTime();

    /* Serialize and send JSON packet to Base*/
    bool transmit();

    /* Wait temporarily and receive a message*/
    bool waitAndReceive(int milliseconds = INIT_TIMEOUT);

    /* Returns most recent message type*/
    String getMessageType();

    /* Returns most recent message body*/
    String getMessageBody();

    /* Package data for transmit */
    void packageData(DataType packType);

    /* Manual package data for transmit */
    void packageData(String type, String message);

    /* Tells the max4820 to disable the radio relay. */
    void powerDownRadio();

    /* Tells the max4820 to enable the radio relay. */
    void powerRadio();

    /* Tells the max4820 to enable the GNSS relay. */
    void powerGNSS();

    /* Tells the max4820 to disable the GNSS relay. */
    void powerDownGNSS();

    /**************** DEBUG INFORMATION ****************/

    /* Waits for input over serial to output debug information about the base */
    void debugInformation();

    /* Print the current diagnostic information about the base station */
    void printDiagnostics();

/*** ALARM FUNCTIONS ******************************************************************************************************************************************/
  
    /** Schedule alarm for the RTK process when base and rover are talking to each other 
     * 
     * @param alarmMode The alarm type. Typically  DS3231_A1_Minute is used, triggering every hour when minutes and seconds match.
    */
    void scheduleRTKAlarm(Ds3231Alarm1Mode alarmMode = DS3231_A1_Minute);

    /** Schedule alarm for sleep alarm when rover is asleep 
     * 
     * @param alarmMode The alarm type. Typically  DS3231_A1_Minute is used, triggering every hour when minutes and seconds match.
    */
    void scheduleSleepAlarm(Ds3231Alarm1Mode alarmMode = DS3231_A1_Minute);

    /** Versatile alarm function for custom sleep times
     * 
     * @param sec       Time in seconds that an alarm will be scheduled for in the future 
     * @param alarmMode The alarm type. Typically  DS3231_A1_Minute is used, triggering every hour when minutes and seconds match.

    */
    void scheduleAlarm(int sec, Ds3231Alarm1Mode alarmMode = DS3231_A1_Minute); 

    /** Sets the mutliplexer to Radio->Feather or Radio->GNSS depending on success of Base contact. Ensure
     *  alarm is set no longer than 59 minutes in the future. 
     * 
     * @param format    The route that the multiplexer will use
    */
    void setMux(MuxFormat format);

    /* Initialize modules that the base uses*/
    bool initRover();

    /* initialize rtc */
    void initRTC();

    /* prints time from real time clock*/
    void printRTCTime();

    /* Attach RTC interrupt to device */
    void attachAlarmInterrupt();

    /* Sleep and wake from interrupt*/
    void toSleep();

/*** FEATHER TIMER FUNCTIONS ******************************************************************************************************************************************/

    /*track start time*/
    void startFeatherTimer();

    /** Sets the timer duration
     * 
     * @param milliseconds The duration in milliseconds the timer will be set to
    */
    void setFeatherTimerLength(int milliseconds);

    /** Returns true if the feather timer duration has exceeded the set timer.
     *  Note: This timer expects the timer length to be set prior to calling this.
     *  TDL: doxygen return format?
    */
    bool isFeatherTimerDone();

/*** DEBUG FUNCTIONS ******************************************************************************************************************************************/

    void printRTCTime_Ben();
    void timeDelay();
    byte bcdSecond(RTC_DS3231);
    void rtc_alarm();

    /**  Sets the rs232 translator chip (MAX3243) to be enable or disabled. Note, this jumpers MUST be in the Z9-C position for this to work.
     *  
    *   @param enable Whether the chip will be enabled or disabled
    */
    void setRS232(bool enable);


    /* Poll the GNSS Controller for a fix */
    bool poll();

    /**
     * QOL Feature waits a set timeout for the serial to be opened before just continuing as normal
     * @param timeout Maximum time to wait before continuing 
    */
    void waitForSerial(int timeout = 10000);


    
private:
    RoverInfo m_rovInfo;            //Rover info that is sent over during handshake, like rover ID
    MAX4820 m_max4820;              //Relay driver, used to power on relays controlling GNSS/Radio
    MAX3243 m_max3243;              //Translator chip for rs-232 communication
    SN74LVC2G53 m_multiplex;        //Multiplexer used for redirecting information from radio rx to GNSS and radio rx to Feather
    RadioManager m_RadioManager;         //In charge of all radio communications
    RTC_DS3231 m_RTC;               //Real time clock object
    DateTime nowDT;
    byte prSec = 0;

    

    /* RTK Poll Variables*/
    unsigned long startTime;
    unsigned long featherTimerLength;
    DynamicJsonDocument m_JSONData;  
    GNSSController m_gnss;

    Diagnostics  m_roverDiagnostics;         // Diagnostics  to track debug information about the base

    /*RTK Variable*/
    String rtkMsg;

    /*  A message consists of an: ID, TYPE, MSG
        The definitions are as such:
            ID: ID of the rover sending the message
            TYPE: Message type, such as REQUEST, UPLOAD
            MSG: data upload, eg: "152.21312,12.12312, etc"
     */
  


    
};

#endif  _Rover_H_
