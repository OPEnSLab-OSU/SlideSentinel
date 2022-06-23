#include "Rover.h"
#include <Wire.h>


/* Ran on first bootup of Main. Pass in */
Rover::Rover() :    m_max4280(MAX_CS, &SPI), 
                    m_multiplex(SPDT_SEL, -1),
                    m_serial(Serial1),
                    m_RHSerialDriver(m_serial),
                    m_RHManager(m_RHSerialDriver, CLIENT_ADDR),
                    m_RHMessage(1024) {
    m_rovInfo.id = CLIENT_ADDR;
    m_rovInfo.serverAddr = SERVER_ADDR;
    m_rovInfo.init_retries = INIT_RETRIES;
    m_rovInfo.timeout = INIT_TIMEOUT;
    m_RHMessage["ID"] = m_rovInfo.id; //example using dynamic json document to set information TBD
    m_RHMessage["TYPE"] = "";
    m_RHMessage["MSG"] = "";    
}

void Rover::initRadio(){
    m_serial.begin(115200);

    m_RHManager.setTimeout(m_rovInfo.timeout);
    m_RHManager.setRetries(m_rovInfo.init_retries);

    m_RHManager.init();
}

void Rover::wake(){
    powerRadio();
    for (int i = 0; i < 20; i++) {
        Serial.println(i);
        Serial.println(" ");
        delay(1000);
    }
      
}

bool Rover::request(){
    //TDL: Conditionally enable max3243
    setMux(RadioToFeather);
    JsonObject RHJson = m_RHMessage.to<JsonObject>();
    //wipe message first

    RHJson["TYPE"] = "REQUEST";
    RHJson["MSG"] = "REQUEST";
    // Serial.println(m_RHMessage);
    // String RHMessageStr = "";

    //serialize json object into a string format
    char processedRHMessage[255];
    serializeJson(RHJson, processedRHMessage);
    Serial.println(processedRHMessage);
    // ast string to a uint8_t* so radiohead can send it
    // uint8_t* processedRHMessage = reinterpret_cast<uint8_t*>((char *)RHMessageStr.c_str());

    //will block while waiting on timeout, should be 2-4 seconds by default
    bool status = m_RHManager.sendtoWait((uint8_t*)processedRHMessage, measureJson(RHJson), SERVER_ADDR);
    return status;
}

void Rover::sendManualMsg(char* msg){
    // String RHMessageStr = "";
    StaticJsonDocument<JSON_OBJECT_SIZE(3)> testdoc;
    JsonObject RHMessageObject = testdoc.to<JsonObject>();
    RHMessageObject["TYPE"] = "Debug";
    RHMessageObject["MSG"] = msg;

    // uint8_t* processedRHMessage = reinterpret_cast<uint8_t*>((char *)RHMessageStr.c_str());
    char processedRHMessage[255];
    serializeJson(RHMessageObject, processedRHMessage);
    Serial.println(processedRHMessage);
    m_RHManager.sendtoWait((uint8_t*)processedRHMessage, measureJson(RHMessageObject), SERVER_ADDR);
    // Serial.println(status);
}

void Rover::debugRTCPrint(){
    delay(5000);
    Serial.println("Testing RTC...");
    Wire.begin();
    if(m_RTC.begin()){
        Serial.println("RTC connected");
    }else{
        Serial.println("RTC Failed");
    }
}

//prototype
uint8_t len = RH_SERIAL_MAX_MESSAGE_LEN;
char m_buf[RH_SERIAL_MAX_MESSAGE_LEN];
bool Rover::listen(){
    if(m_RHManager.available()){
        if (m_RHManager.recvfromAckTimeout((uint8_t *)m_buf, &len, m_rovInfo.init_retries, 0))
            Serial.println(m_buf);
            return true;
    }
    return false;
}

void Rover::powerRadio(){
    Serial.println("Powering radio on.");
    m_max4280.assertRail(0);
}

void Rover::powerDownRadio(){
    Serial.println("Powering radio down.");
    m_max4280.assertRail(1);
}

void Rover::powerGNSS(){
    Serial.println("Powering GNSS on.");
    m_max4280.assertRail(2);
}

void Rover::powerDownGNSS(){
    Serial.println("Powering GNSS down.");
    m_max4280.assertRail(3);
}

void Rover::setMux(MuxFormat format){
    if(format == RadioToFeather){
        m_multiplex.comY1();         
        Serial.println("Radio ---> Feather");
    }else if(format == RadioToGNSS){
        m_multiplex.comY2();          
        Serial.println("Radio ---> GNSS");
    }
}
