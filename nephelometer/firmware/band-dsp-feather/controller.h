#ifndef _controller_h
#define _controller_h 1

#include "settings.h"

/*
 * A Controller provides an event loop to monitor and manage growth automatically.
 */

class Controller : public ParamSettings {
  public:
    Controller() : _rtcPrevious(0) { }

    // Called once when a controller begins to monitor and manage.
    // Return 0 if all is well and non-zero for a problem indicating that the controller cannot run
    virtual int begin(void);

    // Called once per second to allow the controller to monitor and manage.
    // Return 0 to continue control and non-zero when control should return to manual
    virtual int loop(void);

    // Called when control is switched to a different controller while this is running
    // This IS called after loop() returns non-zero on the next loop
    virtual void end(void);

    // Name of the controller algorithm
    virtual const char *name(void);

    // One-letter character for selecting the controller
    virtual char letter(void);

  protected:
    // Returns the time in real-time clock seconds
    static unsigned long rtcSeconds(void) { return millis() / ((unsigned long) 1000); }

    int delayOneSecond(void);
  private:
    unsigned long _rtcPrevious;
};

class TrivialController : public Controller
{
  public:
    TrivialController(void) { Serial.println("TrivialController::TrivialController()"); }
    int begin(void) { Serial.println("TrivialController::begin()"); return 0; }
    int loop(void) { delayOneSecond(); Serial.println("TrivialController::loop()"); return 0; }
    void end(void) { Serial.println("TrivialController::loop()"); }
    const char *name(void) { return "Trivial Controller"; }
    char letter(void) { return 'z'; }
    void readEeprom(unsigned int eepromBase) { /* No parameters */ }
    void writeEeprom(unsigned int eepromBase) { /* No parameters */ }
    void manualSetParams(void) { /* No parameters */ }
    void formatParams(char *buf, unsigned int buflen) { buf[0] = 0; }

};

#endif /* !defined(_controller_h) */
