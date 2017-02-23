#ifndef _turbidostat_h
#define _turbidostat_h 1

#include "controller.h"

class Turbido : public Controller
{
  public:
    Turbido(Nephel &neph, Pump &pump);

    int begin(void);
    int loop(void);

    void readEeprom(unsigned int eepromBase);
    void writeEeprom(unsigned int eepromBase);
    void formatParams(char *buf, unsigned int buflen);
    void manualSetParams(void);

    const char *name(void) { return "Turbidostat"; }
    char letter(void) { return 't'; }
    
  private:
    Nephel &_neph;
    Pump &_pump;
  
    long _pumpOn;  // Measurement for pump-on
    long _pumpOff; // Measurement for pump-off

    long _startSec;
    long _startPumpMsec;
};

#endif /* !defined(_turbidostat_h) */
