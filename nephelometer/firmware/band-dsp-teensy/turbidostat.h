#ifndef _turbidostat_h
#define _turbidostat_h 1

#include "controller.h"

class Turbido : public Controller
{
  public:
    Turbido(Supervisor &s, int pumpno);

    int begin(void);
    int loop(void);
    void end(void) { }

    void readEeprom(unsigned int eepromBase);
    void writeEeprom(unsigned int eepromBase);
    void formatParams(char *buf, unsigned int buflen);
    void manualSetParams(void);

    const char *name(void) { return "Turbidostat"; }
    char letter(void) { return 't'; }
  protected:
    long mLower(void) { return _mLower; }
    long mUpper(void) { return _mUpper; }
    
    long measure(void);

    const Pump &pump(void);
    void setPumpOn(void);
    void setPumpOff(void);
  private:
    Supervisor &_s;
  
    long _mUpper;  // Measurement for pump-on
    long _mLower; // Measurement for pump-off
    long _pumpno;

    long _startSec;
    long _startPumpMsec;
        
};

/*Create new Stepping turbidostat class to implement OD stepping.*/

class StepTurbido : public Turbido
{
  public:
    StepTurbido(Supervisor &s, int pumpno);

    int begin(void);
    int loop(void);

    void readEeprom(unsigned int eepromBase);
    void writeEeprom(unsigned int eepromBase);
    void formatParams(char *buf, unsigned int buflen);
    void manualSetParams(void);
    
    const char *name(void) {return "Stepping Turbidostat";}
    char letter(void) {return 's';}
  private:
    Supervisor &_s;

    long _mUpper;  // Measurement for pump-on
    long _mLower; // Measurement for pump-off
    long _pumpno;
    
    long _startSec;
    long _startPumpMsec;
    
    long _stepLength;  //How long, in seconds, of each step
    long _stepSize;  //size, in OD, of each step
    long _conversion;  //IR->OD conversion
    long _startTime; //beginning time of each step, so we can track length of time
    long _step;  //define the step we are on
    long _stepMode;  //Use to break out of the stepping algorithm
};

#endif /* !defined(_turbidostat_h) */
