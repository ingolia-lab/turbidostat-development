#ifndef _turbidomix_h
#define _turbidomix_h 1

#include "controller.h"
#include "pump.h"
#include "turbidobase.h"

class TurbidoMixBase : public TurbidoBase
{
  public:
    TurbidoMixBase(Supervisor &s);

    int begin(void);

    void formatHeader(char *buf, unsigned int buflen);
    void formatLine(char *buf, unsigned int buflen, long currMeasure);

    void formatParams(char *buf, unsigned int buflen);
    void manualReadParams(void);

  protected:
    Pump &pump1(void);
    Pump &pump2(void);

    void setPumpOn(void);
    void setPump1On(void);
    void setPump2On(void);
    void setPumpOff(void);

    uint8_t pumpCountIncr(void) { unsigned long cycle = _cycleCount++; return (uint8_t) (cycle % 100); }

    virtual uint8_t pump1Percent() = 0;
  private:
    uint8_t _pump1;
    uint8_t _pump2;

    unsigned long _cycleCount;
};

class TurbidoMixFixed : public TurbidoMixBase
{
  public:
    TurbidoMixFixed(Supervisor &s);

    void formatHeader(char *buf, unsigned int buflen);
    void formatLine(char *buf, unsigned int buflen, long currMeasure);

    void formatParams(char *buf, unsigned int buflen);
    void manualReadParams(void);

    const char *name(void) { return "Turbidostat Ratio"; }
    char letter(void) { return 'r'; }
  protected:
    uint8_t pump1Percent() { return _pump1Pct; }

  private:
    uint8_t _pump1Pct;
};

#endif /* !defined(_turbidomix_h) */
