#pragma once

/******** Data Size ********/
#define MAX_DATA_LEN 1000

#define consts constexpr const char *
#define consti constexpr int

namespace errorMsg {
consts WRITE_ERR = "ERROR: failed to write to SD card";
consts REPLY_ERR = "ERROR: no reply from server";
consts ACK_ERR = "ERROR: no ack from server";
consts SER_ERR = "ERROR: failed to serialize data";
consts CHNNL_ERR = "ERROR: channel currently busy";
} // namespace Constants
