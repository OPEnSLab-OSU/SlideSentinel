#include "FSController.h"
#include "Console.h"

FSController::FSController(State *state, uint8_t cs, uint8_t rst)
    : Controller("FS", state), m_cs(cs), m_rst(rst),
      m_WRITE_ERR("{\"ID\":\"LOG\",\"MSG\":\"ERR: failed to write to SD\"}"),
      m_GNSS("gnss.csv"), m_LOG("log.txt") {}

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
  if (strcmp(type, "LOG") == 0)
    return m_logMsg(doc["MSG"], m_LOG);
  if (strcmp(type, "GNSS") == 0)
    return m_logMsg(doc["MSG"], m_GNSS);
  return false;
}

void FSController::log(JsonDocument &doc) {
  if (!m_dispatch(doc))
    m_logMsg((char *)m_WRITE_ERR, m_LOG);
  doc.clear();
}

// TODO error check and make sure the directory is not already made, maybe the
// accelerometer triggers
// TODO maintian a way to determine if SD failed and reactivley reattempt to
// reinit()
bool FSController::setupWakeCycle(char *timestamp, char *format) {
  // reset ptr, enter "/data", make timestamped dir, enter
  // /data/<TIMESTAMP_DIR>, make gnss.csv and log.txt, set file ptr gnss.csv
  if (!(m_sd.chdir() && m_sd.chdir("data") && m_sd.mkdir(timestamp) &&
        m_sd.chdir(timestamp) && m_mkFile(m_GNSS) && m_mkFile(m_LOG) &&
        m_setFile(m_GNSS)))
    return false;

  m_write(format);     // write the data header
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

bool FSController::m_write(char *msg) { return m_file.println(msg); }

bool FSController::m_logMsg(const char *msg, const char *file) {
  console.debug(file);
  console.debug(msg);
  if (!(m_setFile(file) && m_write((char *)msg))) {
    m_file.close();
    return false;
  }
  m_file.close();
  return true;
}

void FSController::status(uint8_t verbosity, JsonDocument &doc) {}