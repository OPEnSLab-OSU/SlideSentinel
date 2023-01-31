#include "Rover.h"
#include <Wire.h>

Rover::Rover(Uart& ser) :    m_max4820(MAX_CS, &SPI), 
                    m_max3243(FORCEOFF_N),
                    m_multiplex(SPDT_SEL, -1),
                    m_RadioManager(),
                    m_gnss(ser, GNSS_BAUD, GNSS_RX, GNSS_TX, INIT_LOG_FREQ),
                    m_JSONData(1024) {
    m_rovInfo.id = CLIENT_ADDR;
    m_rovInfo.serverAddr = SERVER_ADDR;
    m_rovInfo.init_retries = INIT_RETRIES;
    m_rovInfo.timeout = INIT_TIMEOUT;
}

void Rover::initRTC(){
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk; //enable deep sleep mode
    pinMode(RTC_INT,INPUT_PULLUP);
    
    m_RTC.begin();
    m_RTC.adjust(DateTime(F(__DATE__), F(__TIME__)));//set date-time manualy:yr,mo,dy,hr,mn,sec   
}

/**
 * Initialize the rover
 */ 
bool Rover::initRover(){
    initRTC();
    setRS232(IS_RS232);
    m_RadioManager.initRadio();
    m_gnss.init();
    return true;
}

void Rover::wake(){
    powerRadio();
    for (int i = 0; i < 2; i++) {
        Serial.println(i);
        Serial.println(" ");
        delay(1000);
    } 
}
bool Rover::poll(){

    // If we have a fix we don't need to continue polling
    if(m_gnss.hasFix())
        return true;

    // Poll data from the GNSS
    m_gnss.poll();

    // If not a fix check if we have new data and then set the current values to the values of lastPoll
    if(m_gnss.isNewData()){
        m_gnss.populateGNSS();
        Serial.println(m_gnss.getGNSSData());
    }
    return false;
}

/* Fixed common type data package (REQUEST, UPLOAD, etc.)*/
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
            RHJson["MSG"] = serialized(m_gnss.getGNSSData());
            break;
        case ALERT:
            RHJson["TYPE"] = "ALERT";
            RHJson["MSG"] = "UNSPECIFIED_ALERT";
            break;

    }
}

/* Arbitrary messages for type and message, manual*/
void Rover::packageData(String type, String message){
    JsonObject RHJson = m_JSONData.to<JsonObject>();
    RHJson["TYPE"] = type;
    RHJson["MSG"] = message;
}

/* Transmit packaged data to the base*/
bool Rover::transmit(){
    
    //TDL: Conditionally enable max3243
    setMux(RadioToFeather);
    delay(5);
    
    //Stringify the json message 
    String processedRHMessage;
    serializeJson(m_JSONData, processedRHMessage);
    Serial.println(processedRHMessage);

    // Use our radio to send the packet to the server
    return m_RadioManager.sendPacket(processedRHMessage, SERVER_ADDR);
}

/* Wait for data from the base and read the contents in */
bool Rover::waitAndReceive(int milliseconds){
    if(!m_RadioManager.waitForPacket(milliseconds)){
        Serial.println("[Rover] No Packet Received!");
        return false;
    }
    
    Serial.println("[Rover] Packet Received!");
    return m_RadioManager.readHeader();
}

/* Get the message type from the last received packet */
String Rover::getMessageType(){
    return m_RadioManager.getRoverPacket()["TYPE"];
}

/* Get the message body from the last received packet */
String Rover::getMessageBody(){
    return m_RadioManager.getRoverPacket()["MSG"];
}

void Rover::setRTCTime(){
    // Initialize Wire and start communication with the RTC
    Wire.begin();
    if(m_RTC.begin()) {

        // Set the RTC time to the programed time
        m_RTC.adjust(DateTime(F(__DATE__), F(__TIME__)));//set date-time manualy:yr,mo,dy,hr,mn,sec   
        Serial.println("[Rover] RTC time Set!");
    } else {
        Serial.println("[Rover] RTC time failed to set");
    }
}

/* ISR for handling the RTC interrupt*/
void RTCInterruptISR(){
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
    attachInterrupt(digitalPinToInterrupt(RTC_INT), RTCInterruptISR, LOW);
    attachInterrupt(digitalPinToInterrupt(RTC_INT), RTCInterruptISR, LOW);     //@will richards. attached twice because it works?     
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

/**
 * Reads bytes in from the Serial bus to print out requested data
 */ 
void Rover::debugInformation(){

    // Check for user input on serial to request information about the base
    if(Serial.available()){
        char cmd = Serial.read();
        if(cmd == '1'){
            printDiagnostics();
        }
    }
}

/**
 * Print the base's diagnostic and just general information to the serial bus
 */ 
void Rover::printDiagnostics(){

    //Print Basic Configuration Information
    Serial.println("\n**** Configuration ****");
    Serial.println("\tBase ID: " + String(m_rovInfo.id));
    Serial.println("\tRetry Count: " + String(m_rovInfo.init_retries));
    Serial.println("\tRadio Baud Rate: " + String(m_rovInfo.radioBaud));
    Serial.println("\n*************************");

    // Print out the diagnostics to serial
    m_roverDiagnostics.print_serial();
}


