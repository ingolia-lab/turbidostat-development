#ifndef _trophostat_h
#define _trophostat_h 1

#include "controller.h"

class Tropho : public Controller
{
  public:
    Tropho(Supervisor &s, int goodPumpno = 0, int badPumpno = 1);

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

    long measure(void) { return _s.nephelometer().measure(); }
    void setPumpGood(void) { _s.pump(_goodPumpno).setPumping(1); _s.pump(_badPumpno).setPumping(0); }
    void setPumpBad(void)  { _s.pump(_goodPumpno).setPumping(0); _s.pump(_badPumpno).setPumping(1); }
    void setPumpNone(void) { _s.pump(_goodPumpno).setPumping(0); _s.pump(_badPumpno).setPumping(0); }

    const Pump &pumpGood(void) { return _s.pump(_goodPumpno); }
    const Pump &pumpBad(void) { return _s.pump(_badPumpno); }
  private:
    Supervisor &_s;

    long _mUpper;
    long _mTarget;
    long _mLower;

    unsigned long _dutyNumer;
    unsigned long _dutyDenom;

    long _goodPumpno;
    long _badPumpno;
    
    long _startSec;
    long _startPumpGoodMsec;
    long _startPumpBadMsec;
};

#endif /* !defined(_trophostat_h) */
