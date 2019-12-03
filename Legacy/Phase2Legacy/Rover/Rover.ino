#include <Arduino.h>  // required before wiring_private.h
#include "LowPower.h" // from sparkfun low power library found here https://github.com/rocketscream/Low-Power
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <Adafruit_MMA8451.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_SleepyDog.h>
#include <EnableInterrupt.h>
#include "wiring_private.h"
#include "SlideS_parser.h"
#include <RTClibExtended.h>


#define DEBUG 1 // allow printing to serial monitor,
#define DEBUG_SD 1

#define RTC_WAKE_PERIOD 1 // Interval to wake and take sample in Min, reset alarm based on this period (Bo - 5 min), 15 min
#define STANDARD_WAKE 20  // Length of time to take measurements under periodic wake condition
#define ALERT_WAKE 20     // Length of time to take measurements under acceleration wake condition

// Debugging flags
#define TOGGLE_SLEEP true
#define TOGGLE_NIGHT_SLEEP false
#define TOGGLE_WDT false
#define TOGGLE_TX true

#define RTC_MODE 1 // enable RTC interrupts
#define SD_WRITE 1 // enable SD card logging
#define NODE_NUM 0 // ID for node
#define GPS_BUFFER_LEN 500
#define BAUD 115200 // reading and writing occurs at

#define GPS_EN_PIN A2      // Attach to RESET on relay
#define GPS_DISABLE_PIN A4 // Attach to SET on relay
#define ALERT_WAKE_PIN A3  // Attach A3 to int1 on accelerometer
#define RTC_WAKE_PIN 5     // Attach to SQW pin on RTC
#define SERIAL2_RX 12      // Rx pin for first serial port
#define SERIAL2_TX 11      // Tx pin for first serial port
#define BATTERYPIN A1
#define chipSelect 4

// Function Prototypes
void mmaCSVRead(Adafruit_MMA8451 device, String &to_fill, int count);
void configInterrupts(Adafruit_MMA8451 device);
void mmaPrintIntSRC(uint8_t dataRead);
void mmaSetupSlideSentinel();

// Object instantiation
Adafruit_MMA8451 mma = Adafruit_MMA8451();
RTC_DS3231 RTC_DS;

// Hardware Serial Port 2
// RX pin 12, TX pin 11, configuring for rover UART
Uart Serial2(&sercom1, SERIAL2_RX, SERIAL2_TX, SERCOM_RX_PAD_3, UART_TX_PAD_0);
void SERCOM1_Handler()
{
    Serial2.IrqHandler();
}

// Global variables
sensors_event_t event;
String accel_data;
unsigned long int timer, count;
String RTC_monthString, RTC_dayString, RTC_hrString, RTC_minString, RTC_timeString = "", stringTransmit = "";

const char *CurrentWakeGPS = "CRWKG";
const char *AllLogs = "ALL.TXT";

// Declare RTC/Accelerometer specific variables
volatile bool alertFlag = false; // Flag for an accelerometer transient event
volatile bool TimerFlag = false; // Flag which resets the sleep timer and prolongs the device from going to sleep
volatile bool RTCFlag = false;   // Flag which toggles when the RTC wakes up
volatile bool stateSent = false; // Flag which is asserted when the processor is asleep and wakes

//used to put the Rover back to sleep if it is dark, unless the rover was woken up via accelerometer interrupt
volatile bool accelAwake = false;
volatile bool nightFlag = false;

volatile int HR = 8;        // Hr of the day we want alarm to go off
volatile int MIN = 0;       // Min of each hour we want alarm to go off
volatile int awakeFor = 20; // number of seconds to stay awake and take measurements for

/**********************************************************************************************
   wakeUp_()
   Description: function that takes place after device wakes up
   See: Interrupt Service Routine (ISR)
   wakeUp_alert
   - sets SampleFlag and alertFlag TRUE;
   wakeUp_RTC
   - sets SampleFlag TRUE upon typical wakeup
**********************************************************************************************/

void wakeUp_alert()
{
    detachInterrupt(digitalPinToInterrupt(ALERT_WAKE_PIN));
    TimerFlag = true;
    alertFlag = true;
}

void wakeUp_RTC()
{
    detachInterrupt(digitalPinToInterrupt(RTC_WAKE_PIN));
    TimerFlag = true;
    RTCFlag = true;
}

void setup()
{
    Serial.begin(115200);
    while(!Serial);

    setup_sd();

    //Setup UART
    Serial2Setup();        // GPS UART
    Serial1.begin(115200); // Radio UART

    //configureAccelerometer
    mmaSetupSlideSentinel();
    configInterrupts(mma);
    digitalWrite(ALERT_WAKE_PIN, INPUT_PULLUP); // LOW interrupt

    attachInterrupt(digitalPinToInterrupt(ALERT_WAKE_PIN), wakeUp_alert, LOW);
    //attachInterrupt(digitalPinToInterrupt(RTC_WAKE_PIN), wakeUp_RTC, FALLING);

    Serial.println("Interrupt attached");
    mma.readRegister8(MMA8451_REG_TRANSIENT_SRC); //When this register is read it clears the interrupt for the transient detection

    //configure RTC
    Serial.println("Setting up RTC");
    initializeRTC();
    Serial.print("Alarm set to go off every ");
    Serial.print(RTC_WAKE_PERIOD);
    Serial.println(" min from program time");

    //initialize globals
    timer = millis();
    count = 0;
    //initialize pins
    initializePins();
#if TOGGLE_WDT
    Watchdog.enable(20000);
#endif
    gps_on();
}

void loop()
{
    if (Serial.available())
        while (1)
            ;
    timerFlagCheck(); // set the timer each time an interrupt is triggered, prolong the wake period
    RTCFlagCheck();   // reset RTC interrupts
    alertFlagCheck(); // reset accelerometer interrupts,
    readNMEA();       // read from serial port if available
    tryStandby();
}

/* Just a printing function */
void mmaCSVRead(Adafruit_MMA8451 device, String &to_fill, unsigned long int count)
{
    device.getEvent(&event);
    to_fill += count;
    to_fill += ',';
    to_fill = device.x;
    to_fill += ',';
    to_fill += device.y;
    to_fill += ',';
    to_fill += device.z;
}

/* Setup for mma use with Slide Sentinel, other use cases will be pretty similar */
void mmaSetupSlideSentinel()
{
    Serial.println("setting up mma");
    if (!mma.begin())
    {
        Serial.println("+-");
        while (1)
            ;
    }

    Serial.println("MMA8451 found!");
    mma.setRange(MMA8451_RANGE_2_G);
    mma.setDataRate(MMA8451_DATARATE_6_25HZ);
    Serial.print("Range = ");
    Serial.print(2 << mma.getRange());
    Serial.println("G");
}

void initializePins()
{
    pinMode(RTC_WAKE_PIN, INPUT_PULLUP);   //active low interrupts
    pinMode(ALERT_WAKE_PIN, INPUT_PULLUP); //active low interrupts
    pinMode(GPS_EN_PIN, OUTPUT);
    digitalWrite(GPS_EN_PIN, LOW);
    pinMode(GPS_DISABLE_PIN, OUTPUT);
    digitalWrite(GPS_DISABLE_PIN, LOW);
    pinMode(BATTERYPIN, INPUT);
}

void initializeRTC()
{
    // RTC Timer settings here
    if (!RTC_DS.begin())
    {
#if DEBUG == 1
        Serial.println("Couldn't find RTC");
#endif
        while (1)
            ;
    }
    // This may end up causing a problem in practice - what if RTC looses power in field? Shouldn't happen with coin cell batt backup
    if (RTC_DS.lostPower())
    {
#if DEBUG == 1
        Serial.println("RTC lost power, lets set the time!");
#endif
        // following line sets the RTC to the date & time this sketch was compiled
        RTC_DS.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
    //clear any pending alarms
    clearRTCAlarm();

    // Query Time and print
    DateTime now = RTC_DS.now();

    //Set SQW pin to OFF (in my case it was set by default to 1Hz)
    //The output of the DS3231 INT pin is connected to this pin
    //It must be connected to arduino Interrupt pin for wake-up
    RTC_DS.writeSqwPinMode(DS3231_OFF);

    //Set alarm1
    clearRTCAlarm();
}

/* Function to query current RTC time and add the period to set next alarm cycle */
void setRTCAlarm()
{
    DateTime now = RTC_DS.now(); // Check the current time
    // Calculate new time
    MIN = (now.minute() + RTC_WAKE_PERIOD) % 60;                      // wrap-around using modulo every 60 sec
    HR = (now.hour() + ((now.minute() + RTC_WAKE_PERIOD) / 60)) % 24; // quotient of now.min+periodMin added to now.hr, wraparound every 24hrs
#if DEBUG == 1
    Serial.print("RTC Time is: ");
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.println(now.second(), DEC);
    Serial.print("Resetting Alarm 1 for: ");
    Serial.print(HR);
    Serial.print(":");
    Serial.println(MIN);
#endif
    // Set alarm1
    RTC_DS.setAlarm(ALM1_MATCH_HOURS, MIN, HR, 0); //set your wake-up time here
    RTC_DS.alarmInterrupt(1, true);                //code to pull microprocessor out of sleep is tied to the time -> here
}

void readNMEA()
{
    uint16_t i = 0; // safety index
    char input;
    char buffer[GPS_BUFFER_LEN];
    memset(buffer, '\0', sizeof(buffer));
    if (PSTI(buffer, '$') && PSTI(buffer, 'P') && PSTI(buffer, 'S') && PSTI(buffer, 'T') && PSTI(buffer, 'I') && PSTI(buffer, ',') && PSTI(buffer, '0') && PSTI(buffer, '3') && PSTI(buffer, '0'))
    {
        while (Serial2.available())
        {
            input = Serial2.read();
            i++;
            // read till <CR>
            if (input == 13)
            {
                append(buffer, '\n');
                // verify the NMEA checksum, if its good write the string to SD
                if (verifyChecksum(buffer))
                {
                    if (!accelAwake && TOGGLE_NIGHT_SLEEP)
                    { // if accel Awake is not asserted then the rover woke due to RTC and we want to verify that it is bright out
                        Serial.println("Wake from RTC checking if it is light out");
                        checkNight(getValueAt(buffer, 2));
                    }
                    else
                        Serial.println("Wake from ACCEL no need to check if it is light out");

                    sd_save_elem_nodelim((char *)CurrentWakeGPS, buffer);
                    sd_save_elem_nodelim((char *)AllLogs, buffer);
#if TOGGLE_WDT
                    Watchdog.reset();
#endif
                }
                memset(buffer, '\0', sizeof(buffer));
                return;
            }
            // this simply protects the code in case a <CR> doesnt get received, we cannot go over the buffer limit
            if (i == GPS_BUFFER_LEN)
            {
                memset(buffer, '\0', sizeof(buffer));
                return;
            }
            append(buffer, input);
        }
    }
    memset(buffer, '\0', sizeof(buffer));
}

bool PSTI(char buf[], char chr)
{
    char input;
    if (Serial2.available())
    {
        input = Serial2.read();
        if (input == chr)
        {
            append(buf, input);
            return true;
        }
        else
            return false;
    }
}

void clearRTCAlarm()
{
    //clear any pending alarms
    RTC_DS.armAlarm(1, false);
    RTC_DS.clearAlarm(1);
    RTC_DS.alarmInterrupt(1, false);
    RTC_DS.armAlarm(2, false);
    RTC_DS.clearAlarm(2);
    RTC_DS.alarmInterrupt(2, false);
}

void interruptReset()
{
    detachInterrupt(digitalPinToInterrupt(ALERT_WAKE_PIN));
    detachInterrupt(digitalPinToInterrupt(RTC_WAKE_PIN));
    EIC->INTFLAG.reg = 0x01ff; // clear interrupt flags pending
    delay(10);                 // GPS switch will trigger accel interrupt if no delay
    attachInterrupt(digitalPinToInterrupt(ALERT_WAKE_PIN), wakeUp_alert, LOW);
    attachInterrupt(digitalPinToInterrupt(RTC_WAKE_PIN), wakeUp_RTC, LOW);
}

void timerFlagCheck()
{
    if (TimerFlag)
    {
        //**************** IMPORTANT, DO NOT EDIT INTERRUPT ISRs ****************
        //clear interrupt registers, attach interrupts EVERY TIME THE INTERRUPT IS CALLED
        interruptReset();
        timer = millis();
        TimerFlag = false;
    }
}

//Occurs when RTC alarm goes off
void RTCFlagCheck()
{
    if (RTCFlag)
    {
        clearRTCAlarm();
        Serial.println("Processor wake from RTC");

        DateTime now = RTC_DS.now();
        uint8_t mo = now.month();
        uint8_t d = now.day();
        uint8_t h = now.hour();
        uint8_t mm = now.minute();

        RTC_monthString = String(mo, DEC);
        RTC_dayString = String(d, DEC);
        RTC_hrString = String(h, DEC);
        RTC_minString = String(mm, DEC);
        RTC_timeString = RTC_hrString + ":" + RTC_minString + "_" + RTC_monthString + "/" + RTC_dayString + "\n";

        //Debugging
        sd_save_elem_nodelim((char *)AllLogs, "RTC WAKE: ");
        sd_save_elem_nodelim((char *)AllLogs, (char *)RTC_timeString.c_str());
        char msg[200];
        memset(msg, '\0', sizeof(msg));
        getVoltage(msg);
        strcat(msg, "\n");
        sd_save_elem_nodelim((char *)AllLogs, "Voltage: ");
        sd_save_elem_nodelim((char *)AllLogs, msg);
        //end debugging

#if DEBUG == 1
        Serial.println(RTC_timeString);
#endif
        RTCFlag = false;
        alertFlag = false;
        awakeFor = STANDARD_WAKE;
    }
}

//Occurs when Accelerometer goes off
void alertFlagCheck()
{
    if (alertFlag)
    {
        Serial.println("Processor wake from accelerometer");
        uint8_t dataRead = mma.readRegister8(MMA8451_REG_TRANSIENT_SRC); //clear the interrupt register
        mmaPrintIntSRC(dataRead);
        if (!stateSent)
        {
            Serial.println("Sending Accelerometer message");
            stateSent = true;
            accelAwake = true;
            delay(2000);
#if TOGGLE_TX
            sendState(mma, true);
#endif
            //Debugging
            DateTime now = RTC_DS.now();
            uint8_t mo = now.month();
            uint8_t d = now.day();
            uint8_t h = now.hour();
            uint8_t mm = now.minute();

            RTC_monthString = String(mo, DEC);
            RTC_dayString = String(d, DEC);
            RTC_hrString = String(h, DEC);
            RTC_minString = String(mm, DEC);
            RTC_timeString = RTC_hrString + ":" + RTC_minString + "_" + RTC_monthString + "/" + RTC_dayString + "\n";

            sd_save_elem_nodelim((char *)AllLogs, "ACCEL WAKE: ");
            sd_save_elem_nodelim((char *)AllLogs, (char *)RTC_timeString.c_str());
            char msg[200];
            memset(msg, '\0', sizeof(msg));
            getVoltage(msg);
            strcat(msg, "\n");
            sd_save_elem_nodelim((char *)AllLogs, "Voltage: ");
            sd_save_elem_nodelim((char *)AllLogs, msg);
            //end debugging
        }

        interruptReset();
        alertFlag = false;
        awakeFor = ALERT_WAKE;
    }
}

void resetFlags()
{
    TimerFlag = false;
    alertFlag = false;
}

void sendState(Adafruit_MMA8451 device, bool accel)
{
    char msg[150];
    memset(msg, '\0', sizeof(msg));
    String reading;

    device.getEvent(&event);

    // we only want to send accelerometer data if the device
    if (accel)
    {
        strcat(msg, "/Accel");
        strcat(msg, ",");
    }
    else
    {
        strcat(msg, "/State");
        strcat(msg, ",");
    }

    reading = "0";
    strcat(msg, reading.c_str());
    strcat(msg, ",");
    reading = device.x;
    strcat(msg, reading.c_str()); // accel x
    strcat(msg, ",");
    reading = device.y;
    strcat(msg, reading.c_str()); // accel y
    strcat(msg, ",");
    reading = device.z;
    strcat(msg, reading.c_str()); // accel z
    getVoltage(msg);

    //add a checksum to the message
    strcat(msg, ",");
    addChecksum(msg);

    //send 50 replicas to ensure success

    for (uint8_t i = 0; i < 20; i++)
    {
        sendMessage(msg, Serial1);
        delay(50);
    }
}

void getVoltage(char msg[])
{
    int raw;
    float voltage;
    char buf[10];
    memset(buf, '\0', sizeof(buf)); //clear the buffer
    raw = analogRead(BATTERYPIN);
    raw = map(raw, 0, 1023, 0, 3700);
    voltage = ((float)raw / 1000);
    snprintf(buf, sizeof(buf), "%f", voltage);
    strcat(msg, ",");
    strcat(msg, buf); // Battery voltage
}

void tryStandby()
{
    if ((millis() - timer > 1000 * awakeFor || nightFlag) && TOGGLE_SLEEP) //if it is dark out go back to sleep
    {
        //if it is night and the Rover was not triggered by the accelerometer
        if (!nightFlag)
        {
#if TOGGLE_TX
            processGPS((char *)CurrentWakeGPS, NODE_NUM, Serial1);
            sendState(mma, false);
#endif
            Serial.println("GPS data written out");
        }
        delay(2000);

        //------------- TESTING SOME DEBUG STUFF -----------
        DateTime now = RTC_DS.now();
        uint8_t mo = now.month();
        uint8_t d = now.day();
        uint8_t h = now.hour();
        uint8_t mm = now.minute();

        RTC_monthString = String(mo, DEC);
        RTC_dayString = String(d, DEC);
        RTC_hrString = String(h, DEC);
        RTC_minString = String(mm, DEC);
        RTC_timeString = RTC_hrString + ":" + RTC_minString + "_" + RTC_monthString + "/" + RTC_dayString + "\n";

        sd_save_elem_nodelim((char *)AllLogs, "RTC SLEEP: ");
        sd_save_elem_nodelim((char *)AllLogs, (char *)RTC_timeString.c_str());
        //debugging
        char msg[200];
        memset(msg, '\0', sizeof(msg));
        getVoltage(msg);
        strcat(msg, "\n");
        sd_save_elem_nodelim((char *)AllLogs, "Voltage: ");
        sd_save_elem_nodelim((char *)AllLogs, msg);
        //end debugging

#if TOGGLE_WDT
        Watchdog.disable();
#endif
        //-----------------------------------------

        gps_off();
        delay(100);
        //reset all flags
        resetFlags();

        // Fill and send a state packet
        setRTCAlarm(); //reset alarm to go off one wake period from sleeping

        Serial.println("STANDBY");
        Serial.end();
        USBDevice.detach();
        while (Serial2.available())
        {
            readNMEA();
        }
        sd_save_elem_nodelim((char *)AllLogs, "CYCLE\n");

        nightFlag = false;
        accelAwake = false;
        stateSent = false;
        alertFlag = false;

        delay(20);        // delay is so that the gps switch doesn't trigger accelerometer wake
        interruptReset(); //clear interrupt registers, attach interrupts

        LowPower.standby();
        // ========================================================================================
        // ====================== Sleep here and wait for int (accel or RTC) ======================
#if TOGGLE_WDT
        Watchdog.enable(20000);
#endif
        USBDevice.attach();
        gps_on();

        clearRTCAlarm(); //prevent double trigger of alarm interrupt
        Serial.begin(115200);
        delay(5000);
    }
}

void Serial2Setup()
{
    Serial2.begin(BAUD);
    // Assign pins 11,12 SERCOM functionality, internal function
    pinPeripheral(SERIAL2_TX, PIO_SERCOM); //Private functions for serial communication
    pinPeripheral(SERIAL2_RX, PIO_SERCOM);
}

void mmaPrintIntSRC(uint8_t dataRead)
{
    if (dataRead & 0x40)
        Serial.println("Event Active");
    if (dataRead & 0x20)
    {
        Serial.println("\tZ event");
        if (dataRead & 0x10)
            Serial.println("\t\tZ Negative g");
        else
            Serial.println("\t\tZ Positive g");
    }
    if (dataRead & 0x08)
    {
        Serial.println("\tY event");
        if (dataRead & 0x04)
            Serial.println("\t\tY Negative g");
        else
            Serial.println("\t\tY Positive g");
    }
    if (dataRead & 0x02)
    {
        Serial.println("\tX event");
        if (dataRead & 0x01)
            Serial.println("\t\tX Negative g");
        else
            Serial.println("\t\tX Positive g");
    }
}

// Turn the GPS module on (consumes ~150 mA)
void gps_on()
{
    digitalWrite(GPS_EN_PIN, HIGH);
    delay(10);
    digitalWrite(GPS_EN_PIN, LOW);
}

// Turn off the gps module, save power during sleep
// Functions affect physical latch on feather relay
void gps_off()
{
    digitalWrite(GPS_DISABLE_PIN, HIGH);
    delay(10);
    digitalWrite(GPS_DISABLE_PIN, LOW);
}

void setup_sd()
{
    Serial.println("Initializing SD card...");

#if is_lora == 1
    digitalWrite(8, HIGH); // if using LoRa
#endif

    if (!SD.begin(chipSelect))
    {
        Serial.println("SD Initialization failed!");
        Serial.println("Will continue anyway, but SD fuctions wont work");
    }
    Serial.println("initialization done.");
}

void configInterrupts(Adafruit_MMA8451 device)
{
    uint8_t dataToWrite = 0;
    // MMA8451_REG_CTRL_REG2
    // sysatem control register 2

    //dataToWrite |= 0x80;    // Auto sleep/wake interrupt
    //dataToWrite |= 0x40;    // FIFO interrupt
    //dataToWrite |= 0x20;    // Transient interrupt - enabled
    //dataToWrite |= 0x10;    // orientation
    //dataToWrite |= 0x08;    // Pulse interrupt
    //dataToWrite |= 0x04;    // Freefall interrupt
    //dataToWrite |= 0x01;    // data ready interrupt, MUST BE ENABLED FOR USE WITH ARDUINO

    // MMA8451_REG_CTRL_REG3
    // Interrupt control register

    dataToWrite = 0;
    dataToWrite |= 0x80; // FIFO gate option for wake/sleep transition, default 0, Asserting this allows the accelerometer to collect data the moment an impluse happens and preserve that data because the FIFO buffer is blocked. Thus at the end of a wake cycle the data from the initial transient wake up is still in the buffer
    dataToWrite |= 0x40; // Wake from transient interrupt enable
    //dataToWrite |= 0x20;    // Wake from orientation interrupt enable
    //dataToWrite |= 0x10;    // Wake from Pulse function enable
    //dataToWrite |= 0x08;    // Wake from freefall/motion decect interrupt
    //dataToWrite |= 0x02;    // Interrupt polarity, 1 = active high
    dataToWrite |= 0x00; // (0) Push/pull or (1) open drain interrupt, determines whether bus is driven by device, or left to hang

    device.writeRegister8_public(MMA8451_REG_CTRL_REG3, dataToWrite);

    dataToWrite = 0;

    // MMA8451_REG_CTRL_REG4
    // Interrupt enable register, enables interrupts that are not commented

    //dataToWrite |= 0x80;    // Auto sleep/wake interrupt
    //dataToWrite |= 0x40;    // FIFO interrupt
    dataToWrite |= 0x20; // Transient interrupt - enabled
    //dataToWrite |= 0x10;    // orientation
    //dataToWrite |= 0x08;    // Pulse interrupt
    //dataToWrite |= 0x04;    // Freefall interrupt
    dataToWrite |= 0x01; // data ready interrupt, MUST BE ENABLED FOR USE WITH ARDUINO
    device.writeRegister8_public(MMA8451_REG_CTRL_REG4, dataToWrite | 0x01);

    dataToWrite = 0;

    // MMA8451_REG_CTRL_REG5
    // Interrupt pin 1/2 configuration register, bit == 1 => interrupt to pin 1
    // see datasheet for interrupt's description, threshold int routed to pin 1
    // comment = int2, uncoment = int1

    //dataToWrite |= 0x80;    // Auto sleep/wake
    //dataToWrite |= 0x40;    // FIFO
    dataToWrite |= 0x20; // Transient, asserting this routes transients interrupts to INT1 pin
    //dataToWrite |= 0x10;    // orientation
    //dataToWrite |= 0x08;    // Pulse
    //dataToWrite |= 0x04;    // Freefall
    //dataToWrite |= 0x01;    // data ready

    device.writeRegister8_public(MMA8451_REG_CTRL_REG5, dataToWrite);

    dataToWrite = 0;

    // MMA8451_REG_TRANSIENT_CFG
    //dataToWrite |= 0x10;  // Latch enable to capture accel values when interrupt occurs
    dataToWrite |= 0x08; // Z transient interrupt enable
    dataToWrite |= 0x04; // Y transient interrupt enable
    dataToWrite |= 0x02; // X transient interrupt enable
    //dataToWrite |= 0x01;    // High-pass filter bypass
    device.writeRegister8_public(MMA8451_REG_TRANSIENT_CFG, dataToWrite);

    Serial.print("MMA8451_REG_TRANSIENT_CFG: ");
    Serial.println(device.readRegister8(MMA8451_REG_TRANSIENT_CFG), HEX);

    dataToWrite = 0;

    // MMA8451_REG_TRANSIENT_THS
    // Transient interrupt threshold in units of .06g
    //Acceptable range is 1-127
    dataToWrite = 0x01;
    device.writeRegister8_public(MMA8451_REG_TRANSIENT_THS, dataToWrite);

    dataToWrite = 0;

    // MMA8451_REG_TRANSIENT_CT  0x20
    dataToWrite = 0; // value is 0-255 for numer of counts to debounce for, depends on ODR
    device.writeRegister8_public(MMA8451_REG_TRANSIENT_CT, dataToWrite);

    dataToWrite = 0;
}

//For PST time zone only, if accelAwake is not asserted (i.e. this wake cycle is due ot RTC) we check the timestamp on the NMEA string.
//If the time stamp is night time then we se tnight flag, which will cause trystandby to trigger
void checkNight(char *UTCtime)
{
    int time = atoi(UTCtime);
#if DEBUG
    Serial.print("TIME: ");
    Serial.println(time);
#endif
    if (time > 30000 && time < 120000)
    {
        Serial.println("Dark outside");
        nightFlag = true;
    }
    else
        Serial.println("Light outside");
}
