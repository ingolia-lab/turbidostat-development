#ifndef _controller_h
#define _controller_h 1

#include "supervisor.h"

class Controller {
  public:
    virtual int begin(void);
    virtual int loop(void);

    virtual void readEeprom(unsigned int);
    virtual void writeEeprom(unsigned int);
    virtual void manualSetParams(void);
    virtual void formatParams(char *buf, unsigned int buflen);
    inline void serialWriteParams(void) 
    { 
      Serial.print(F("\r\n# Current settings:\r\n"));
      formatParams(Supervisor::outbuf, Supervisor::outbufLen);
      Serial.write(Supervisor::outbuf);
    }

    virtual const char *name(void);
    virtual char letter(void);

    static const int loopMsec = 1000;
  protected:
    static void writeEepromLong(unsigned int base, unsigned int slot, long value);
    static long readEepromLong(unsigned int base, unsigned int slot);
    static void manualReadParam(const char *desc, long &pval);

    static unsigned long rtcSeconds(void) { return millis() / ((unsigned long) 1000); }
};

#endif /* !defined(_controller_h) */
