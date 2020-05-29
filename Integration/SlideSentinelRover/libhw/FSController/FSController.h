#ifndef _FSCONTROLLER_H_
#define _FSCONTROLLER_H_

#include <Arduino.h>
#include <string.h>
#include "Controller.h"
#include "SdFat.h"

#define MAIN "data"
#define SD_SIZE 16 // GB

// TODO consider deep copying over the timestamp so we dont depend on a string
// not within the FSController class
class FSController : public Controller {
private:
  SdFat m_sd;
  SdFile m_file;
  SdFile m_root;
  uint8_t m_cs;
  uint8_t m_rst;
  bool m_did_begin;

  const char *m_DATA;
  const char *m_DIAG;
  char *m_curDir;

  // TODO add error count diagnostic info
  float m_spaceMB; // diagnostic
  int m_cycle;     // diagnostic
  void m_cycles();
  void m_SDspace();

  bool m_logMsg(const char *msg, const char *file);
  bool m_write(char *msg);
  bool m_mkFile(const char *name);
  bool m_setFile(const char *name);

public:
  FSController(uint8_t cs, uint8_t rst);
  void logData(char *data);
  void logDiag(char *data);
  bool setupWakeCycle(char *timestamp, char *format);
  bool init();
  void status(SSModel &model);
  void update(SSModel &model);
};

#endif // _FSCONTROLLER_H_
