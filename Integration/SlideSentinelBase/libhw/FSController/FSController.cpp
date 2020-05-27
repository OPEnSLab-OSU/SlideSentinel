#include "FSController.h"
#include "Plog.h"

FSController::FSController(uint8_t cs, uint8_t rst, int num_rovers)
    : m_cs(cs), m_rst(rst), m_num_rovers(num_rovers), m_last_sd_error(0) {}

bool FSController::init() {
  pinMode(m_cs, OUTPUT);
  if (!m_sd.begin(m_cs, SD_SCK_MHZ(1)) || !m_root.open("/")) {
    LOGE << "Failed to initialize SD";
    return false;
  }

  if (!m_sd.exists(MAIN) && !m_sd.mkdir(MAIN)) {
    LOGE << "Failed to find/make root directory";
    return false;
  }

  m_setup();
  LOGD << "FSController initialized.";
  return true;
}

void FSController::m_clear() { memset(m_buf, '\0', sizeof(char) * BUF_SIZE); }

bool FSController::m_setup() {

  for (int i = 0; i < m_num_rovers; i++) {
    m_clear();
    sprintf(m_buf, "ROVER%d", i);
    LOGD << m_buf;
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
  if (!(m_setFile(rover_num - 1, file) && m_write((char *)msg))) {
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
  uint32_t cardSize = m_sd.card()->cardSize() * 0.000512;
  uint32_t volFree = m_sd.vol()->freeClusterCount();
  m_spaceMB = 0.000512 * volFree * m_sd.vol()->blocksPerCluster();
  m_spaceMB = ((cardSize- m_spaceMB) / cardSize) * 100;
}

void FSController::status(BaseModel &model) {
  m_SDspace();
  model.setSdSpace(m_spaceMB);
  model.setSdError(m_last_sd_error);
}

void FSController::checkSD() {
  if (m_sd.cardErrorCode()) {
    if (m_sd.cardErrorCode() != m_last_sd_error) {
      m_last_sd_error = m_sd.cardErrorCode();
      PLOGE << "Card error: " << (int)m_last_sd_error << " Data: " << m_sd.cardErrorData();
    }
    m_sd.begin(m_cs, SD_SCK_MHZ(1));
  }
}

void FSController::update(BaseModel &model) {}