#ifndef _supervisor_h
#define _supervisor_h 1

class Supervisor
{
  public:

    static const unsigned int outbufLen;
    static char outbuf[];

    /* Delay until a specified time in microseconds
 * Return the number of microseconds delayed
 * If the current time is at or after `until` do not delay and return 0.
 */
  static inline unsigned long delayIfNeeded(unsigned long until)
  {
    unsigned long now = micros();
    if (now < until) {
      delayMicroseconds(until - now);
      return until - now;
    } else {
      return 0;
    }
  }

  static int blockingReadLong(long *res);
};

#endif /* !defined(_supervisor) */
