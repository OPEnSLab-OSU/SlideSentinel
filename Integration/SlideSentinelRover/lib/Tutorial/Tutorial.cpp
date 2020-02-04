#include "Tutorial.h"

/*
 * FIFO to hold received UART bytes before libsbp parses them.
 */
#define FIFO_LEN 512
char sbp_msg_fifo[FIFO_LEN];
u16 head = 0;
u16 tail = 0;

/* Return 1 if true, 0 otherwise. */
u8 fifo_empty(void)
{
  if (head == tail)
    return 1;
  return 0;
}

/*
 * Append a character to our SBP message fifo.
 * Returns 1 if char successfully appended to fifo.
 * Returns 0 if fifo is full.
 */
u8 fifo_write(char c){
  if (fifo_full())
    return 0;

  sbp_msg_fifo[tail] = c;
  tail = (tail+1) % FIFO_LEN;
  return 1;
}

/*
 * Read 1 char from fifo.
 * Returns 0 if fifo is empty, otherwise 1.
 */
u8 fifo_read_char(char *c) {
  if (fifo_empty())
    return 0;

  *c = sbp_msg_fifo[head];
  head = (head+1) % FIFO_LEN;
  return 1;
}

/*
 * Read arbitrary number of chars from FIFO. Must conform to
 * function definition that is passed to the function
 * sbp_process().
 * Returns the number of characters successfully read.
 */
u32 fifo_read(u8 *buff, u32 n, void *context) {
  int i;
  for (i=0; i<n; i++)
    if (!fifo_read_char((char *)(buff + i)))
      break;
  return i;
}

/* Return 1 if true, 0 otherwise. */
u8 fifo_full(void){
  if (((tail+1)%FIFO_LEN) == head) {
    return 1;
  }
  return 0;
}

