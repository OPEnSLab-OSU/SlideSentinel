#ifndef _FSCONTROLLER_H_
#define _FSCONTROLLER_H_

#include "BaseModel.h"
#include "Controller.h"
#include "SdFat.h"
#include "constants.hpp"
#include <Arduino.h>
#include <string.h>

#define MAIN "rovers"
#define DIAG "diag.json"
#define PROPS "props.json"
#define DATA "data.json"
#define ERROR "error.json"
#define BUF_SIZE 50
#define SD_SIZE 16 // GB

using namespace errorMsg;

// TODO consider deep copying over the timestamp so we dont depend on a string
// not within the FSController class
class FSController : public Controller {
private:
  SdFat m_sd;
  SdFile m_file;
  SdFile m_root;

  uint8_t m_cs;
  uint8_t m_rst;
  int m_num_rovers;
  float m_spaceMB; // diagnostic
  char m_buf[BUF_SIZE];

  char *m_curDir;

  bool m_log(int rover_num, const char *msg, const char *file);
  bool m_write(char *msg);

  bool m_chgDir(int rover_num);
  bool m_mkFile(const char *name);
  bool m_setFile(int rover_num, const char *file);

  bool m_setup();
  void m_clear();

  void m_SDspace();

public:
  FSController(uint8_t cs, uint8_t rst, int num_rovers);

  void logData(int rover_num, char *data);
  void logDiag(int rover_num, char *diag);
  void logProps(int rover_num, char *props);

  bool init();
  void status(BaseModel &model);
  void update(BaseModel &model);
};

#endif // _FSCONTROLLER_H_
