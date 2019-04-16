void setup() {
  Serial.begin(19200);

  //communication channel to the ROCKBLOCK+
  Serial.println("Starting Serial Connection wih RS232 interface...");
  Serial1.begin(19200);
  //the configuration parameter to Serial.begin() are the default values,
  //included here explicity due to ROCKBLOCK+ serial specifications
}

void loop() {
  Serial1.print('@');
  delay(100);
}
