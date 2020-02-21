#ifndef _FSCONTROLLER_H_
#define _FSCONTROLLER_H_
#define MAX_PATH_SIZE 30

#include <Arduino.h>
#include <string.h>
#include "ArduinoJson.h"
#include "Controller.h"
#include "SdFat.h"

class FSController : public Controller {
private:
  SdFat m_sd;
  SdFile m_file;
  SdFile m_root;
  uint8_t m_cs;
  uint8_t m_rst;
  char m_buf[MAX_PATH_SIZE];

  const char *m_WRITE_ERR;
  const char *m_GNSS;
  const char *m_STATE;
  const char *m_ERR;

  bool m_dispatch(JsonDocument &doc);

  bool m_logErr(const char *gnss);
  bool m_logState(const char *gnss);
  bool m_logGNSS(const char *gnss);

  void m_clearBuffer();
  void m_writeHeader();
  bool m_mkFile(const char *name);
  bool m_setFile(const char *name);

public:
  FSController(uint8_t cs, uint8_t rst);
  void log(JsonDocument &doc);
  bool newCycle(char *timestamp);
  bool init();
  void update(JsonDocument &doc);
  void status(uint8_t verbosity, JsonDocument &doc);
};

#endif // _FSCONTROLLER_H_
