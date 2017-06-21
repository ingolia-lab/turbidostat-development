#ifndef _turbidogradient_h
#define _turbidogradient_h 1

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

#endif /* !defined(_turbidogradient_h) */
