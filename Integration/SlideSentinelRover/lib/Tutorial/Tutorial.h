#include "common.h"

#define DO_EVERY(n, cmd) do { \
  static u32 do_every_count = 0; \
  if (do_every_count % (n) == 0) { \
    cmd; \
  } \
  do_every_count++; \
} while(0)

/* FIFO functions */
u8 fifo_empty(void);
u8 fifo_full(void);
u8 fifo_write(char c);
u8 fifo_read_char(char *c);
u32 fifo_read(u8 *buff, u32 n, void *context);
