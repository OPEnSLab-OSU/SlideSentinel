#pragma once

#include "tinyfsm.h"

struct Update : tinyfsm::Event { };
struct PowerUp : tinyfsm::Event { };
struct PowerDown : tinyfsm::Event { };
struct Panic : tinyfsm::Event { };