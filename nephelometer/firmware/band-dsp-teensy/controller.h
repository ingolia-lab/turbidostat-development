#ifndef _controller_h
#define _controller_h 1

#include "settings.h"

/*
 * A Controller provides an event loop to monitor and manage growth automatically.
 */

class Controller : public ParamSettings {
  public:
    Controller() { }

    // Called once when a controller begins to monitor and manage.
    // Return 0 if all is well and non-zero for a problem indicating that the controller cannot run
    virtual int begin(void);

    // Called once per second to allow the controller to monitor and manage.
    // Return 0 to continue control and non-zero when control should return to manual
    virtual int loop(void);

    // Called when control is switched to a different controller while this is running
    // NOT called after loop() returns non-zero
    virtual void end(void);

    // Name of the controller algorithm
    virtual const char *name(void);

    // One-letter character for selecting the controller
    virtual char letter(void);

  protected:
};

#endif /* !defined(_controller_h) */
