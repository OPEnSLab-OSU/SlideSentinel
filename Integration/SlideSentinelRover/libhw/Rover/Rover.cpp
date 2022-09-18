#include "Rover.h"
#include <Wire.h>

// #include <avr/sleep.h>



/* Ran on first bootup of Main. Pass in */
Rover::Rover() :    m_max4820(MAX_CS, &SPI), 
                    m_max3243(FORCEOFF_N),
                    m_multiplex(SPDT_SEL, -1),
                    m_RadioManager(),
                    m_gnss(Serial1, 115200, 12, 11, 30),
                    m_JSONData(1024) {
    m_rovInfo.id = CLIENT_ADDR;
    m_rovInfo.serverAddr = SERVER_ADDR;
    m_rovInfo.init_retries = INIT_RETRIES;
    m_rovInfo.timeout = INIT_TIMEOUT;
    //now JSONData
    // m_RHMessage["ID"] = m_rovInfo.id; //example using dynamic json document to set information TBD
    // m_RHMessage["TYPE"] = "";
    // m_RHMessage["MSG"] = "";

    //nowDT = m_RTC.now();
}

void Rover::initRTC(){
    m_RTC.begin();
    m_RTC.adjust(DateTime(F(__DATE__), F(__TIME__)));//set date-time manualy:yr,mo,dy,hr,mn,sec   
}

void Rover::initRHParams(){
    Serial.println("This address is : ");
    // Serial.println(m_RHManager.thisAddress());
   // m_RHManager.setTimeout(m_rovInfo.timeout);
    // m_RHManager.setRetries(m_rovInfo.init_retries);

    // m_RHManager.init();
}

void Rover::wake(){
    powerRadio();
    for (int i = 0; i < 2; i++) {
        Serial.println(i);
        Serial.println(" ");
        delay(1000);
    } 
}
void Rover::packageData(DataType packType){
    JsonObject RHJson = m_JSONData.to<JsonObject>();

    switch(packType){
        case REQUEST: 
            RHJson["TYPE"] = "REQUEST";
            RHJson["MSG"] = "RTK_REQUEST";
            break;
        case UPLOAD:
            RHJson["TYPE"] = "UPLOAD";

            // Take the message in as an object to create a new GNSS data object
            // m_gnss.populateGNSSMessage(RHJson["MSG"].as<JsonObject>()); premerge 8/16
            RHJson["MSG"]=m_gnss.populateGNSS();
            break;
        case ALERT:
            RHJson["TYPE"] = "ALERT";
            RHJson["MSG"] = "UNSPECIFIED_ALERT";
            break;

    }
}

bool Rover::transmit(){
    
//     //TDL: Conditionally enable max3243
    setMux(RadioToFeather);
    delay(5);
    String processedRHMessage;
    serializeJson(m_JSONData, processedRHMessage);
    Serial.println(processedRHMessage);

//     //serialize json object into a string format
//     // ast string to a uint8_t* so radiohead can send it
//     // uint8_t* processedRHMessage = reinterpret_cast<uint8_t*>((char *)RHMessageStr.c_str());
    return m_RadioManager.sendPacket(processedRHMessage, SERVER_ADDR);

//     //will block while waiting on timeout, should be 2-4 seconds by default
//     return m_RHManager.sendtoWait((uint8_t*)processedRHMessage, measureJson(m_RHMessage), SERVER_ADDR);
return true;
}

bool Rover::waitAndReceive(){
    return m_RadioManager.waitForPacket();
}

void Rover::sendManualMsg(char* msg){
//     // String RHMessageStr = "";
//     StaticJsonDocument<JSON_OBJECT_SIZE(3)> testdoc;
//     JsonObject RHMessageObject = testdoc.to<JsonObject>();
//     RHMessageObject["TYPE"] = "DEBUG";
//     RHMessageObject["MSG"] = msg;

//     // uint8_t* processedRHMessage = reinterpret_cast<uint8_t*>((char *)RHMessageStr.c_str());
//     char processedRHMessage[255];
//     serializeJson(RHMessageObject, processedRHMessage);
//     Serial.println(processedRHMessage);
//     m_RHManager.sendtoWait((uint8_t*)processedRHMessage, measureJson(RHMessageObject), SERVER_ADDR);
//     // Serial.println(status);
}

String Rover::getMessageType(){
    
}
void Rover::debugRTCPrint(){
    Wire.begin();
    if(m_RTC.begin()) {
        m_RTC.adjust(DateTime(F(__DATE__), F(__TIME__)));//set date-time manualy:yr,mo,dy,hr,mn,sec   
        Serial.println("RTC connected...");
    } else {
        Serial.println("RTC failed...");
    }
}

void fire_int(){
    detachInterrupt(digitalPinToInterrupt(5)); //use built-in led to diagnose
    digitalWrite(LED_BUILTIN, HIGH);
}

// https://forum.arduino.cc/t/ds3231-read-time-error/909413/3
void Rover::printRTCTime_Ben() {
    char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
    byte myHour = nowDT.hour();
    byte myMin = nowDT.minute(); //to show leading zero of minute
    byte mySec = nowDT.second();
    byte myDay = nowDT.day();
    byte myMonth = nowDT.month();
    
    nowDT = m_RTC.now();

    if (myHour < 10) {
        Serial.print('0');
    }
    Serial.print(myHour); Serial.print(':'); //12:58:57     

    if (myMin < 10) {
        Serial.print('0');
    }
    Serial.print(nowDT.minute()); Serial.print(':');
    //--------------------------------------------------
    if (mySec < 10) {
        Serial.print('0');
    }
    Serial.print(mySec);//(nowDT.second(), DEC);
    //----------------------------------------- `
    Serial.print("  ");
    Serial.print(daysOfTheWeek[nowDT.dayOfTheWeek()]); Serial.print(' ');
    
    if (myDay < 10) {
        Serial.print('0');
    }
    Serial.print(myDay); Serial.print(':');
    //-----------------------------------------------------
    if (myMonth < 10) {
        Serial.print('0');
    }
    Serial.print(nowDT.month()); Serial.print(':');
    Serial.println(nowDT.year());
}

// char *Rover::getTimeStamp() {
//     nowDT = m_RTC.now();
//     memset(m_timestamp, '\0', sizeof(char) * 512);
//     sprintf(m_timestamp, "%.2d.%.2d.%.2d.%.2d.%.2d", nowDT.month(), nowDT.day(),
//          nowDT.hour(), nowDT.minute(), nowDT.second());
  
//     //Serial.println(m_timestamp);
//     return m_timestamp;
// }

// int Rover::test() {
//     return 10;
// }

void Rover::timeDelay() {
  byte prSec = 0;
  prSec = bcdSecond(m_RTC);   //current second of RTC
  while (bcdSecond(m_RTC) == prSec) // != 1 )
  {
    ;
  }
  prSec = bcdSecond(m_RTC); //delay(1000);
}

byte Rover::bcdSecond(RTC_DS3231 rtc) {
  nowDT = rtc.now();
  if (nowDT.second() == 0 )
  {
    return 0;
  }
  else
  {
    return nowDT.second();
  }
}

void Rover::scheduleAlarm(int sec, Ds3231Alarm1Mode alarmMode) {
    m_RTC.clearAlarm(1);
    m_RTC.clearAlarm(2);

    m_RTC.writeSqwPinMode(DS3231_OFF);

    // turn off alarm 2 (in case it isn't off already)
    // again, this isn't done at reboot, so a previously set alarm could easily go overlooked
    m_RTC.disableAlarm(2);

    ////set alarm and configure alarm to activate every time minutes and seconds match
    if (!m_RTC.setAlarm1( m_RTC.now() + TimeSpan(sec), alarmMode)) { 
        Serial.println("Error, no alarm set!");
    }
    else {
        Serial.print("Alarm will happen in ");
        Serial.print((String)sec);
        Serial.println(" seconds!");
    }
}

void Rover::scheduleRTKAlarm(Ds3231Alarm1Mode alarmMode){
    scheduleAlarm(INIT_WAKETIME*60, alarmMode);
}

void Rover::scheduleSleepAlarm(Ds3231Alarm1Mode alarmMode){
    scheduleAlarm(INIT_SLEEPTIME*60, alarmMode);
}

void Rover::attachAlarmInterrupt(){
    attachInterrupt(digitalPinToInterrupt(RTC_INT), fire_int, LOW);
    attachInterrupt(digitalPinToInterrupt(RTC_INT), fire_int, LOW);     //@will richards. attached twice because it works?     
}

void Rover::startFeatherTimer(){
    this->startTime = millis();
}

void Rover::setFeatherTimerLength(int milliseconds){
    this->featherTimerLength = milliseconds;
}

bool Rover::isFeatherTimerDone(){
    if((unsigned long)(millis() - this->startTime) >= this->featherTimerLength){ //calculate if current time exceeds the set timer 
        return true;
    }else{
        return false;
    }
}
void Rover::toSleep(){
    /* Standard sleep code for microcontrollers */
    digitalWrite(LED_BUILTIN, LOW);
    __DSB();    //data sync bus function for stability, refer to m0 cpu documentation for more info
    __WFI();    //wait for interrupt/puts device to sleep
    digitalWrite(LED_BUILTIN, HIGH);
}

//prototype TODO fix this poor placement
uint8_t len = RH_SERIAL_MAX_MESSAGE_LEN;
char m_buf[RH_SERIAL_MAX_MESSAGE_LEN];

// bool Rover::listen(){
//     if(m_RHManager.available()){
//         if (m_RHManager.recvfromAckTimeout((uint8_t *)m_buf, &len, m_rovInfo.init_retries, 0))
//             Serial.println(m_buf);
//             return true;
//     }
//     return false;
// }

void Rover::powerRadio(){
    Serial.println("Powering radio on.");
    m_max4820.assertRail(0);
}

void Rover::powerDownRadio(){
    Serial.println("Powering radio down.");
    m_max4820.assertRail(1);
}

void Rover::powerGNSS(){
    Serial.println("Powering GNSS on.");
    m_max4820.assertRail(2);
}

void Rover::powerDownGNSS(){
    Serial.println("Powering GNSS down.");
    m_max4820.assertRail(3);
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

void Rover::setRS232(bool enable){
    (enable) ? m_max3243.enable() : m_max3243.disable(); //ternary operator
}


