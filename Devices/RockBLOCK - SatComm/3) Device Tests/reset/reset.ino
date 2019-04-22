#include "IridiumSBD.h"
#define IridiumSerial Serial1
#define ON_OFF 9

IridiumSBD modem(IridiumSerial);

void setup()
{
    int err;
    while (!Serial)
        ;
    Serial.println("Turning the rockblock on...");
    digitalWrite(ON_OFF, HIGH);
    IridiumSerial.begin(19200);
    err = modem.begin();
    
    Serial.print("This is the error: ");
    Serial.println(err);

    delay(2000);
    digitalWrite(ON_OFF, LOW);
    Serial.println("flushed everything....");
}

void loop()
{
}