#include "FSController.h"
#include "Console.h"

FSController::FSController(uint8_t cs, uint8_t rst)
    : Controller("FS"), m_cs(cs), m_rst(rst), m_DATA("data.json"),
      m_DIAG("diag.json"), m_cycle(0) {}

// NOTE FAT16 can only have 512 entries in root, but can have 65,534 entries in
// any subdirectory
bool FSController::init() {
  pinMode(m_cs, OUTPUT);
  // set clock rate, open root directory, check if MAIN exists, if not create it
  if (!m_sd.begin(m_cs, SD_SCK_MHZ(50)) || !m_root.open("/") ||
      (!m_sd.exists(MAIN) && m_sd.mkdir(MAIN)))
    return false;
  console.debug("FSController initialized.\n");
  return true;
}

void FSController::logData(char *data) {
  if (!m_logMsg(data, m_DATA))
    m_logMsg((char *)WRITE_ERR, m_DIAG);
}

void FSController::logDiag(char *data) {
  if (!m_logMsg(data, m_DIAG))
    m_logMsg((char *)WRITE_ERR, m_DIAG);
}

// TODO maintian a way to determine if SD failed and reactivley reattempt to
// reinit()
bool FSController::setupWakeCycle(char *timestamp, char *format) {
  m_cycles();

  // check if we wok up instantaneously, might occur due to accelerometer
  if (m_sd.exists(timestamp) && m_sd.chdir(timestamp) && m_setFile(m_DATA))
    return true;

  // reset ptr, enter "/data", cerify timestampped dir does not exists, make
  // timestamped dir, enter /data/<TIMESTAMP_DIR>, make gnss.csv and log.txt,
  // set file ptr gnss.csv
  if (!(m_sd.chdir() && m_sd.chdir(MAIN) && m_sd.mkdir(timestamp) &&
        m_sd.chdir(timestamp) && m_mkFile(m_DATA) && m_mkFile(m_DIAG) &&
        m_setFile(m_DATA)))
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
  console.debug("\n\n");
  console.debug(file);
  console.debug("\n\n");
  console.debug(msg);
  console.debug("\n\n");
  if (!(m_setFile(file) && m_write((char *)msg))) {
    m_file.close();
    return false;
  }
  m_file.close();
  return true;
}

void FSController::m_SDspace() {
  float cardSize = m_sd.card()->cardSize() * 0.000512;
  m_spaceMB = cardSize - m_sd.vol()->freeClusterCount() *
                             m_sd.vol()->blocksPerCluster() * 0.000512;

  // Serial.printf(F(" Size: %d.%02d MB"), (unsigned int)cardSize,
  //               (unsigned int)(cardSize * 100.0) % 100);

  // Serial.printfn(F(" (Used: %d.%02d MB)."), (unsigned int)used,
  //                (unsigned int)(used * 100.0) % 100);
}

void FSController::m_cycles() { m_cycle++; }

void FSController::status(SSModel &model) {
  model.statusFS(m_spaceMB, m_cycle);
}

void FSController::update(SSModel &model) {}