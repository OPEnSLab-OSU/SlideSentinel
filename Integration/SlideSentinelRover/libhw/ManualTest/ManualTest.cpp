
void test();
void wait();

void wait() {
  Serial.println("waiting for input");
  while (!Serial.available()) MARK;
  while (Serial.available()) {
    Serial.read();
  }
}

void test() { MARK;
  char cmd = '\0';
  int count = 0;
  char test[] = "{\"TYPE\":\"ACK\",\"STATE\":[3000,-1,3,4,10,200000]}";

  if (Serial.available())
    cmd = Serial.read();

  switch (cmd) {
  case '1': MARK;// PMController
    Serial.println("\n\nTesting PMCOntroller"); MARK;
    delay(2000); MARK;
    Serial.println("Toggling GNSS");
    pmController.enableGNSS(); MARK;
    delay(2000); MARK;
    pmController.disableGNSS();
    Serial.println("Toggling RADIO");
    pmController.enableRadio(); MARK;
    delay(2000); MARK;
    pmController.disableRadio();
    Serial.print("Battery Voltage: ");
    Serial.println(pmController.readBatStr()); MARK;
    delay(2000); MARK;
    Serial.print("Sleeping Tap Device to wake.....:\n"); MARK;
    pmController.sleep();
    // ...
    delay(2000); MARK;
    Serial.println("Awake from sleep!"); MARK;
    delay(2000); MARK;
    Serial.println("Collecting Status");
    pmController.status(model); MARK;
    delay(2000); MARK;
    model.print();
    model.clear();
    break;

  case '2': MARK;
    Serial.println("\n\nTesting IMUCOntroller"); MARK;
    delay(2000); MARK;
    Serial.print("Collecting Status");
    imuController.status(model);
    model.print();
    model.clear();
    break;

  case '3': MARK;
    Serial.println("\n\nTesting RTCCOntroller"); MARK;
    delay(2000); MARK;
    Serial.print("Timestamp: ");
    Serial.println(rtcController.getTimestamp());
    Serial.println("Setting 1 min wake alarm");
    // internal testing method
    count = 0;
    rtcController.setWakeAlarm();
    while (1) { MARK;
      delay(1000); MARK;
      Serial.print(count);
      Serial.println(" seconds");
      count++;
      if (rtcController.alarmDone())
        break;
    }
    Serial.println("Wake alarm triggered!"); MARK;
    delay(2000); MARK;
    Serial.println("Setting 1 min poll alarm");
    count = 0;
    rtcController.setPollAlarm();
    while (1) { MARK;
      delay(1000); MARK;
      Serial.print(count);
      Serial.println(" seconds");
      count++;
      if (rtcController.alarmDone())
        break;
    }
    Serial.println("Poll alarm triggered!"); MARK;
    delay(2000); MARK;
    Serial.print("Collecting Status");
    rtcController.status(model);
    model.print();
    model.clear();
    break;

    // TODO update routine so we can dynamically change props and test
    // log frequency not effecting logging rate?
    // model not getting updated
  case '4': MARK;
    Serial.println("\n\nTesting GNSSController"); MARK;
    delay(2000); MARK;
    Serial.println("Polling for data");
    pmController.enableGNSS();
    rtcController.setPollAlarm();
    while (1) { MARK;
      if (gnssController->poll(model))
        model.print();
      if (rtcController.alarmDone())
        break;
    }
    pmController.disableGNSS();
    gnssController->status(model);
    model.print();
    model.clear();
    break;

  case '5': MARK;
    Serial.println("\n\nTesting FSController"); MARK;
    delay(2000); MARK;
    Serial.print("Creating new timestampped directory: "); MARK;
    delay(2000); MARK;
    Serial.println(rtcController.getTimestamp());
    if (fsController.setupWakeCycle(rtcController.getTimestamp(),
                                    gnssController->getFormat()))
      Serial.println("Timestampped directory created!");
    else {
      Serial.println("Throwing write error");
      model.setError(WRITE_ERR);
      Serial.println(model.toError());
      fsController.logDiag(model.toError());
    } MARK;
    delay(2000); MARK;
    Serial.println("Simulated Wake...");
    pmController.enableGNSS();
    rtcController.setPollAlarm();
    while (1) { MARK;
      if (gnssController->poll(model))
        fsController.logData(model.toData(0));
      if (rtcController.alarmDone())
        break;
    }
    pmController.disableGNSS();
    Serial.println("Wake Cycle complete!"); MARK;
    delay(2000); MARK;
    if (fsController.setupWakeCycle(rtcController.getTimestamp(),
                                    gnssController->getFormat()))
      Serial.println("Timestampped directory created!");
    else {
      Serial.println("Throwing write error");
      model.setError(WRITE_ERR);
      Serial.println(model.toError());
      fsController.logDiag(model.toError());
    } MARK;
    delay(2000); MARK;
    Serial.println("Throwing random error");
    delay(2000); MARK;
    model.setError(ACK_ERR);
    Serial.println(model.toError());
    fsController.logDiag(model.toError());
    Serial.println("Get state of device"); MARK;
    delay(2000); MARK;
    manager.status(model);
    Serial.println("Status of machine: ");
    model.print(); MARK;
    delay(2000); MARK;
    Serial.println("Created diagnostic and state packet:");
    Serial.println(model.toDiag());
    Serial.println(model.toProp());
    Serial.println("Writing data to SD..."); MARK;
    delay(2000); MARK;
    fsController.logDiag(model.toDiag());
    fsController.logDiag(model.toProp());
    Serial.println("Complete");
    break;

  case '6': MARK;
    Serial.println("\n\nTesting COMController"); MARK;
    delay(2000); MARK;
    Serial.println("Creating Request packet");
    manager.status(model);
    Serial.println(model.toDiag());
    comController->request(model); MARK;
    delay(2000); MARK;
    Serial.println("Creating Upload Packet");
    Serial.println(model.toData(3));
    comController->upload(model);
    model.clear();
    Serial.println("Handling response"); MARK;
    delay(2000); MARK;
    model.handleRes(test);
    Serial.println("Collecting status"); MARK;
    delay(2000); MARK;
    comController->status(model);
    model.print();
    model.clear();
    break;

  case '7': MARK;
    Serial.println("\n\nTesting COMController");
    Serial.println("Creating Request packet");
    manager.status(model);
    Serial.println(model.toDiag());
    comController->request(model);
    break;
  case '8': MARK;
    Serial.print("CD pin: ");
    if (comController->channelBusy(model))
      Serial.println("BUSY");
    else
      Serial.println("NOT BUSY");
    break;
  case 'a': MARK;
    max3243.enable();
    pmController.enableRadio();
    break;
  case 'c': MARK;
    pmController.disableRadio();
    max3243.disable();
    break;
  case 'd': MARK;
    Serial1.println(" MOOOSE MOOOSE MOOOSE MOOOSE  MOOOSE MOOOSE  MOOOSE "
                    "MOOOSE  MOOOSE MOOOSE  MOOOSE MOOOSE  MOOOSE MOOOSE ");
    break;
  case 'b': MARK;
    for (int i = 0; i < 2000; i++) { MARK;
      if (Serial1.available()) {
        Serial.print((char)Serial1.read());
      }
    }
    break;
/* Code for testing bad behavior
  case 'f': MARK;
    __builtin_trap();
    break;
  case 'h': MARK;
   while(true);
   break;
*/
  }
  MARK;
}