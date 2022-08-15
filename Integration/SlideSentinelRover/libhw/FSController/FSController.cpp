#include "FSController.h"
#include "Console.h"
#include "FeatherTrace.h"

FSController::FSController(uint8_t cs, uint8_t rst)
    : m_cs(cs), m_rst(rst), m_did_begin(false), m_DATA("data.json"), m_DIAG("diag.json"),
      m_cycle(0), m_spaceMB(0) {}


/* Initializes SD card */
bool FSController::init() {
  pinMode(4, OUTPUT); MARK; // SD card pin is 4

  if (!m_sd.begin(4, SD_SCK_MHZ(10))) { // If the SD isn't inserted then fail
    Serial.println("Card failed, or not present");
    return false;
  } 
  else { 
    Serial.println("FSController initialized!");
    m_did_begin = true;
  }

  return true;
}


/* Pass in a char* to write data into SD card */
void FSController::logData(char *data) {
  if (!m_logMsg(data, m_DATA)) { 
    m_logMsg((char *)WRITE_ERR, m_DATA);
  }
}

/* Pass in a char* to write data into SD card */
void FSController::logDiag(char *data) {
  if (!m_logMsg(data, m_DIAG))
    m_logMsg((char *)WRITE_ERR, m_DIAG);
}

/* Creates needed files and only called during WAKE mode */
bool FSController::setupWakeCycle(char *timestamp, char *format) { MARK;
  Serial.println("Creating new wake cycle directory: ");
  Serial.println(timestamp);
  Serial.println(format);
  m_curDir = timestamp;
  m_cycles();

  m_mkFile(m_DATA); // Creates data.json 
  m_mkFile(m_DIAG); // Creates diag.json

  m_file.open(m_DATA, O_WRONLY | O_APPEND); // Open to write only and append data
  m_write(format);  // Writes in a certain format
  if (!m_file.close()) // Close file
    return false;
  return true;
}

/* Makes a file */
bool FSController::m_mkFile(const char *name) { MARK;
  // If it can't open and is closed, fail
  if (!(m_file.open(name, O_WRONLY | O_CREAT | O_EXCL) && m_file.close()))
    return false;
  return true;
}

/* Sets a file */
bool FSController::m_setFile(const char *file) { MARK;
  // If it can't open then fail
  if (!(m_file.open(file, O_WRONLY | O_APPEND)))
    return false;

  return true;
}

/* Call in a function to write */
bool FSController::m_write(char *msg) { MARK; 
  return m_file.println(msg); 
}

/* Collects data and writes it to respective file */
bool FSController::m_logMsg(const char *msg, const char *file) { MARK;
  char to_print[MAX_DATA_LEN]; // Allocate maximum space
  sprintf(to_print, "Writing %s to file %s \n", msg, file);
  Serial.println(to_print);
  if (!(m_setFile(file) && m_write((char *)msg))) { // Writes msg to SD card, otherwise fail
    Serial.println("Failed to write...");
    m_file.close(); // Close file
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