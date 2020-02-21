#include "FSController.h"
#include "Console.h"

FSController::FSController(uint8_t cs, uint8_t rst)
    : Controller("FS"), m_cs(cs), m_rst(rst),
      m_WRITE_ERR("{\"ID\":\"ERR\",\"MSG\":\"WRITE ERROR\"}"),
      m_GNSS("gnss.csv"), m_STATE("state.csv"), m_ERR("err.txt") {}

void FSController::m_clearBuffer() {
  memset(m_buf, '\0', sizeof(char) * MAX_PATH_SIZE);
}

// NOTE FAT16 can only have 512 entries in root, but can have 65,534 entries in
// any subdirectory
bool FSController::init() {
  pinMode(m_cs, OUTPUT);
  if (!m_sd.begin(m_cs, SD_SCK_MHZ(50)))
    return false;
  if (!m_root.open("/")) // create root
    return false;
  if (!m_sd.mkdir("logs")) // create system wide logs
    return false;
  if (!m_sd.mkdir("data")) // create system wide data logs
    return false;
  return true;
}

bool FSController::m_dispatch(JsonDocument &doc) {
  const char *type = doc["ID"];
  if (strcmp(type, "ERR") == 0)
    return m_logErr(doc["MSG"]);
  if (strcmp(type, "GNSS") == 0)
    return m_logGNSS(doc["MSG"]);
  if (strcmp(type, "STATE") == 0)
    return m_logState(doc["MSG"]);
  return false;
}

void FSController::log(JsonDocument &doc) {
  if (!m_dispatch(doc))
    m_logErr(m_WRITE_ERR);
  doc.clear();
}

// TODO error check and make sure the directory is not already made
// TODO clean up error handling in this controller
// TODO construct global state object
bool FSController::newCycle(char *timestamp) {
  if (!m_sd.chdir()) // reset file ptr
    console.debug("failed to change dir to root");
  if (!m_sd.chdir("data")) // change to data dirå
    console.debug("failed to change dir to data");
  if (!m_sd.mkdir(timestamp)) // create timestamped folder for wake cycle
    console.debug("failed to make timestampped dir");
  if (!m_sd.chdir(timestamp)) // change to data dir
    console.debug("failed to change dir to timestampped dir");
  if (!m_mkFile(m_GNSS)) // create GNSS file
    console.debug("failed to GNSS file");
  if (!m_mkFile(m_STATE)) // create state file
    console.debug("failed to STATE file");
  if (!m_mkFile(m_ERR)) // create error logs
    console.debug("failed to ERROR file");
  if (!m_setFile(m_GNSS)) // set the file ptr to GNSS
    console.debug("failed to set file ptr to gnss");
  m_writeHeader();     // write the data header
  if (!m_file.close()) // close the ptr
    console.debug("failed to close GNSS file");
  m_sd.ls();
  return true;
}

bool FSController::m_mkFile(const char *name) {
  if (!m_file.open(name, O_WRONLY | O_CREAT | O_EXCL))
    return false;
  if (!m_file.close())
    return false;
  return true;
}

bool FSController::m_setFile(const char *file) {
  if (!m_file.open(file, O_WRONLY | O_APPEND))
    return false;
  return true;
}

void FSController::m_writeHeader() {
  m_file.print(F("Week,Seconds,Latitude,Longitude,Height,Satellites,BNorth,"
                 "BEast,BDown,VNorth,VEast,VDown,GDOP,HDOP,PDOP,TDOP,VDOP\n"));
}

// TODO implement these
bool FSController::m_logErr(const char *err) {}
bool FSController::m_logState(const char *state) {}
bool FSController::m_logGNSS(const char *gnss) {}

void FSController::update(JsonDocument &doc) {}
void FSController::status(uint8_t verbosity, JsonDocument &doc) {}