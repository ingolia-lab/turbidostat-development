#ifndef _util_h
#define _util_h 1

#include <stdio.h>

extern const unsigned int outbufLen;
extern char outbuf[];

/* Delay until a specified time in microseconds
 * Return the number of microseconds delayed
 * If the current time is at or after `until` do not delay and return 0.
 */
inline unsigned long delayIfNeeded(unsigned long until)
{
  unsigned long now = micros();
  if (now < until) {
    delayMicroseconds(until - now);
    return until - now;
  } else {
    return 0;
  }
}

/* Print a microseconds time to Serial
 * Print the number of seconds and (as a decimal) milliseconds in a microseconds time t
 */
inline void formatMicros(char *buf, unsigned int buflen, unsigned long t)
{
  snprintf(buf, buflen, "%lu.%lu%lu%lu", t / ((unsigned long) 1000000),
  t / ((unsigned long)  100000) % 10,
  t / ((unsigned long)   10000) % 10,
  t / ((unsigned long)    1000) % 10);
}

#endif /* defined(_util_h) */
