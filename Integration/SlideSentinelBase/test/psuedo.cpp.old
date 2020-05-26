/*
- barrel power connector 
- ground copper exposed on drill holes in each corner
- round corners of PCB
- mount Piksi directly to PCB
- mount ZED-F9P directly to PCB, FTDI for uploading firmware
- rotate Feather for easier micro-usb access
- relocate SD Card
- permanent SD Card
- remove 3.3v to reset on radio (let it float)
- single jumper block for Z9-C/Z9-T toggle
- expose TX2 on header block
- Add current sensor:
http://ww1.microchip.com/downloads/en/DeviceDoc/20005386B.pdf
*/

Msg msg;

struct MSG2 {
  IMU_FLAG
  NODE_ID
  TIMEOUT
  RETIRES
  SENSTIVITY
  SLEEPTIME
  WAKETIME
}

// FIXME log the state we are in
// update state and get state function, for logging state
enum State {
  WAKE,
  HANDSHAKE,
  UPDATE,
  POLL,
  UPLOAD,
  SLEEP
}

State state = WAKE; // global state variable

while (1) {
  switch (state) {
  case WAKE:
    FSController.setupWakeCycle(TimingController.getTimeStamp());
    // create a new directory for this wake cycle
    PMCOntroller.enableRadio(); // turn on the radio
    state = HANDSHAKE;
  case HANDSHAKE:
    ConManager.status(msg);            // get the status of the system
    FSController.logDiag(msg);         // log the status
    if (!ComController.request(msg)) { // produce a request
      FSController.logDiag(msg.error());
      state = SLEEP;
    }
    state = UPDATE;

  case UPDATE:
    ConController.update(msg.toReq);
    PMController.enableGNSS(); // Turn on the GNSS receiver
    TimingController.start();  // start the millis() timer

  case POLL:
    if (GNSSController.poll(msg))
      FSController.logData(msg.data());

    if (TimingController.done()) // check if the millis timer is done
      state = UPLOAD;

  case UPLOAD:
    PMController.disableGNSS();
    // clear mail
    ConManager.status(msg);

    // log system state before sleeping
    FSController.logDiag(msg);
    /*
     * Collect the message to send to the base. Iterates over every
     * controller, and checks for data calling the base class
     * Controller.status() method. For example the GNSSController.status()
     * will append any RTK positional info to the packet for the wake cycle.
     * If the verbosity flag is in debug though the GNSSController may
     * include hdop and satellites view values to the packet for greater
     * information about the rover. The PMController may append the battery
     * voltage to the packet as another example.
     */

    if (!ComController.upload(
            msg)) // attempt to upload the packet to the base, convert to
                  // stack JSON, serialize, send in fragments
      FSController.logDiag(
          msg.error()); // if a failure occurs mail is populated with
                        // error message, log this to SD

    state = SLEEP;
  case SLEEP:
    PMController.disableRadio(); // turn of off the radio
    TimingController.setAlarm(); // set the alarm to wake up
    PMController.sleep();        // go to sleep
    state = WAKE;
  }
}

/*
 * ------- CONFIG MESSAGES, WHAT DO THEY CONTAIN -------
 * Config data and verbosity
 * ---> Force upload, high verbosity upload immediately
 *
 */

/* UPDATE
IMPLEMENT ERROR HANDLING ON THE ROVERS, CHECKCONFIG FUNCTION
FSController        []
IMUController       [accelerometer sensitivity]
TimingController    [wake duration, sleep duration]
ComController       [retries, timeout]
PMController        []
GNSSController      [log frequency]
ConManager          [verbosity]      <--- We could migrate the verbosity to the
ConManager, instead of making it global
*/

// Ther verbosity of the rover dictates how much of this data is appended to the
// packet prior to sending REVIEW How do we structure the data, maybe the state
// is sent during the RTS, then
/* STATUS
FSController        [available SD card space, number of wake cycles to date]
IMUController       [sensitivity]
TimingController    [wake duration, sleep duration]
ComController       [retries, timeout]
PMController        [battery voltage]
GNSSController      [hdop, pdop, satellites view during the wake cycle, report
resolution (maybe the user is fine with RTK float data)] ConManager [verboisy]
<--- We could migrate the verbosity to the ConManager, instead of making it
global
*/

/*




*/
