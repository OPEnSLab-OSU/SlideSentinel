#pragma once

#ifdef UNIT_TEST
#ifdef ARDUINO
#include <Arduino.h>
#include <FeatherFault.h>
#include "unity.h"

void setup() {
    Serial.begin(115200);
    while(!Serial)
        yield();

    if (FeatherFault::DidFault()) {
        FeatherFault::PrintFault(Serial);
        while(true)
            yield();
    }

    process();
}

void loop() {
    MARK;
    digitalWrite(13, HIGH);
    delay(100);
    digitalWrite(13, LOW);
    delay(500);
}

#else // ARDUINO

int main(int argc, char **argv) {
    process();
    return 0;
}

#endif // ARDUINO
#endif // UNIT_TEST