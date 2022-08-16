#ifndef _Rover_H_
#define _Rover_H_
#include "MAX4820.h"
#include "SN74LVC2G53.h"
#include "pcb_2.0.0.h"
#include "network_config_2.0.0.h"

#include <HardwareSerial.h>
#include <RH_Serial.h>
#include <RHReliableDatagram.h>

#include <ArduinoJson.h>
#include <GNSSController.h>
#include "SPI.h"
#include <RTClib.h>
/**
 * @brief The Rover class is responsible for controlling all subparts of the rover.
 *  
 */

class Rover {

public:
    Rover(int radioType);
    Rover();
    bool listen();

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
    
    /* Powers radio via relay, called in wake cycle in main, and waits 20 seconds to ensure proper power on */
    void wake();

    /* Called after radio has been enabled in the HANDSHAKE section. Attempts to make contact with base. If successful:
    transition to polling mode, if not: power down and set short wake timer if base is busy. */
    //bool request();

    /* Serialize and send JSON packet to Base*/
    bool transmit();

    /* Package data for transmit */
    void packageData(DataType packType);

    /* Send a test message */
    void sendManualMsg(char* msg);

    /* Tells the max4820 to disable the radio relay. */
    void powerDownRadio();

    /* Tells the max4820 to enable the radio relay. */
    void powerRadio();

    /* Tells the max4820 to enable the GNSS relay. */
    void powerGNSS();

    /* Tells the max4820 to disable the GNSS relay. */
    void powerDownGNSS();

/***ALARM FUNCTIONS******************************************************************************************************************************************/
  
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

    /* Initialize RadioHead objects */
    void initRadio();

    /* initialize rtc */
    void initRTC();

    /* Debug function for RTC to print out time*/
    void debugRTCPrint();

    /* prints time from real time clock*/
    void printRTCTime();

    /* Attach RTC interrupt to device */
    void attachAlarmInterrupt();

    /* Sleep and wake from interrupt*/
    void toSleep();

    void printRTCTime_Ben();
    void timeDelay();
    byte bcdSecond(RTC_DS3231);
    void rtc_alarm();


   

    /* RTC Final Functions */



    
private:
    RoverInfo m_rovInfo;            //Rover info that is sent over during handshake, like rover ID
    MAX4820 m_max4820;              //Relay driver, used to power on relays controlling GNSS/Radio
    SN74LVC2G53 m_multiplex;        //Multiplexer used for redirecting information from radio rx to GNSS and radio rx to Feather
    HardwareSerial &m_serial;       //Reference to a serial interface object
    RH_Serial m_RHSerialDriver;             //Driver class for radio communication. Uses serial pins for feather.
    RHReliableDatagram m_RHManager;         //RadioHead communication manager class
    RTC_DS3231 m_RTC;               //Real time clock object
    DateTime nowDT;
    byte prSec = 0;
    GNSSController m_gnss;   //gnss controller that handles data incoming into the feather from piksi

    /*  A message consists of an: ID, TYPE, MSG
        The definitions are as such:
            ID: ID of the rover sending the message
            TYPE: Message type, such as REQUEST, UPLOAD
            MSG: data upload, eg: "152.21312,12.12312, etc"
     */
    DynamicJsonDocument m_RHMessage;    //document that will contain all information sent to the base


    
};

#endif  _Rover_H_
