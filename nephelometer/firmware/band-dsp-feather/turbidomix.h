#ifndef _turbidomix_h
#define _turbidomix_h 1

#include "controller.h"
#include "pump.h"

class TurbidoMix : public Controller
{
  public:
    TurbidoMix(Supervisor &s);

    int begin(void);
    int loop(void);
    void end(void) { }

    void formatParams(char *buf, unsigned int buflen);
    void manualSetParams(void);

    const char *name(void) { return "Turbidostat Mix"; }
    char letter(void) { return 'u'; }
  protected:
    long mLower(void) { return _mLower; }
    long mUpper(void) { return _mUpper; }
    
    long measure(void);

    const Pump &pump1(void);
    const Pump &pump2(void);
    int eitherPumping(void) { return pump1().isPumping() || pump2().isPumping(); }
    void setPump1(void);
    void setPump2(void);
    void setPumpNone(void);

    unsigned long pumpCountIncr(void) { return _cycleCount++; }
    void setPumpOnCycle(void);
  private:
    Supervisor &_s;
  
    long _mUpper;  // Measurement for pump-on
    long _mLower; // Measurement for pump-off

    uint8_t _pump1;
    uint8_t _pump2;
    uint8_t _pump1Pct;
    
    long _startSec;
    long _startPump1Msec;
    long _startPump2Msec;
    unsigned long _cycleCount;
};

#endif /* !defined(_turbidostat_h) */
