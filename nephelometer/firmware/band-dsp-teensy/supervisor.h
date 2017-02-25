#ifndef _supervisor_h
#define _supervisor_h 1

class Supervisor;

#include "controller.h"
#include "manual.h"
#include "nephelometer.h"
#include "pump.h"
#include "settings.h"
#include "turbidostat.h"

class Supervisor : protected ParamSettings
{
  public:
    Supervisor(void);

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

    inline Nephel &nephelometer(void) { return *_neph; }
    inline int nPumps(void) { return _nPumps; }
    inline Pump &pump(unsigned int pumpno) { if (pumpno >= _nPumps) { return _pumps[0]; } else { return _pumps[pumpno]; } }

    void begin(void);
    void loop(void);

    void serialWriteControllers(void);    
    void pickNextController(void);
    void manualSetupController(void);

    void useTestNephel(void) { _neph = new TestNephel(_pumps[0]); }
  protected:
    Controller &defaultController(void) { return _defaultController; }
    Controller &runningController(void) { return *_runningController; }
    Controller *pickController(void);

    void readEeprom(unsigned int);
    void writeEeprom(unsigned int);
    void manualSetParams(void);
    void formatParams(char *buf, unsigned int buflen);

  private:
    Nephel *_neph;

    static const unsigned int _nPumps = 2;
    static const int motor1Pin = 16;
    static const int motor2Pin = 17;
    Pump _pumps[_nPumps] = { Pump(motor1Pin, 1), Pump(motor2Pin, 1) };

    unsigned int _nControllers;
    Controller **_controllers;       

//    TrivialController _defaultController;
    ManualController _defaultController;
    
    Controller *_runningController;
    Controller *_nextController;

    void manualLoop(void);

    static const long version = 10000;
    static const unsigned int versionSlot = 0;
    static const unsigned int runningControllerSlot = 1;
    static const unsigned int nephelBase = 0x10;
    static const unsigned int controllerBase = 0x30;
};


#endif /* !defined(_supervisor) */
