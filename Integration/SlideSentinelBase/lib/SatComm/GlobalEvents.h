#pragma once

#include "tinyfsm.h"

struct Update : tinyfsm::Event { };
struct PowerUp : tinyfsm::Event { };
struct PowerDown : tinyfsm::Event { };
struct Panic : tinyfsm::Event { };
struct SyncTime : tinyfsm::Event {
  SyncTime(const tm& time_in)
      : time(time_in) {}

  tm time;
};