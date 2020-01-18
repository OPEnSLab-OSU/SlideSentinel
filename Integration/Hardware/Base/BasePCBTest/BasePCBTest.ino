#include <Arduino.h>
#include <SPI.h>
#include <wiring_private.h>
#include <RHReliableDatagram.h>
#include <RH_Serial.h>
#include <SD.h>

// RADIO INTERFACE
#define RST 5

// Switch Pin Def
#define SPDT_SEL A0

// Relay constants
#define GNSS_ON_PIN A2
#define GNSS_OFF_PIN 12

// SD CARD CONSTANTS
#define SD_CS 10

// PMC REGULATOR
#define VCC2_EN 21 //SCL

// Switch Pin Def
#define SPDT_SEL A0

// Radio Serial
#define ROCK_TX 11
#define ROCK_RX 12
#define ROCK_ONOFF 9
#define ROCK_NETAV A5
#define ROCK_RINGAL A3

// Reliable Datagram usage
#define CLIENT_ADDRESS 1
#define SERVER_ADDRESS 2
RH_Serial driver(Serial1);
RHReliableDatagram manager(driver, SERVER_ADDRESS);
uint8_t buf[RH_SERIAL_MAX_MESSAGE_LEN];
uint8_t data[] = "ACK";

#define IRIDIUM_BAUD 115200
Uart Serial2(&sercom1, ROCK_RX, ROCK_TX, SERCOM_RX_PAD_3, UART_TX_PAD_0);
void SERCOM1_Handler()
{
    Serial2.IrqHandler();
}

void Serial2Setup(uint16_t baudrate)
{
    Serial2.begin(baudrate);
    pinPeripheral(ROCK_TX, PIO_SERCOM); //Private functions for serial communication
    pinPeripheral(ROCK_RX, PIO_SERCOM);
}

void useRelay(uint8_t pin)
{
    digitalWrite(pin, HIGH);
    delay(4);
    digitalWrite(pin, LOW);
}

void setup()
{
    Serial.begin(115200);

    // Radio Serial
    Serial1.begin(115200);

    // Set up Reliable datagram socket
    if (!manager.init())
        Serial.println("init failed");

    // Iridium Serial
    Serial2Setup(IRIDIUM_BAUD);
    pinMode(ROCK_ONOFF, OUTPUT);
    pinMode(ROCK_NETAV, INPUT);
    pinMode(ROCK_RINGAL, INPUT);

    // setup SD
    pinMode(SD_CS, OUTPUT);
    setup_sd();

    // setup Relay
    pinMode(GNSS_ON_PIN, OUTPUT);
    pinMode(GNSS_OFF_PIN, OUTPUT);
    useRelay(GNSS_OFF_PIN);

    // SPDT INIT, Feather receives data
    pinMode(SPDT_SEL, OUTPUT);
    digitalWrite(SPDT_SEL, HIGH);

    // prevent radio reset
    pinMode(RST, OUTPUT);
    digitalWrite(RST, HIGH);

    pinMode(VCC2_EN, OUTPUT);
    digitalWrite(VCC2_EN, HIGH);
}

void loop()
{
    //test();
    memset(buf, '\0', sizeof(buf));
    if (manager.available())
    {
        // Turn this high so the base can send an ACK!
        digitalWrite(SPDT_SEL, HIGH);
        // Wait for a message addressed to us from the client
        uint8_t len = sizeof(buf);
        uint8_t from;
        if (manager.recvfromAck(buf, &len, &from))
        {
            Serial.print("got request from : 0x");
            Serial.print(from, HEX);
            Serial.print(": ");
            Serial.println((char *)buf);

            // Send a reply back to the originator client
            if (!manager.sendtoWait(data, sizeof(data), from))  //SEND CONFIG DATA TIED TO THIS
                Serial.println("sendtoWait failed");
        }
        // Start RTK 
        if (buf[0] == 'a')
        {

            Serial.println("(A0 HIGH) GNSS -----> RADIO");
            digitalWrite(SPDT_SEL, LOW);
            useRelay(GNSS_ON_PIN);
        }
        // End RTK
        else if (buf[0] == 'b')
        {
            Serial.println("(A0 HIGH) FEATHER_M0 -----> RADIO");
            digitalWrite(SPDT_SEL, HIGH);
            useRelay(GNSS_OFF_PIN);
        }
    }
}

void test()
{
    char cmd;
    if (Serial.available())
    {
        cmd = Serial.read();
        switch (cmd)
        {
        case '1':
            Serial.println("gnss on");
            useRelay(GNSS_ON_PIN);
            break;
        case '2':
            Serial.println("gnss off");
            useRelay(GNSS_OFF_PIN);
            break;
        case '3':
            Serial1.println("TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST");
            Serial.println("Writing data out...");
            break;
        case '4':
            Serial.println("(A0 LOW) GNSS -----> RADIO");
            digitalWrite(SPDT_SEL, LOW);
            break;
        case '5':
            Serial.println("(A0 HIGH) FEATHER_M0 -----> RADIO");
            digitalWrite(SPDT_SEL, HIGH);
            break;
        case '6':
            Serial2.println("TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST TEST");
            Serial.println("Writing data out...");
            break;
        case '7':
            digitalWrite(ROCK_ONOFF, HIGH);
            Serial.println("Turning SATCOM on");
            break;
        case '8':
            digitalWrite(ROCK_ONOFF, LOW);
            Serial.println("Turning SATCOM off");
            break;
        case '9':
            digitalWrite(VCC2_EN, HIGH);
            Serial.println("Turning on second voltage rail");
            break;
        case '0':
            digitalWrite(VCC2_EN, LOW);
            Serial.println("Turning off second voltage rail");
            break;
        case 'f':
            Serial.println("Resetting the radio, DRIVING RST LOW");
            digitalWrite(RST, LOW);
            delay(2000);
            digitalWrite(RST, HIGH);
            break;
        }
    }
    if(Serial1.available()){
        Serial.print((char)Serial1.read());
    }
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
