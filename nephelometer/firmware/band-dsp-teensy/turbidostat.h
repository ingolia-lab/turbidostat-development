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
    virtual void printHeader(void);
    virtual void printStatus(long m);

    const char *name(void) { return "Turbidostat"; }
    char letter(void) { return 't'; }
  protected:
    long mLower(void) { return _mLower; }
    long mUpper(void) { return _mUpper; }
    long getStartSec(void) { return _startSec; }
    void setBounds(long nUpper, long nLower) {_mUpper = nUpper, _mLower = nLower;}
    long getPGAScale(void);
    int  getPumpNo(void) { return _pumpno;}
    int  setPumpNo(int nPumpNo) { _pumpno = nPumpNo; } 
    
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

/******************StepTurbido child class**********************************/
/*  JBB, 2017_04_05
 *  StepTurbido is a child class of Turbidostat. It steps through Optical density measurements
 *  from an initial step to a final OD of 1. 
 *  Default time is 16,200 seconds. This is 3 generations, assuming a generation time of 1.5 hrs.
 *  Default step size is 0.1 O.D. 
 *  The default conversion is 2000. T***his needs to be changed/calibrated every experiment*** 
 *  The initial step is the next lowest step size.
 *  ie. You want steps of 0.1 O.D., but you start the culture at 0.137 O.D. When you enter the 
 *  conversion factor, there is an algorithm to determine what IR to stay between, and the first
 *  step is determined as 0.2 O.D. This is the next smallest step above where you started, to 
 *  prevent the Arduino from pumping media initially in an attempt to dilute the culture. 
 */

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
    void printHeader(void);
    void printStatus(long m);
    
    const char *name(void) {return "Stepping Turbidostat";}
    char letter(void) {return 's';}
  private:
    long _stepLength;  //How long, in seconds, of each step
    long _stepSize;  //size, in OD, of each step
    long _conversion;  //IR->OD conversion
    long _startTime; //beginning time of each step, so we can track length of time
    long _step;  //define the step we are on
    long _stepMode;  //Use to break out of the stepping algorithm
    
};

#endif /* !defined(_turbidostat_h) */
