#ifndef _turbidostat_h
#define _turbidostat_h 1

#include "controller.h"

class Turbido : public Controller
{
  public:
    Turbido(Nephel &neph, Pump &pump);

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
    
    long measure(void) { return _neph.measure(); }

    const Pump &pump(void) { return _pump; }
    void setPumpOn(void)  { _pump.setPumping(1); }
    void setPumpOff(void) { _pump.setPumping(0); }
  private:
    Nephel &_neph;
    Pump &_pump;
  
    long _mUpper;  // Measurement for pump-on
    long _mLower; // Measurement for pump-off

    long _startSec;
    long _startPumpMsec;
};

#endif /* !defined(_turbidostat_h) */
