#ifndef _trophostat_h
#define _trophostat_h 1

#include "controller.h"

class Tropho : public Controller
{
  public:
    Tropho(Nephel &neph, Pump &pumpGood, Pump &pumpBad);

    int begin(void);
    int loop(void);
    void end(void) { }

    void readEeprom(unsigned int eepromBase);
    void writeEeprom(unsigned int eepromBase);
    void formatParams(char *buf, unsigned int buflen);
    void manualSetParams(void);

    const char *name(void) { return "Trophostat"; }
    char letter(void) { return 'f'; }
  protected:
    long mUpper(void) { return _mUpper; }
    long mTarget(void) { return _mTarget; }
    long mLower(void) { return _mLower; }
  
    unsigned long dutyNumer(void) { return _dutyNumer; }
    unsigned long dutyDenom(void) { return _dutyDenom; }
  
    unsigned long rtcRunSeconds(void) { return rtcSeconds() - _startSec; }
    int dutyPump(unsigned long runSeconds);
    unsigned long dutyFractionPercent(void);

    long measure(void) { return _neph.measure(); }
    void setPumpGood(void) { _pumpGood.setPumping(1); _pumpBad.setPumping(0); }
    void setPumpBad(void)  { _pumpGood.setPumping(0); _pumpBad.setPumping(1); }
    void setPumpNone(void) { _pumpGood.setPumping(0); _pumpBad.setPumping(0); }

    const Pump &pumpGood(void) { return _pumpGood; }
    const Pump &pumpBad(void) { return _pumpBad; }
  private:
    Nephel &_neph;
    Pump &_pumpGood;
    Pump &_pumpBad;

    long _mUpper;
    long _mTarget;
    long _mLower;

    unsigned long _dutyNumer;
    unsigned long _dutyDenom;
    
    long _startSec;
    long _startPumpGoodMsec;
    long _startPumpBadMsec;
};

#endif /* !defined(_trophostat_h) */
