#include "FSController.h"
#include "Console.h"

FSController::FSController(uint8_t cs, uint8_t rst, int num_rovers)
    : m_cs(cs), m_rst(rst), m_num_rovers(num_rovers) {}

bool FSController::init() {
  pinMode(m_cs, OUTPUT);
  if (!m_sd.begin(m_cs, SD_SCK_MHZ(50)) || !m_root.open("/"))
    return false;

  if (!m_sd.exists(MAIN) && !m_sd.mkdir(MAIN)) {
    console.debug("failed to create");
    return false;
  }

  m_setup();
  console.debug("FSController initialized.\n");
  return true;
}

void FSController::m_clear() { memset(m_buf, '\0', sizeof(char) * BUF_SIZE); }

bool FSController::m_setup() {

  for (int i = 0; i < m_num_rovers; i++) {
    m_clear();
    sprintf(m_buf, "ROVER%d", i);
    console.debug("\n");
    console.debug(m_buf);
    m_curDir = m_buf;

    // make data.json, diag.json, props.json for each rover
    if (!(m_sd.chdir() && m_sd.chdir(MAIN) && m_sd.mkdir(m_curDir) &&
          m_sd.chdir(m_curDir) && m_mkFile(PROPS) && m_mkFile(DIAG) &&
          m_mkFile(DATA) && m_mkFile(ERROR)))
      return false;
  }
  return true;
}

bool FSController::m_mkFile(const char *name) {
  if (!(m_file.open(name, O_WRONLY | O_CREAT | O_EXCL) && m_file.close()))
    return false;
  return true;
}

bool FSController::m_write(char *msg) { return m_file.println(msg); }

bool FSController::m_chgDir(int rover_num) {
  m_clear();
  sprintf(m_buf, "ROVER%d", rover_num);
  m_curDir = m_buf;
  if (!(m_sd.chdir() && m_sd.chdir(MAIN) && m_sd.chdir(m_curDir)))
    return false;
  return true;
}

bool FSController::m_setFile(int rover_num, const char *file) {
  if (!(m_chgDir(rover_num) && m_file.open(file, O_WRONLY | O_APPEND)))
    return false;
  return true;
}

bool FSController::m_log(int rover_num, const char *msg, const char *file) {
  if (!(m_setFile(rover_num-1, file) && m_write((char *)msg))) {
    m_file.close();
    return false;
  }
  m_file.close();
  return true;
}

void FSController::logData(int rover_num, char *data) {
  if (!m_log(rover_num, data, DATA))
    m_log(rover_num, WRITE_ERR, ERROR);
}

void FSController::logDiag(int rover_num, char *diag) {
  if (!m_log(rover_num, diag, DIAG))
    m_log(rover_num, WRITE_ERR, ERROR);
}

void FSController::logProps(int rover_num, char *props) {
  if (!m_log(rover_num, props, PROPS))
    m_log(rover_num, WRITE_ERR, ERROR);
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

void FSController::status(BaseModel &model) {}
void FSController::update(BaseModel &model) {}