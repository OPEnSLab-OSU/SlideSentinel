#pragma once

#ifdef UNIT_TEST
#ifdef ARDUINO
#include <Arduino.h>
#include "FeatherTrace.h"
#include "unity.h"

FEATHERTRACE_BIND_ALL()

void setup() {
    Serial.begin(115200);
    while(!Serial)
        yield();

    if (FeatherTrace::DidFault()) {
        FeatherTrace::PrintFault(Serial);
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