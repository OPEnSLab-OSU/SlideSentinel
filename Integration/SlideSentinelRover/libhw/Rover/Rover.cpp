#include "Rover.h"
#include "pcb_2.0.0.h"


/* Ran on first bootup of Main*/
Rover::Rover() : max4280(MAX_CS, &SPI){}

void Rover::powerRadio(){
    Serial.println("Powering radio on.");
    max4280.assertRail(0);
}

void Rover::powerDownRadio(){
    Serial.println("Powering radio down.");
    max4280.assertRail(1);
}

void Rover::powerGNSS(){
    Serial.println("Powering GNSS on.")
      m_max4280.assertRail(2);
}

void Rover::powerDownGNSS(){
    Serial.printlnt("Powering GNSS down.")
    m_max4280.assertRail(3);

}