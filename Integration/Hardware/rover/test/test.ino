#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_MMA8451.h>
#include <RTClibExtended.h>
#include <wiring_private.h> // Pin peripheral
#include <RHReliableDatagram.h>
#include <RH_Serial.h>
#include <SD.h>

// Test Toggle
#define ADVANCED true

// RS232 Interface Pin Def
#define FORCEOFF_N A5

// RADIO INTERFACE
#define RST 6
#define CD 10
// COMMUNICATION OVER SERIAL1

// Switch Pin Def
#define SPDT_SEL A0

// Relay constants
#define CS_4280 9
#define GNSS_RESET 0x08
#define GNSS_SET 0x04
#define RADIO_RESET 0x02
#define RADIO_SET 0x01
#define CLEAR_RELAYS 0x00

// SD CARD CONSTANTS
#define SD_CS 18

// PMC REGULATOR
#define VCC2_EN 13

void useRelay(uint8_t word)
{
    digitalWrite(CS_4280, LOW);
    SPI.transfer(word);
    digitalWrite(CS_4280, HIGH);
    delay(200);
    digitalWrite(CS_4280, LOW);
    SPI.transfer(CLEAR_RELAYS);
    digitalWrite(CS_4280, HIGH);
}

// Instatiate ACCELEROMETER Object
#define ACCEL_INT A3
Adafruit_MMA8451 mma = Adafruit_MMA8451();

// Instatiate RTC Object
#define RTC_INT 5
RTC_DS3231 RTC_DS;
volatile int HR = 8; //  These should not be volatile
volatile int MIN = 0;
volatile int awakeFor = 20;
#define RTC_WAKE_PERIOD 1 // Interval to wake and take sample in Min, reset alarm based on this period (Bo - 5 min), 15 min

// Reliable Datagram
#define CLIENT_ADDRESS 1
#define SERVER_ADDRESS 2
RH_Serial driver(Serial1);
RHReliableDatagram manager(driver, CLIENT_ADDRESS);
uint8_t data[2];
uint8_t buf[RH_SERIAL_MAX_MESSAGE_LEN];

// GNSS Serial Init
#define SERIAL2_TX 11
#define SERIAL2_RX 12
#define GNSS_BAUD 115200
Uart Serial2(&sercom1, SERIAL2_RX, SERIAL2_TX, SERCOM_RX_PAD_3, UART_TX_PAD_0);
void SERCOM1_Handler()
{
    Serial2.IrqHandler();
}
void Serial2Setup(uint16_t baudrate)
{
    Serial2.begin(baudrate);
    // Assign pins 11,12 SERCOM functionality, internal function
    pinPeripheral(SERIAL2_TX, PIO_SERCOM); // Private functions for serial communication
    pinPeripheral(SERIAL2_RX, PIO_SERCOM);
}

void setup()
{
    Serial.begin(115200);
    while (!Serial)
        ;

    // SPI INIT
    SPI.begin();
    SPI.setClockDivider(SPI_CLOCK_DIV8);

    // RADIO INIT
    Serial1.begin(115200);
    pinMode(RST, OUTPUT);
    digitalWrite(RST, HIGH);
    pinMode(CD, INPUT);

    if (!manager.init())
        Serial.println("init failed");

    // Clear Relays on INIT
    useRelay(CLEAR_RELAYS);

    // RS232 INIT
    pinMode(FORCEOFF_N, OUTPUT);
    Serial.println("RS232 ---> ON (Driving it HIGH)");
    digitalWrite(FORCEOFF_N, HIGH);

    // SPDT INIT
    pinMode(SPDT_SEL, OUTPUT);
    digitalWrite(SPDT_SEL, LOW);

    // RELAY DRIVER INIT
    pinMode(CS_4280, OUTPUT);

    // ACCELEROMETER INIT
    Serial.println("Setting up MMA");
    digitalWrite(ACCEL_INT, INPUT_PULLUP);
    mmaSetupSlideSentinel();
    configInterrupts(mma);
    attachInterrupt(digitalPinToInterrupt(ACCEL_INT), accelInt, FALLING);
    mma.readRegister8(MMA8451_REG_TRANSIENT_SRC);

    // RTC INIT
    pinMode(RTC_INT, INPUT_PULLUP); //active low interrupts
    initializeRTC();

    // GNSS INIT
    Serial2Setup(GNSS_BAUD);

    // Set the XOSC32K to run in standby, external 32 KHz clock must be used for interrupt detection in order to catch falling edges
    SYSCTRL->XOSC32K.bit.RUNSTDBY = 1;

    // INIT EXTERNAL OSCILLATOR FOR RISING AND FALLING interrupts  // Configure EIC to use GCLK1 which uses XOSC32K
    // This has to be done after the first call to attachInterrupt()
    GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID(GCM_EIC) |
                        GCLK_CLKCTRL_GEN_GCLK1 |
                        GCLK_CLKCTRL_CLKEN;

    // SD Card Initialization
    pinMode(SD_CS, OUTPUT);
    setup_sd();

    // REGULATOR INIT
    pinMode(VCC2_EN, OUTPUT);
    digitalWrite(VCC2_EN, HIGH);
}

void loop()
{
    if (ADVANCED)
    {
        advancedTest();
    }
}

void advancedTest()
{
    char cmd;
    if (Serial.available())
    {
        DateTime now = RTC_DS.now();
        cmd = Serial.read();
        switch (cmd)
        {
        case '1':
            Serial.println("set gnss");
            useRelay(GNSS_SET);
            break;
        case '2':
            Serial.println("reset gnss");
            useRelay(GNSS_RESET);
            break;
        case '3':
            Serial.println("set radio");
            useRelay(RADIO_SET);
            break;
        case '4':
            Serial.println("reset radio");
            useRelay(RADIO_RESET);
            break;
        case '5':
            Serial1.println("TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST");
            Serial.println("Writing data out...");
            break;
        case '6':
            Serial.println("RADIO -----> FEATHER M0 (A0 LOW)");
            digitalWrite(SPDT_SEL, LOW);
            break;
        case '7':
            Serial.println("RADIO -----> GNSS RECEIVER (A0 HIGH)");
            digitalWrite(SPDT_SEL, HIGH);
            break;
        case '8':
            Serial.println("RS232 ---> OFF (Driving it LOW)");
            digitalWrite(FORCEOFF_N, LOW);
            break;
        case '9':
            Serial.println("RS232 ---> ON (Driving it HIGH)");
            digitalWrite(FORCEOFF_N, HIGH);
            break;
        case 'r':
            Serial.println("Resetting the radio, DRIVING RST LOW");
            digitalWrite(RST, LOW);
            delay(2000);
            digitalWrite(RST, HIGH);
            break;
        case 't':
            Serial.print("STATE of CD pin: ");
            Serial.println(digitalRead(CD));
            break;
        case 'w':
            setRTCAlarm();
            break;
        case 'j':
            Serial.print("RTC Time is: ");
            Serial.print(now.hour(), DEC);
            Serial.print(':');
            Serial.print(now.minute(), DEC);
            Serial.print(':');
            Serial.println(now.second(), DEC);
            break;
        case 's':
            Serial.println("Sleeping");
            //Disable USB
            USB->DEVICE.CTRLA.reg &= ~USB_CTRLA_ENABLE;
            //Enter sleep mode
            SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
            __DSB();
            __WFI();
            //...Sleep
            //Enable USB
            USB->DEVICE.CTRLA.reg |= USB_CTRLA_ENABLE;
            useRelay(RADIO_SET);
            delay(1000);
            useRelay(RADIO_RESET);
            Serial.begin(115200);
            Serial.println("Awake");
            break;
        case 'x':
            Serial.println("Turning off VCC2");
            digitalWrite(VCC2_EN, LOW);
            break;
        case '$':
            Serial.println("Turning BASE station ON");
            memset(buf, '\0', sizeof(buf));
            data[0] = 'a';
            if (manager.sendtoWait(data, sizeof(data), SERVER_ADDRESS))
            {
                // Now wait for a reply from the server
                uint8_t len = sizeof(buf);
                uint8_t from;
                if (manager.recvfromAckTimeout(buf, &len, 2000, &from))
                {
                    Serial.print("got reply from : 0x");
                    Serial.print(from, HEX);
                    Serial.print(": ");
                    Serial.println((char *)buf);
                    Serial.println("Switching: RADIO TX ------> GNSS");
                    digitalWrite(SPDT_SEL, HIGH);
                }
                else
                {
                    Serial.println("No reply, is serial_reliable_datagram_server running?");
                }
            }
            else
                Serial.println("sendtoWait failed");
            delay(500);
            break;
        case '#':       // Make sure this works
            Serial.println("Turning BASE station OFF");
            // Drive Low to receive ack from BASE
            digitalWrite(SPDT_SEL, LOW);
            memset(buf, '\0', sizeof(buf));
            data[0] = 'b';
            if (manager.sendtoWait(data, sizeof(data), SERVER_ADDRESS))
            {
                // Now wait for a reply from the server
                uint8_t len = sizeof(buf);
                uint8_t from;
                if (manager.recvfromAckTimeout(buf, &len, 2000, &from))
                {
                    Serial.print("got reply from : 0x");
                    Serial.print(from, HEX);
                    Serial.print(": ");
                    Serial.println((char *)buf);
                    Serial.println("Switching: RADIO TX ------> FEATHER M0");
                    digitalWrite(SPDT_SEL, LOW);
                }
                else
                {
                    Serial.println("No reply, is serial_reliable_datagram_server running?");
                }
            }
            else
                Serial.println("sendtoWait failed");
            delay(500);
            break;
        }
    }

    if (Serial1.available())
    {
        Serial.print((char)Serial1.read());
    }

    if (Serial2.available())
    {
        Serial.print(Serial2.read());
    }
}

void mmaSetupSlideSentinel()
{
    if (!mma.begin())
    {
        Serial.println("Unable to find MMA8451");
        while (1)
            ;
    }

    mma.setRange(MMA8451_RANGE_2_G);
    mma.setDataRate(MMA8451_DATARATE_6_25HZ);
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

    dataToWrite = 0;

    // MMA8451_REG_TRANSIENT_THS
    // Transient interrupt threshold in units of .06g
    //Acceptable range is 1-127
    dataToWrite = 0x2F;
    device.writeRegister8_public(MMA8451_REG_TRANSIENT_THS, dataToWrite);

    dataToWrite = 0;

    // MMA8451_REG_TRANSIENT_CT  0x20
    dataToWrite = 0; // value is 0-255 for numer of counts to debounce for, depends on ODR
    device.writeRegister8_public(MMA8451_REG_TRANSIENT_CT, dataToWrite);

    dataToWrite = 0;
}

void accelInt()
{
    detachInterrupt(digitalPinToInterrupt(ACCEL_INT));
    Serial.println("Accelerometer Wake");
    attachInterrupt(digitalPinToInterrupt(ACCEL_INT), accelInt, CHANGE);
}

void initializeRTC()
{
    if (!RTC_DS.begin())
    {
        Serial.println("Couldn't find RTC");
        while (1)
            ;
    }
    //clear any pending alarms
    clearRTCAlarm();

    //Set SQW pin to OFF (in my case it was set by default to 1Hz)
    //The output of the DS3231 INT pin is connected to this pin
    //It must be connected to arduino Interrupt pin for wake-up
    RTC_DS.writeSqwPinMode(DS3231_OFF);

    //Set alarm1
    clearRTCAlarm();
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

void setRTCAlarm()
{
    DateTime now = RTC_DS.now();                                      // Check the current time
    MIN = (now.minute() + RTC_WAKE_PERIOD) % 60;                      // wrap-around using modulo every 60 sec
    HR = (now.hour() + ((now.minute() + RTC_WAKE_PERIOD) / 60)) % 24; // quotient of now.min+periodMin added to now.hr, wraparound every 24hrs

    Serial.print("Setting Alarm 1 for: ");
    Serial.print(HR);
    Serial.print(":");
    Serial.println(MIN);

    // Set alarm1
    RTC_DS.setAlarm(ALM1_MATCH_HOURS, MIN, HR, 0); //set your wake-up time here
    RTC_DS.alarmInterrupt(1, true);                //code to pull microprocessor out of sleep is tied to the time -> here
    attachInterrupt(digitalPinToInterrupt(RTC_INT), rtcInt, FALLING);
}

void rtcInt()
{
    detachInterrupt(digitalPinToInterrupt(RTC_INT));
    Serial.println("RTC Wake");
    attachInterrupt(digitalPinToInterrupt(RTC_INT), rtcInt, FALLING);
}

void setup_sd()
{
    Serial.println("Initializing SD card...");

    if (!SD.begin(SD_CS))
    {
        Serial.println("SD Initialization failed!");
        Serial.println("Will continue anyway, but SD functions will be skipped");
    }
    else
    {
        Serial.println("initialization complete");
    }
}
