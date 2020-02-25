/*
- round corners of PCB
- mount Piksi directly to PCB
- mount ZED-F9P directly to PCB, FTDI for uploading firmware
- rotate Feather for easier micro-usb access
- relocate SD Card
- permanent SD Card
- remove 3.3v to reset on radio (let it float)
- single jumper block for Z9-C/Z9-T toggle
- expose TX2 on header block
*/


StaticJsonDocument<RH_SERIAL_MAX_MESSAGE_LEN> mail; // global messaging variable

enum State {
  WAKE,
  HANDSHAKE,
  UPDATE,
  POLL,
  UPLOAD,
  SLEEP
}

enum Verbosity {
  QUIET,
  NORMAL,
  VERBOSE,
  VERY_VERBOSE,
  DEBUG
}

Verbosity verbosity = NORMAL;

State state = WAKE; // global state variable

while (1) {
  switch (state) {
  case WAKE:
    FSController.setupWakeCycle(
        TimingController
            .getTimeStamp());   // create a new directory for this wake cycle
    PMCOntroller.enableRadio(); // turn on the radio
    state = HANDSHAKE;
  case HANDSHAKE:
    IMUController.getFlag(mail); // check if the Accelerometer woke the device
    if (!ComController.request(mail)) {
      FSController.log(mail); // if the request fails log the failure ERR
      state = SLEEP;
    }
    state = UPDATE;
    // TODO Immediate response when the rover wakes
  case UPDATE:
    // TODO central state object at global scope
    // NOTE how do we handle the fact that state variables such as m_timeout,
    // need to be configured in nested objects of the controllers? NOTE do we
    // have reinit() method? Store global state in an object?
    ConManager.update(mail);
    /* "Controller Manager"
     * Iterates over every controller, passing the config data collected from
     * the base during the request. Each Controller gets this data passed to its
     * .update() method which dynamically checks if there is data for the
     * particualr controller in the packet. If there is data for the controller,
     * the controller updates its internal variables thus changing the behavior
     * of the device.
     */
    PMController.enableGNSS(); // Turn on the GNSS receiver
    TimingController.start();  // start the millis() timer
    state = POLL;

  case POLL:
    if (GNSSController.poll(mail))
      FSController.log(mail); // if so log the data, DAT

    if (TimingController.done()) // check if the millis timer is done
      state = UPLOAD;

  case UPLOAD:
    PMController.disableGNSS();
    // clear mail
    ConManager.status(mail, verbosity);
    /*
     * Collect the message to send to the base. Iterates over every controller,
     * and checks for data calling the base class Controller.status() method.
     * For example the GNSSController.status() will append any RTK positional
     * info to the packet for the wake cycle. If the verbosity flag is in debug
     * though the GNSSController may include hdop and satellites view values to
     * the packet for greater information about the rover. The PMController may
     * append the battery voltage to the packet as another example.
     */

    FSController.log(mail);          // log the packet collected
    if (!ComController.upload(mail)) // attempt to upload the packet to the base
      FSController.log(mail); // if a failure occurs mail is populated with
                              // error message, log this to SD

    state = SLEEP;
  case SLEEP:
    PMController.disableRadio(); // turn of off the radio
    TimingController.setAlarm(); // set the alarm to wake up
    PMController.sleep();        // go to sleep
    state = WAKE;
  }
}

/* POSSIBLE MESSAGE ID's
RTS,            // request for service
  - ROV: [rover ID]
  - MSG: [Wake type]
RES,           // conifguration data
  - MSG: []     
GNSS,           // data collected by the receiver
  - MSG: [position data]
LOG,          // state data collected, error messages in the system
  - MSG: state or err string
PKT,          // condensed data packet 
  - MSG: all data aggregated into the packet
*/

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
// packet prior to sending
/* STATUS
FSController        [available SD card space, number of wake cycles to date]
IMUController       [accelerometer sensitivity]
TimingController    [wake duration, sleep duration]
ComController       [retries, timeout, number of dropped packets]
PMController        [battery voltage]
GNSSController      [hdop, pdop, satellites view during the wake cycle, report
resolution (maybe the user is fine with RTK float data)] ConManager [verboisy]
<--- We could migrate the verbosity to the ConManager, instead of making it
global
*/

/*




*/
