#ifndef _turbidoschedule_h
#define _turbidoschedule_h 1

#include "controller.h"
#include "pump.h"
#include "turbidobase.h"
#include "turbidomix.h"

class TurbidoGradient : public TurbidoMixBase
{
  public:
    TurbidoGradient(Supervisor &s);

    void formatHeader(char *buf, unsigned int buflen);
    void formatLine(char *buf, unsigned int buflen, long currMeasure);

    void formatParams(char *buf, unsigned int buflen);
    void manualReadParams(void);

    const char *name(void) { return "Turbidostat Mix Gradient"; }
    char letter(void) { return 'g'; }
    
  protected:
    uint8_t pump1Percent();
    
  private:
    uint8_t _pump1StartPct;
    uint8_t _pump1StepPct;
    long _nSteps;
    long _stepTime;
};

class TurbidoCycle : public TurbidoMixBase
{
  public:
    TurbidoCycle(Supervisor &s);
    
    void formatHeader(char *buf, unsigned int buflen);
    void formatLine(char *buf, unsigned int buflen, long currMeasure);

    void formatParams(char *buf, unsigned int buflen);
    void manualReadParams(void);

    const char *name(void) { return "Turbidostat Mix Cycle"; }
    char letter(void) { return 'c'; }
    
  protected:
    uint8_t pump1Percent();
    
  private:
    uint8_t _pump1FirstPct;
    uint8_t _pump1SecondPct;
    long _firstTime;
    long _secondTime;
};

#endif /* !defined(_turbidoschedule_h) */
