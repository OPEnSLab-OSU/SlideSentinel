#include "Controller.h"

Controller::Controller(const char *header, State *state)
    : m_state(state), m_HEADER(header) {}