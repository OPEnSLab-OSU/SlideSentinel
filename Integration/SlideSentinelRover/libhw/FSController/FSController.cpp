#include "FSController.h"
#include "Console.h"
#include "FeatherTrace.h"

FSController::FSController(uint8_t cs, uint8_t rst)
    : m_cs(cs), m_rst(rst), m_did_begin(false), m_DATA("data.json"), m_DIAG("diag.json"),
      m_cycle(0), m_spaceMB(0) {}

// NOTE FAT16 can only have 512 entries in root, but can have 65,534 entries in
// any subdirectory
bool FSController::init() {
  pinMode(4, OUTPUT); MARK;

  // Initializes SD card
  if (!m_sd.begin(4, SD_SCK_MHZ(10))) {
    Serial.println("Card failed, or not present");
    return false;
  } 
  else {
    Serial.println("FSController initialized!");
  }

  return true;
}


void FSController::logData(char *data) {
  if (!m_logMsg(data, m_DATA)) {
    m_logMsg((char *)WRITE_ERR, m_DATA);
  }
}

void FSController::logDiag(char *data) {
  if (!m_logMsg(data, m_DIAG))
    m_logMsg((char *)WRITE_ERR, m_DIAG);
}

// TODO maintian a way to determine if SD failed and reactivley reattempt to
// reinit()
bool FSController::setupWakeCycle(char *timestamp, char *format) { MARK;
  Serial.println("Creating new wake cycle directory: ");
  Serial.println(timestamp);
  Serial.println(format);
  m_curDir = timestamp;
  m_cycles();

  // check if we wok up instantaneously, might occur due to accelerometer
  if (m_sd.exists(m_curDir) && m_sd.chdir(m_curDir))
    return true;
 
  m_mkFile(m_DATA);

  m_file.open(m_DATA, O_WRONLY | O_APPEND);
  m_write(format);     // write the data header
  if (!m_file.close()) // close the ptr
    return false;
  return true;
}

bool FSController::m_mkFile(const char *name) { MARK;
  if (!(m_file.open(name, O_WRONLY | O_CREAT | O_EXCL) && m_file.close()))
    return false;
  return true;
}

bool FSController::m_setFile(const char *file) { MARK;
  // if (!(m_sd.chdir() && m_sd.chdir(MAIN) && m_sd.chdir(m_curDir) &&
  //     m_file.open(file, O_WRONLY | O_APPEND)))
  //   return false;
  if (!(m_file.open(file, O_WRONLY | O_APPEND)))
    return false;

  return true;
}

bool FSController::m_write(char *msg) { MARK; return m_file.println(msg); }

bool FSController::m_logMsg(const char *msg, const char *file) { MARK;
  Serial.print("Writing ");
  Serial.print(msg);
  Serial.print(" to file ");
  Serial.print(file);
  Serial.println("");
  //(char *)msg))
  if (!(m_setFile(file) && m_write((char *)msg))) {
    Serial.println("Failed to write...");
    m_file.close();
    return false;
  }
  m_file.close();
  return true;
}

void FSController::m_SDspace() { MARK;
  m_spaceMB = (m_sd.vol()->freeClusterCount() *
  (m_sd.vol()->blocksPerCluster() / 2)) / 1024;
  MARK;
}

void FSController::m_cycles() { m_cycle++; }

bool FSController::check_init() {return m_did_begin; }

void FSController::status(SSModel &model) {
  if (m_did_begin){
    m_SDspace();
    model.setSpace(m_spaceMB);
    model.setCycles(m_cycle);
  }
  
}

void FSController::update(SSModel &model) {}