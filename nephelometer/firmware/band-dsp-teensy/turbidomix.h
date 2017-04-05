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
    virtual void printHeader(void);       //General print header so it can be modified in the child StepTurbidoMix
    virtual void printStatus(long m);     //General print status so that it can be modified by the child StepTurbidoMix.

    const char *name(void) { return "Turbidostat Mix"; }
    char letter(void) { return 'u'; }
  protected:
    long mLower(void) { return _mLower; }
    long mUpper(void) { return _mUpper; }
    long setmLower(long nLower) { _mLower = nLower; }
    long setmUpper(long nUpper) { _mUpper = nUpper; }
    long getStartSec(void) { return _startSec; }
    long getPumpAShare(void) { return _pumpAShare; }
    long getPumpBShare(void) { return _pumpBShare; }
    long setPumpShares(long nPAS, long nPBS) { _pumpAShare = nPAS, _pumpBShare = nPBS; }    
    long getPGAScale(void) { return _s.nephelometer().pgaScale(); }

    long measure(void);

    const Pump &pumpA(void);
    const Pump &pumpB(void);
    int eitherPumping(void) { return pumpA().isPumping() || pumpB().isPumping(); }
    void setPumpA(void);
    void setPumpB(void);
    void setPumpNone(void);
    void setPumpOnCycle(void);
    long getPumpA(void) {return _pumpA; }
    long getPumpB(void) {return _pumpB; }
    long setNumPumpA(long newA) { _pumpA = newA; }
    long setNumPumpB(long newB) { _pumpB = newB; }

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

/******************StepTurbidoMix child class**********************************/
/* JBB, 2017_04_05
 * The StepTurbidoMix is a child of TurbidoMix that automatically steps through several pump ratios.
 * The default parameters give a step time of 3 Yeast growth cycles, so 4.5hrs per cycle, for 58.5hrs run time to complete every cycle. 
 * The parameter is in units of seconds.
 * The default stepping is a geometric series on 0.71. This gives 13 different levels from 100 to 0.
 * @ _stepRate = 0.71, pumpA shares are: 100, 71, 50, 35, 24, 17, 12, 8, 5, 3, 2, 1, 0. 
 * PumpA is assumed to be the good media pump at placement 0.
 * PumpB is assumed to be the bad media pump at placement 1. 
 *  
 *  
 *  
 */

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

    const char *name(void) { return "Stepping Turbidostat Mix"; }
    char letter(void) { return 'v'; }
  private:
    long _stepLength;  //How long, in seconds, of each step
    long _startTime; //beginning time of each step, so we can track length of time
    long _stepRate;  //Rate of decrease in our geometric series. Must be a fraction of 1. 
};

#endif /* !defined(_turbidostat_h) */
