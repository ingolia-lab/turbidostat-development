#ifndef _turbidoschedule_h
#define _turbidoschedule_h 1

#include "controller.h"
#include "pump.h"
#include "turbidobase.h"
#include "turbidoconc.h"
#include "turbidomix.h"

/* GTR
 * Gradient, steps by Time, for media Ratio
 */
class TurbidoGradient : public TurbidoRatioBase
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

/* CTR
 * Cycle, steps by Time, for media Ratio
 */

class TurbidoCycle : public TurbidoRatioBase
{
  public:
    TurbidoCycle(Supervisor &s);
    
    void formatHeader(char *buf, unsigned int buflen);
    void formatLine(char *buf, unsigned int buflen, long currMeasure);

    void formatParams(char *buf, unsigned int buflen);
    void manualReadParams(void);

    const char *name(void) { return "Turbidostat Cycle Time Ratio"; }
    char letter(void) { return 'd'; }
    
  protected:
    uint8_t pump1Percent();
    
  private:
    uint8_t _pump1FirstPct;
    uint8_t _pump1SecondPct;
    long _firstTime;
    long _secondTime;
};

/* GTC = Gradient, steps by Time, for media Concentration */
class TurbidoConcGradient : public TurbidoConcBase
{
  public:
    TurbidoConcGradient(Supervisor &s);

    void formatHeader(char *buf, unsigned int buflen);
    void formatLine(char *buf, unsigned int buflen, long currMeasure);

    void formatParams(char *buf, unsigned int buflen);
    void manualReadParams(void);

    const char *name(void) { return "Turbidostat Gradient Time Conc"; }
    char letter(void) { return 'h'; }
    
  protected:
    unsigned long targetPpm1();
    
  private:
    unsigned long _startTargetPpm1;
    long _stepTargetPpm1;
    long _nSteps;
    long _stepTime;
};

class TurbidoConcLogGradient : public TurbidoConcBase
{
    public:
    TurbidoConcLogGradient(Supervisor &s);

    void formatHeader(char *buf, unsigned int buflen);
    void formatLine(char *buf, unsigned int buflen, long currMeasure);

    void formatParams(char *buf, unsigned int buflen);
    void manualReadParams(void);

    const char *name(void) { return "Turbidostat Log-Gradient Time Conc"; }
    char letter(void) { return 'l'; }
    
  protected:
    unsigned long targetPpm1();
    
  private:
    unsigned long _startTargetPpm1;
    unsigned long _stepPct;
    long _nSteps;
    long _stepTime;
};

/* CTC = Cycle, steps by Time, for media Concentration */
class TurbidoConcCycle: public TurbidoConcBase
{
  public:
    TurbidoConcCycle(Supervisor &s);

    void formatHeader(char *buf, unsigned int buflen);
    void formatLine(char *buf, unsigned int buflen, long currMeasure);

    void formatParams(char *buf, unsigned int buflen);
    void manualReadParams(void);

    const char *name(void) { return "Turbidostat Cycle Time Conc"; }
    char letter(void) { return 'e'; }
    
  protected:
    unsigned long targetPpm1();

  private:
    unsigned long _firstTargetPpm1;
    unsigned long _secondTargetPpm1;
    long _firstTime;
    long _secondTime;
};

#endif /* !defined(_turbidoschedule_h) */
