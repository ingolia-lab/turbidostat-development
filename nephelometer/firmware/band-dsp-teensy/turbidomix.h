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

    void readEeprom(unsigned int eepromBase);
    void writeEeprom(unsigned int eepromBase);
    void formatParams(char *buf, unsigned int buflen);
    void manualSetParams(void);
    virtual void printHeader(void);
    virtual void printStatus(long m);     //see what needs to be input

    const char *name(void) { return "Turbidostat Mix"; }
    char letter(void) { return 'u'; }
  protected:
    long mLower(void) { return _mLower; }
    long mUpper(void) { return _mUpper; }
    long setmLower(long nLower) { return _mLower = nLower; }
    long setmUpper(long nLower) { return _mUpper = nUpper; }
    
    long measure(void);

    const Pump &pumpA(void);
    const Pump &pumpB(void);
    int eitherPumping(void) { return pumpA().isPumping() || pumpB().isPumping(); }
    void setPumpA(void);
    void setPumpB(void);
    void setPumpNone(void);

    unsigned long pumpCountIncr(void) { return _cycleCount++; }
    unsigned long pumpCycle(void) { return _pumpAShare + _pumpBShare; }
  private:
    Supervisor &_s;
  
    long _mUpper;  // Measurement for pump-on
    long _mLower; // Measurement for pump-off

    long _pumpA;
    long _pumpB;
    long _pumpAShare;
    long _pumpBShare;

    long _startSec;
    long _startPumpAMsec;
    long _startPumpBMsec;
    unsigned long _cycleCount;
};














/*Start StepTurbidoMix child class here*/

class StepTurbidoMix : public TurbidoMix
{
public:
    StepTurbidoMix(Supervisor &s);

    int begin(void);
    int loop(void);

    void readEeprom(unsigned int eepromBase);
    void writeEeprom(unsigned int eepromBase);
    void formatParams(char *buf, unsigned int buflen);
    void manualSetParams(void);
    void printHeader(void);
    void printStatus(long m);
    void setPumpOnCycle(void);

    const char *name(void) { return "Stepping Turbidostat Mix"; }
    char letter(void) { return 'v'; }
  private:




  
}


























#endif /* !defined(_turbidostat_h) */
