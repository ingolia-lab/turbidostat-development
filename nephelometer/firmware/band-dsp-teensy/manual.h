#ifndef _manual_h
#define _manual_h 1

/*
 * Manual commands that can be run from the supervisor
 */

#include "supervisor.h"

class ManualCommand
{
  public:
    // Create a manual command linked to the supervisor
    ManualCommand(Supervisor &s) : _supervisor(s) { }

    // Name of the manual command
    virtual const char *name(void);

    // One-letter command character to invoke the command
    virtual char letter(void);

    // Help information for the manual command
    virtual const char *help(void);

    // Execute the manual command (may block indefinitely pending user input, etc.)
    virtual void run(void);
  protected:
    Supervisor &supervisor(void) { return _supervisor; }
  private:
    Supervisor &_supervisor;
};

class ManualAnnotate : public ManualCommand
{
  public:
    ManualAnnotate(Supervisor &s) : ManualCommand(s) { }
    const char *name(void) { return "Annotate"; }
    char letter(void) { return 'a'; }
    const char *help(void) { return "Type a note into the log file"; }
    void run(void);
};

class ManualDelayScan : public ManualCommand
{
  public:
    ManualDelayScan(Supervisor &s) : ManualCommand(s) { }
    const char *name(void) { return "Delay Scan"; }
    char letter(void) { return 'd'; }
    const char *help(void) { return "Scan timing parameters for nephelometer measurements"; }
    void run(void);  
};

class ManualHelp : public ManualCommand
{
  public:
    ManualHelp(Supervisor &s) : ManualCommand(s) { }
    const char *name(void) { return "Help"; }
    char letter(void) { return 'h'; }
    const char *help(void) { return "Print help information"; }
    void run(void);
};

class ManualMeasure : public ManualCommand
{
  public:
    ManualMeasure(Supervisor &s) : ManualCommand(s) { }
    const char *name(void) { return "Measure"; }
    char letter(void) { return 'm'; }
    const char *help(void) { return "Take online measurements"; }
    void run(void);

    static const unsigned long intervalMsec = 500;
};

class ManualNephelSettings : public ManualCommand
{
  public:
    ManualNephelSettings(Supervisor &s) : ManualCommand(s) { }
    const char *name(void) { return "Nephelometer Settings"; }
    char letter(void) { return 'n'; }
    const char *help(void) { return "Set nephelometer timing parameters and gain"; }
    void run(void);
};

class ManualPump : public ManualCommand
{
  public:
    ManualPump(Supervisor &s) : ManualCommand(s) { }
    const char *name(void) { return "Pump"; }
    char letter(void) { return 'p'; }
    const char *help(void) { return "Manually switch on a pump"; }
    void run(void);  
};

#endif /* !defined(_manual_h) */
