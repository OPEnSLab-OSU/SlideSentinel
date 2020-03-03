#pragma once

#define consts constexpr const char *
#define consti constexpr int

namespace ErrorMsg {
consts WRITE_ERR = "ERROR: failed to write to SD card";
consts REPLY_ERR = "ERROR: no reply from server";
consts ACK_ERR = "ERROR: no ack from server";
consts SER_ERR = "ERROR: failed to serialize data";
} // namespace ErrorMsg
