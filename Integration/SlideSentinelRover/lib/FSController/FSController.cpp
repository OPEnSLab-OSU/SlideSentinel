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
  if (!(m_sd.begin(m_cs, SD_SCK_MHZ(50)) && m_root.open("/") &&
        m_sd.mkdir("logs") && m_sd.mkdir("data")))
    return false;
  return true;
}

bool FSController::m_dispatch(JsonDocument &doc) {
  serializeJsonPretty(doc, Serial);
  const char *type = doc["ID"];
  if (strcmp(type, "ERR") == 0)
    return m_logMsg(doc["MSG"], m_ERR);
  if (strcmp(type, "GNSS") == 0)
    return m_logMsg(doc["MSG"], m_GNSS);
  if (strcmp(type, "STATE") == 0)
    return m_logMsg(doc["MSG"], m_STATE);
  return false;
}

void FSController::log(JsonDocument &doc) {
  if (!m_dispatch(doc))
    m_logMsg((char *)m_WRITE_ERR, m_ERR);
  doc.clear();
}

// TODO error check and make sure the directory is not already made
bool FSController::setupWakeCycle(char *timestamp) {
  // reset ptr, enter "/data", make timestamped dir, enter /data/[TIMESTAMP],
  // make GNSS, STATE and ERR files, set file ptr to current dir
  if (!(m_sd.chdir() && m_sd.chdir("data") && m_sd.mkdir(timestamp) &&
        m_sd.chdir(timestamp) && m_mkFile(m_GNSS) && m_mkFile(m_STATE) &&
        m_mkFile(m_ERR) && m_setFile(m_GNSS)))
    return false;

  m_writeHeader();     // write the data header
  if (!m_file.close()) // close the ptr
    return false;
  return true;
}

bool FSController::m_mkFile(const char *name) {
  if (!(m_file.open(name, O_WRONLY | O_CREAT | O_EXCL) && m_file.close()))
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

bool FSController::m_write(char *msg) {
  console.debug(msg);
  return m_file.println(msg);
}

bool FSController::m_logMsg(const char *msg, const char *file) {
  console.debug(file);
  if (!(m_setFile(file) && m_write((char *)msg))) {
    m_file.close();
    return false;
  }
  m_file.close();
  return true;
}

void FSController::update(JsonDocument &doc) {}
void FSController::status(uint8_t verbosity, JsonDocument &doc) {}