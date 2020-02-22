#ifndef _FSCONTROLLER_H_
#define _FSCONTROLLER_H_

#include <Arduino.h>
#include "ArduinoJson.h"
#include "Controller.h"
#include "SdFat.h"
#include <string.h>

class FSController : public Controller {
private:
  SdFat m_sd;
  SdFile m_file;
  SdFile m_root;
  uint8_t m_cs;
  uint8_t m_rst;

  const char *m_WRITE_ERR;
  const char *m_GNSS;
  const char *m_LOG;

  bool m_dispatch(JsonDocument &doc);
  bool m_logMsg(const char *msg, const char *file);
  bool m_write(char *msg);
  bool m_mkFile(const char *name);
  bool m_setFile(const char *name);

public:
  FSController(uint8_t cs, uint8_t rst);
  void log(JsonDocument &doc);
  bool setupWakeCycle(char *timestamp, char *format);
  bool init();
  void update(JsonDocument &doc);
  void status(uint8_t verbosity, JsonDocument &doc);
};

#endif // _FSCONTROLLER_H_
