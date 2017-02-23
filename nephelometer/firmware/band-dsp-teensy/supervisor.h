#ifndef _supervisor_h
#define _supervisor_h 1

class Supervisor;

#include "controller.h"
#include "manual.h"
#include "nephelometer.h"
#include "pump.h"
#include "settings.h"
#include "turbidostat.h"

class Supervisor
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

    inline Nephel &nephelometer(void) { return _neph; }
    inline int nPumps(void) { return _nPumps; }
    inline Pump &pump(unsigned int pumpno) { if (pumpno >= _nPumps) { return _pumps[0]; } else { return _pumps[pumpno]; } }

    void begin(void);
    void loop(void);
    void help(void);
    
    int pickController(void);
    void setupController(int);
    int startController(int);

    // Returns the time in real-time clock seconds
    static unsigned long rtcSeconds(void) { return millis() / ((unsigned long) 1000); }

  private:
    Nephel _neph;

    static const unsigned int _nPumps = 2;
    static const int motor1Pin = 16;
    static const int motor2Pin = 17;
    Pump _pumps[_nPumps] = { Pump(motor1Pin, 1), Pump(motor2Pin, 1) };

    unsigned int _nCommands;
    ManualCommand **_commands;
    char *_commandChars;

    unsigned int _nControllers;
    Controller **_controllers;

    // Number of running controller, or -1 for manual
    int _runningController;
    // Controller to start next time, or -1 for no change
    int _nextController;
    // RTC seconds of previous loop
    unsigned long _rtcPrevious;

    const char *_version = "band-dsp-teensy 2017-02-22";

    void manualLoop(void);
};


#endif /* !defined(_supervisor) */
