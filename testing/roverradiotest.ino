#include <SPI.h>
#define _BV(bit) (1 << (bit))
int incomingByte = 0; // for incoming serial data
int m_cs = 9;
uint8_t m_clear = 0x00;
 SPIClass *m_spi;
void setup() {
   m_spi = &SPI;  
  // put your setup code here, to run once:
  Serial1.begin(115200);
  Serial.begin(115200);
  pinMode((uint8_t)A5,OUTPUT);
  digitalWrite((uint8_t)A5,HIGH);
  pinMode(14,OUTPUT);
  digitalWrite(14, LOW);
  pinMode(13, OUTPUT);//polulu
  digitalWrite(13, HIGH);
  pinMode(m_cs,OUTPUT);//max4280
   // SPI INIT
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV8);
  assertRail(1);
  delay(500);
  assertRail(0);
}

int last_loop = millis();
int t_elapsed = 0;
int loop_num = 0;

void loop() {
  
  // send data only when you receive data:
  if (Serial1.available() > 0) {
    
//     Serial.print(Serial1.available());
    do {
      incomingByte = Serial1.read();
      Serial.print((char)incomingByte);
    }while(Serial1.available() > 0);
//    Serial.println();
    // read the incoming byte:
    
  }else{
    t_elapsed += millis()-last_loop;
    last_loop = millis();
    if(t_elapsed > 1000){
      Serial.println("Test");
      Serial1.print("This is a test to check communication reliability. GPS:152.3344556677,53.1122334455, loop num: ");
      Serial1.println(loop_num);
      t_elapsed=0;
      loop_num++;

    }
  }
  
}
void assertRail(uint8_t num) {
  digitalWrite(m_cs, LOW);
  m_spi->transfer(_BV(num));
  digitalWrite(m_cs, HIGH);
  delay(8);
  digitalWrite(m_cs, LOW);
  m_spi->transfer(m_clear);
  digitalWrite(m_cs, HIGH);
}

//void loop() {
//  
//  if(Serial1.available())
//  Serial1.println("testCode");
//  delay(500);
//  // put your main code here, to run repeatedly:
//
//}